#include "PluginProcessor.h"
#include "PluginEditor.h"

CrystalVstAudioProcessor::CrystalVstAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()) {
  std::random_device rd;
  randomEngine.seed(rd());
  
  currentSinePhases.fill(0.0f);
  for (auto& s : smoothedChordFreqs) s.reset(44100.0, 0.1);
  generateRandomChord();
}

CrystalVstAudioProcessor::~CrystalVstAudioProcessor() {}

void CrystalVstAudioProcessor::prepareToPlay(double sampleRate,
                                             int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  circularBuffer.setSize(getTotalNumInputChannels(), (int)(sampleRate * 2.0));
  circularBuffer.clear();
  writePosition = 0;
  samplesSinceLastGrain = 0;

  for (auto &grain : grains)
    grain.active = false;

  // Initialize Effects
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

  reverb.prepare(spec);
  reverb.setParameters({0.5f, 0.5f, 0.5f, 0.5f, 0.1f, 0.0f});

  phaser.prepare(spec);
  phaser.setRate(0.5f);
  phaser.setDepth(0.5f);
  phaser.setCentreFrequency(1000.0f);
  phaser.setFeedback(0.5f);
  
  filterChain.prepare(spec);

  smoothedGain.reset(sampleRate, 0.1);
  smoothedMix.reset(sampleRate, 0.1);
  smoothedHpfFreq.reset(sampleRate, 0.1);
  smoothedReverbRoom.reset(sampleRate, 0.1);
  smoothedPhaserFreq.reset(sampleRate, 0.1);
  smoothedPhaserFeedback.reset(sampleRate, 0.1);

  for (auto& s : smoothedChordFreqs) s.reset(sampleRate, 0.1);
  
  smoothedGain.setCurrentAndTargetValue(apvts.getRawParameterValue("GAIN")->load());
  smoothedMix.setCurrentAndTargetValue(apvts.getRawParameterValue("MIX")->load());
  smoothedHpfFreq.setCurrentAndTargetValue(apvts.getRawParameterValue("HPF_FREQ")->load());
  smoothedReverbRoom.setCurrentAndTargetValue(0.5f);
  smoothedPhaserFreq.setCurrentAndTargetValue(1000.0f);
  smoothedPhaserFeedback.setCurrentAndTargetValue(0.5f);

  auto hpfFreq = apvts.getRawParameterValue("HPF_FREQ")->load();
  *filterChain.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpfFreq, 0.707f);
}

void CrystalVstAudioProcessor::releaseResources() {
    juce::ignoreUnused(this);
}

bool CrystalVstAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

void CrystalVstAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                            juce::MidiBuffer &midiMessages) {
  juce::ignoreUnused(midiMessages);
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  float density = apvts.getRawParameterValue("DENSITY")->load(); // Grains per beat
  float lifeMin = apvts.getRawParameterValue("LIFE_MIN")->load();
  float lifeMax = apvts.getRawParameterValue("LIFE_MAX")->load();
  float loopCycleMaxBeats = apvts.getRawParameterValue("LOOP_BEATS")->load();
  float delayProb = apvts.getRawParameterValue("DELAY_PROB")->load();
  float delayMaxBeats = apvts.getRawParameterValue("DELAY_MAX")->load();
  int inputSource = (int)apvts.getRawParameterValue("INPUT_SOURCE")->load();
  float mix = apvts.getRawParameterValue("MIX")->load();
  float gain = apvts.getRawParameterValue("GAIN")->load();
  float revProb = apvts.getRawParameterValue("REVERSE_PROB")->load();
  float attackMs = apvts.getRawParameterValue("ATTACK")->load();
  float decayMs = apvts.getRawParameterValue("DECAY")->load();
  float hpfFreq = apvts.getRawParameterValue("HPF_FREQ")->load();

  smoothedGain.setTargetValue(gain);
  smoothedMix.setTargetValue(mix);
  smoothedHpfFreq.setTargetValue(hpfFreq);

  // Update HPF ( Butterworth 0.707 resonance fixed)
  *filterChain.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), smoothedHpfFreq.getNextValue(), 0.707f);

  double bpm = 120.0;
  if (auto* playHead = getPlayHead()) {
    if (auto pos = playHead->getPosition()) {
        if (auto bpmOpt = pos->getBpm())
            bpm = *bpmOpt;
    }
  }

  double samplesPerBeat = (getSampleRate() * 60.0) / bpm;
  int spawnInterval = (int)(samplesPerBeat / (density > 0.01f ? density : 0.01f));
  
  int attackSamples = (int)(getSampleRate() * (attackMs / 1000.0f));
  int decaySamples = (int)(getSampleRate() * (decayMs / 1000.0f));

  std::uniform_real_distribution<float> rand01(0.0f, 1.0f);
  
  // Rhythmic divisions relative to a beat (1.0 = 1/4 note)
  static const std::vector<double> divisions = { 
      0.25, 0.333, 0.5, 0.666, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0 
  };

  float inLevel = 0.0f;
  float outLevel = 0.0f;

  // Optimize: Pre-allocate grain block for the whole block
  juce::AudioBuffer<float> grainBlock(totalNumOutputChannels, buffer.getNumSamples());
  grainBlock.clear();

  // Process grains in parallel/block instead of per-sample for efficiency
  for (auto &grain : grains) {
    if (grain.active || grain.waitingToStart) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            grain.process(circularBuffer, grainBlock, i, circularBuffer.getNumSamples(), getSampleRate());
        }
    }
  }

  for (int i = 0; i < buffer.getNumSamples(); ++i) {
    float chordSample = 0.0f;
    if (inputSource == 1) { // CHORD
        float sr = (float)getSampleRate();
        for (int h = 0; h < 6; ++h) {
            float currentFreq = smoothedChordFreqs[(size_t)h].getNextValue();
            chordSample += std::sin(currentSinePhases[(size_t)h]) * 0.15f;
            currentSinePhases[(size_t)h] += (2.0f * juce::MathConstants<float>::pi * currentFreq) / sr;
            if (currentSinePhases[(size_t)h] > 2.0f * juce::MathConstants<float>::pi)
                currentSinePhases[(size_t)h] -= 2.0f * juce::MathConstants<float>::pi;
        }
    }

    // Capture input and track input level
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
      float s = (inputSource == 1) ? chordSample : buffer.getSample(channel, i);
      if (inputSource == 1 && channel < totalNumOutputChannels) 
          buffer.setSample(channel, i, s); // Populate buffer for dry/wet mix

      circularBuffer.setSample(channel, writePosition, s);
      inLevel = std::max(inLevel, std::abs(s));
    }

    // Spawn grains
    samplesSinceLastGrain++;
    if (samplesSinceLastGrain >= spawnInterval && spawnInterval > 0) {
      samplesSinceLastGrain = 0;

      // Find inactive grain
      for (auto &grain : grains) {
        if (!grain.active && !grain.waitingToStart) {
          // Select Random Duration (Life) within range
          if (lifeMin > lifeMax) std::swap(lifeMin, lifeMax); // Safety
          std::uniform_real_distribution<float> lifeDist(lifeMin, lifeMax);
          grain.duration = (int)(samplesPerBeat * lifeDist(randomEngine));

          grain.attackSamples = attackSamples;
          grain.decaySamples = decaySamples;
          grain.isReversed = rand01(randomEngine) < revProb;
          
          if (loopCycleMaxBeats > 0.01f) {
              std::vector<double> validLoopDivs;
              for (auto d : divisions) if (d <= loopCycleMaxBeats) validLoopDivs.push_back(d);
              double loopDiv = validLoopDivs.empty() ? loopCycleMaxBeats : validLoopDivs[(size_t)std::uniform_int_distribution<int>(0, (int)validLoopDivs.size()-1)(randomEngine)];
              
              grain.isLooping = true;
              grain.loopDuration = (int)(samplesPerBeat * loopDiv);
              if (grain.loopDuration > grain.duration)
                  grain.loopDuration = grain.duration;
          } else {
              grain.isLooping = false;
              grain.loopDuration = 0;
          }

          // Random position in the past (up to 1.5 seconds)
          // ANTI-GLITCH: Added safety offset (512 samples) to avoid reading what we are currently writing
          std::uniform_int_distribution<int> posDist(
              512, (int)(getSampleRate() * 1.5));
          int offset = posDist(randomEngine);
          grain.startSample =
              (writePosition - offset + circularBuffer.getNumSamples()) %
              circularBuffer.getNumSamples();

          // Random pitch: -4 to +4 octaves (discrete)
          int pitchMin = (int)apvts.getRawParameterValue("PITCH_MIN")->load();
          int pitchMax = (int)apvts.getRawParameterValue("PITCH_MAX")->load();
          if (pitchMin > pitchMax) std::swap(pitchMin, pitchMax);
          std::uniform_int_distribution<int> pitchDist(pitchMin, pitchMax);
          grain.pitchRatio = std::pow(2.0f, (float)pitchDist(randomEngine));

          // Normalization logic: adjust for active grain count
          grain.amplitude = 1.0f / std::sqrt((float)maxGrains * 0.1f); 
          
          // Balanced Kinetic Panning
          float panSpeed = apvts.getRawParameterValue("PAN_SPEED")->load();
          grain.panStart = rand01(randomEngine); // Random starting position
          // Drift direction: 50% left-to-right, 50% right-to-left
          float driftDir = (rand01(randomEngine) > 0.5f) ? 1.0f : -1.0f;
          // Calculate drift per sample based on speed.
          // At max speed (1.0), it should travel across the whole stereo field (0 to 1) in 1 second.
          grain.panDrift = driftDir * (panSpeed / (float)getSampleRate());

          // Delay logic
          if (rand01(randomEngine) < delayProb && delayMaxBeats > 0.01f) {
              std::vector<double> validDivs;
              for (auto d : divisions) if (d <= delayMaxBeats) validDivs.push_back(d);
              if (!validDivs.empty()) {
                  std::uniform_int_distribution<int> dDist(0, (int)validDivs.size() - 1);
                  grain.delaySamples = (int)(samplesPerBeat * validDivs[(size_t)dDist(randomEngine)]);
                  grain.waitingToStart = true;
                  grain.active = false;
              } else {
                  grain.active = true;
                  grain.currentSample = 0;
              }
          } else {
              grain.active = true;
              grain.waitingToStart = false;
              grain.currentSample = 0;
          }
          // Per-Grain Filter Setup
          float filtProb = apvts.getRawParameterValue("GRAIN_FILTER_DEPTH")->load();
          if (rand01(randomEngine) < filtProb) {
              grain.filterActive = true;
              std::uniform_real_distribution<float> fDist(100.0f, 8000.0f);
              grain.filterStartFreq = fDist(randomEngine);
              grain.filterEndFreq = fDist(randomEngine);
              grain.filterRes = apvts.getRawParameterValue("GRAIN_FILTER_RES")->load();
          } else {
              grain.filterActive = false;
          }

          break;
        }
      }
    }

    // Mix and Gain with Smoothing
    float currentMix = smoothedMix.getNextValue();
    float currentGain = smoothedGain.getNextValue();

    float dry = 1.0f - currentMix;
    float wet = currentMix;

    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
        float drySample = buffer.getSample(channel, i) * dry;
        float wetSample = grainBlock.getSample(channel, i) * wet;
        float combined = (drySample + wetSample) * currentGain;

        // Soft saturation clipper to prevent harsh digital clipping glitches
        if (combined > 1.0f) combined = 1.0f - std::exp(-combined);
        else if (combined < -1.0f) combined = -1.0f + std::exp(combined);

        buffer.setSample(channel, i, combined);
        if (channel == 0) outLevel = std::max(outLevel, std::abs(combined));
    }

    writePosition++;
    if (writePosition >= circularBuffer.getNumSamples())
      writePosition = 0;
  }

  // Apply Effects
  juce::dsp::AudioBlock<float> block(buffer);
  juce::dsp::ProcessContextReplacing<float> context(block);

  filterChain.process(context);

  // Randomize Reverb Room slightly over time for psychedelic feel
  if (rand01(randomEngine) < 0.05f) { // 5% chance per block to change decay
      std::uniform_real_distribution<float> decayDist(0.4f, 0.95f);
      smoothedReverbRoom.setTargetValue(decayDist(randomEngine));
  }
  
  // Apply smoothed Reverb params
  juce::Reverb::Parameters revParams = reverb.getParameters();
  revParams.roomSize = smoothedReverbRoom.getNextValue();
  revParams.damping = 0.2f;
  revParams.wetLevel = 0.3f;
  revParams.dryLevel = 1.0f;
  reverb.setParameters(revParams);
  reverb.process(context);

  // Randomize Phaser parameters
  if (rand01(randomEngine) < 0.1f) {
      std::uniform_real_distribution<float> freqDist(400.0f, 3000.0f);
      smoothedPhaserFreq.setTargetValue(freqDist(randomEngine));
      smoothedPhaserFeedback.setTargetValue(rand01(randomEngine) * 0.7f);
  }
  
  phaser.setCentreFrequency(smoothedPhaserFreq.getNextValue());
  phaser.setFeedback(smoothedPhaserFeedback.getNextValue());
  phaser.process(context);

  // Smoothly update levels
  inputLevel = inputLevel * 0.9f + inLevel * 0.1f;
  outputLevel = outputLevel * 0.9f + outLevel * 0.1f;
}

