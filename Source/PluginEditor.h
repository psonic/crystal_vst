#pragma once

#include "PluginProcessor.h"

class LevelMeter : public juce::Component {
public:
  void setLevel(float newLevel) { level = newLevel; }
  void paint(juce::Graphics &g) override {
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds, 4.0f);

    float h = bounds.getHeight() * level;
    auto r = bounds.withTop(bounds.getBottom() - h);
    
    juce::ColourGradient grad(juce::Colours::cyan, bounds.getX(), bounds.getBottom(),
                               juce::Colours::magenta, bounds.getX(), bounds.getY(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, 4.0f);
    
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
  }
private:
  float level = 0.0f;
};

class CrystalVstAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer {
public:
  CrystalVstAudioProcessorEditor(CrystalVstAudioProcessor &);
  ~CrystalVstAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

private:
  CrystalVstAudioProcessor &audioProcessor;

  LevelMeter inputMeter;
  LevelMeter outputMeter;

  juce::Slider densitySlider;
  juce::Slider pitchMinSlider;
  juce::Slider pitchMaxSlider;
  juce::Slider lifeMinSlider;
  juce::Slider lifeMaxSlider;
  juce::Slider mixSlider;
  juce::Slider gainSlider;
  juce::Slider revSlider;
  juce::Slider attackSlider;
  juce::Slider decaySlider;
  juce::Slider loopCycleSlider;
  juce::Slider delayProbSlider;
  juce::Slider delayMaxSlider;
  juce::Slider hpfFreqSlider;
  juce::Slider grnFiltSlider;
  juce::Slider grnResSlider;
  juce::Slider panSpeedSlider;
  juce::ComboBox sourceSelector;

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      densityAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      pitchMinAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      pitchMaxAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      lifeMinAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      lifeMaxAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      mixAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      gainAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      revAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attackAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      decayAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      loopCycleAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      delayProbAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      delayMaxAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      hpfFreqAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      grnFiltAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      grnResAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panSpeedAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      sourceAttachment;

  juce::Label densityLabel;
  juce::Label pitchMinLabel;
  juce::Label pitchMaxLabel;
  juce::Label lifeMinLabel;
  juce::Label lifeMaxLabel;
  juce::Label mixLabel;
  juce::Label gainLabel;
  juce::Label revLabel;
  juce::Label attackLabel;
  juce::Label decayLabel;
  juce::Label loopCycleLabel;
  juce::Label delayProbLabel;
  juce::Label delayMaxLabel;
  juce::Label hpfFreqLabel;
  juce::Label grnFiltLabel;
  juce::Label grnResLabel;
  juce::Label panSpeedLabel;
  juce::Label sourceLabel;

  void setupSlider(juce::Slider &slider, juce::Label &label,
                   const juce::String &name, const juce::String &paramId);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrystalVstAudioProcessorEditor)
};
