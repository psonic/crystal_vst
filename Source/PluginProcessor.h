#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <random>
#include <vector>

struct Grain {
  int startSample;
  int currentSample;
  int duration;
  float pitchRatio;
  float amplitude;
  bool active = false;
  bool isReversed = false;
  int attackSamples = 0;
  int decaySamples = 0;
  
  bool isLooping = false;
  int loopDuration = 0; // Length of the loop in samples

  int delaySamples = 0; // Samples to wait before starting playback
  bool waitingToStart = false;

  float panStart = 0.5f; 
  float panDrift = 0.0f; 

  // Per-Grain Filter State (Tpt Filter / SVF)
  float v1 = 0.0f, v2 = 0.0f; 
  float filterStartFreq = 20000.0f;
  float filterEndFreq = 20000.0f;
  float filterRes = 0.707f;
  bool filterActive = false;

  void process(const juce::AudioBuffer<float> &sourceBuffer,
               juce::AudioBuffer<float> &outputBuffer, int writePtr,
               int bufferSize, double sampleRate) {
    if (!active) {
        if (waitingToStart) {
            if (--delaySamples <= 0) {
                waitingToStart = false;
                active = true;
                currentSample = 0;
                v1 = 0.0f; v2 = 0.0f; // Reset filter state
            }
        }
        return;
    }

    float phase = (float)currentSample * pitchRatio;
    
    // Looping logic (restored)
    float monoSample = 0.0f;
    if (isLooping && loopDuration > 512) {
        float loopPos = std::fmod(phase, (float)loopDuration);
        int xfadeSamples = 256;
        
        int readIdx1 = (startSample + (int)loopPos + bufferSize) % bufferSize;
        float s1 = 0.0f;
        for (int c = 0; c < sourceBuffer.getNumChannels(); ++c) s1 += sourceBuffer.getReadPointer(c)[readIdx1];
        s1 /= (float)sourceBuffer.getNumChannels();

        if (loopPos > (float)(loopDuration - xfadeSamples)) {
            float xfade = (loopPos - (float)(loopDuration - xfadeSamples)) / (float)xfadeSamples;
            int readIdx2 = (startSample + (int)(loopPos - (float)loopDuration) + bufferSize) % bufferSize;
            float s2 = 0.0f;
            for (int c = 0; c < sourceBuffer.getNumChannels(); ++c) s2 += sourceBuffer.getReadPointer(c)[readIdx2];
            s2 /= (float)sourceBuffer.getNumChannels();
            monoSample = (s1 * (1.0f - xfade)) + (s2 * xfade);
        } else {
            monoSample = s1;
        }
    } else {
        float relativePos = phase;
        if (isReversed) relativePos = (float)duration - relativePos;
        int readIdx = (startSample + (int)relativePos + bufferSize) % bufferSize;
        for (int c = 0; c < sourceBuffer.getNumChannels(); ++c)
            monoSample += sourceBuffer.getReadPointer(c)[readIdx];
        monoSample /= (float)sourceBuffer.getNumChannels();
    }

    // Per-Grain Modulating Filter
    if (filterActive) {
        float progress = (float)currentSample / (float)duration;
        float freq = filterStartFreq + (filterEndFreq - filterStartFreq) * progress;
        if (freq < 20.0f) freq = 20.0f;
        if (freq > 20000.0f) freq = 20000.0f;

        float g = std::tan(juce::MathConstants<float>::pi * freq / (float)sampleRate);
        float k = 1.0f / filterRes;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float mono_v0 = (monoSample - k * v2 - g * v1) * a1;
        float v1_next = g * mono_v0 + v1;
        float v2_next = g * v1_next + v2;
        
        monoSample = v2_next; 
        v1 = v1_next;
        v2 = v2_next;
    }

    // Distribution & Window
    float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (float)currentSample / (float)duration));
    float env = 1.0f;
    if (currentSample < attackSamples && attackSamples > 0)
      env = (float)currentSample / (float)attackSamples;
    else if (currentSample > (duration - decaySamples) && decaySamples > 0)
      env = (float)(duration - currentSample) / (float)decaySamples;

    float totalGain = window * env * amplitude;
    
    // Kinetic Panning with Bouncing
    float p = panStart + panDrift * (float)currentSample;
    
    // Robust ping-pong fold between 0.0 and 1.0
    // Shift to guaranteed positive domain, wrap to 2.0, fold at 1.0
    p = p + 10000.0f; 
    p = std::fmod(p, 2.0f);
    if (p > 1.0f) {
        p = 2.0f - p;
    }
    
    float panL = std::cos(p * juce::MathConstants<float>::halfPi);
    float panR = std::sin(p * juce::MathConstants<float>::halfPi);

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {
        float channelGain = (channel == 0) ? panL : (channel == 1 ? panR : 1.0f);
        outputBuffer.getWritePointer(channel)[writePtr] += monoSample * totalGain * channelGain;
    }

    currentSample++;
    if (currentSample >= duration)
      active = false;
  }
};

class CrystalVstAudioProcessor : public juce::AudioProcessor {
public:
  CrystalVstAudioProcessor();
  ~CrystalVstAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "CrystalVST"; }

  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
  const juce::String getProgramName(int index) override { juce::ignoreUnused(index); return {}; }
  void changeProgramName(int index, const juce::String &newName) override {
      juce::ignoreUnused(index, newName);
  }

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  juce::AudioProcessorValueTreeState apvts;

  float getInputLevel() const { return inputLevel.load(); }
  float getOutputLevel() const { return outputLevel.load(); }

private:
  juce::AudioBuffer<float> circularBuffer;
  int writePosition = 0;

  static constexpr int maxGrains = 64;
  std::array<Grain, maxGrains> grains;
  int samplesSinceLastGrain = 0;

  std::mt19937 randomEngine;
  
  // Sine Chord Generator
  void generateRandomChord();
  std::array<float, 6> chordFrequencies;
  std::array<float, 6> currentSinePhases;

  // Effects
  juce::dsp::Reverb reverb;
  juce::dsp::Phaser<float> phaser;
  juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>> filterChain;
  
  // Smoothed Parameters for Glitch-Free changes
  juce::LinearSmoothedValue<float> smoothedGain;
  juce::LinearSmoothedValue<float> smoothedMix;
  juce::LinearSmoothedValue<float> smoothedHpfFreq;
  
  // FX Modulation Smoothers
  juce::LinearSmoothedValue<float> smoothedReverbRoom;
  juce::LinearSmoothedValue<float> smoothedPhaserFreq;
  juce::LinearSmoothedValue<float> smoothedPhaserFeedback;

  // Chord Smoothing
  std::array<juce::LinearSmoothedValue<float>, 6> smoothedChordFreqs;

  std::atomic<float> inputLevel{0.0f};
  std::atomic<float> outputLevel{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrystalVstAudioProcessor)
};