juce::AudioProcessorValueTreeState::ParameterLayout
CrystalVstAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  // DENSITY: Higher = More grains (grains per beat). Skew 0.5 for more resolution at low/mid density
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DENSITY", "Density (grains/beat)", juce::NormalisableRange<float>(0.25f, 16.0f, 0.01f, 0.5f), 2.0f));
  // LIFE_MIN/MAX: Min and Max grain duration in beats
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "LIFE_MIN", "Min Life (beats)", 0.0625f, 2.0f, 0.25f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "LIFE_MAX", "Max Life (beats)", 0.0625f, 2.0f, 1.0f));

  // PITCH_MIN/MAX: Discrete Range in octaves (-4 to +4)
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "PITCH_MIN", "Min Pitch (oct)", -4, 4, 0));
  params.push_back(std::make_unique<juce::AudioParameterInt>(
      "PITCH_MAX", "Max Pitch (oct)", -4, 4, 0));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "MIX", "Mix", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "GAIN", "Gain", 0.0f, 4.0f, 1.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "REVERSE_PROB", "Reverse Prob", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "ATTACK", "Envelope Attack (ms)", 0.0f, 100.0f, 10.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DECAY", "Envelope Decay (ms)", 0.0f, 100.0f, 10.0f));
  // LOOP_BEATS: Max loop cycle in beats (picked randomly within)
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "LOOP_BEATS", "Max Loop (beats)", 0.0f, 8.0f, 0.25f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DELAY_PROB", "Delay Prob", 0.0f, 1.0f, 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DELAY_MAX", "Max Delay (beats)", 0.0f, 2.0f, 0.5f));
  
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "INPUT_SOURCE", "Input Source", juce::StringArray{"Live", "Chord"}, 0));

  // HPF_FREQ: Skew 0.3 for better precision in low frequencies
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "HPF_FREQ", "HPF Freq", juce::NormalisableRange<float>(20.0f, 5000.0f, 1.0f, 0.3f), 20.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "GRAIN_FILTER_DEPTH", "Grn Filt Prob", 0.0f, 1.0f, 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "GRAIN_FILTER_RES", "Grn Filt Res", 0.1f, 5.0f, 1.0f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "PAN_SPEED", "Pan Speed", 0.0f, 1.0f, 0.0f));

  return {params.begin(), params.end()};
}

void CrystalVstAudioProcessor::generateRandomChord() {
    std::uniform_real_distribution<float> freqDist(100.0f, 400.0f);
    for (int i = 0; i < 6; ++i) {
        float newFreq = freqDist(randomEngine) * (1.0f + (float)i * 0.5f);
        chordFrequencies[(size_t)i] = newFreq;
        smoothedChordFreqs[(size_t)i].setTargetValue(newFreq);
    }
}

void CrystalVstAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void CrystalVstAudioProcessor::setStateInformation(const void *data,
                                                   int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));
  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(apvts.state.getType()))
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor *CrystalVstAudioProcessor::createEditor() {
  return new CrystalVstAudioProcessorEditor(*this);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new CrystalVstAudioProcessor();
}
