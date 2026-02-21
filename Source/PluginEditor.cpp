#include "PluginEditor.h"
#include "PluginProcessor.h"

CrystalVstAudioProcessorEditor::CrystalVstAudioProcessorEditor(
    CrystalVstAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  
  addAndMakeVisible(inputMeter);
  addAndMakeVisible(outputMeter);

  setupSlider(densitySlider, densityLabel, "DENSITY");
  setupSlider(pitchMinSlider, pitchMinLabel, "PITCH MIN");
  setupSlider(pitchMaxSlider, pitchMaxLabel, "PITCH MAX");
  setupSlider(lifeMinSlider, lifeMinLabel, "LIFE MIN");
  setupSlider(lifeMaxSlider, lifeMaxLabel, "LIFE MAX");
  setupSlider(mixSlider, mixLabel, "MIX");
  setupSlider(gainSlider, gainLabel, "GAIN");
  setupSlider(revSlider, revLabel, "REVERSE");
  setupSlider(attackSlider, attackLabel, "ATTACK");
  setupSlider(decaySlider, decayLabel, "DECAY");
  setupSlider(loopCycleSlider, loopCycleLabel, "LOOP CYCLE");
  setupSlider(delayProbSlider, delayProbLabel, "DELAY %");
  setupSlider(delayMaxSlider, delayMaxLabel, "DELAY MAX");
  setupSlider(hpfFreqSlider, hpfFreqLabel, "HPF FREQ");
  setupSlider(grnFiltSlider, grnFiltLabel, "GRN FILT");
  setupSlider(grnResSlider, grnResLabel, "GRN RES");

  densityAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "DENSITY", densitySlider);
  pitchMinAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "PITCH_MIN", pitchMinSlider);
  pitchMaxAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "PITCH_MAX", pitchMaxSlider);
  lifeMinAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "LIFE_MIN", lifeMinSlider);
  lifeMaxAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "LIFE_MAX", lifeMaxSlider);
  mixAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "MIX", mixSlider);
  gainAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "GAIN", gainSlider);
  revAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "REVERSE_PROB", revSlider);
  attackAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "ATTACK", attackSlider);
  decayAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "DECAY", decaySlider);
  loopCycleAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "LOOP_BEATS", loopCycleSlider);
  delayProbAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "DELAY_PROB", delayProbSlider);
  delayMaxAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "DELAY_MAX", delayMaxSlider);
  hpfFreqAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "HPF_FREQ", hpfFreqSlider);
  grnFiltAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "GRAIN_FILTER_DEPTH", grnFiltSlider);
  grnResAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "GRAIN_FILTER_RES", grnResSlider);

  sourceSelector.addItem("LIVE INPUT", 1);
  sourceSelector.addItem("PSYCH CHORD", 2);
  sourceSelector.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(sourceSelector);
  
  sourceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
      audioProcessor.apvts, "INPUT_SOURCE", sourceSelector);

  sourceLabel.setText("INPUT SOURCE", juce::dontSendNotification);
  sourceLabel.setJustificationType(juce::Justification::centred);
  sourceLabel.setColour(juce::Label::textColourId, juce::Colours::magenta);
  addAndMakeVisible(sourceLabel);

  setSize(900, 600);
  startTimerHz(30);
}

CrystalVstAudioProcessorEditor::~CrystalVstAudioProcessorEditor() {
    stopTimer();
}

void CrystalVstAudioProcessorEditor::timerCallback() {
    inputMeter.setLevel(audioProcessor.getInputLevel());
    outputMeter.setLevel(audioProcessor.getOutputLevel());
    inputMeter.repaint();
    outputMeter.repaint();
}

void CrystalVstAudioProcessorEditor::setupSlider(juce::Slider &slider,
                                                 juce::Label &label,
                                                 const juce::String &name) {
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  
  // Custom Crystal Style
  slider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
  slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::magenta.withAlpha(0.6f));
  slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha(0.1f));
  
  addAndMakeVisible(slider);

  label.setText(name, juce::dontSendNotification);
  label.setJustificationType(juce::Justification::centred);
  label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  label.setColour(juce::Label::textColourId, juce::Colours::cyan);
  addAndMakeVisible(label);
}

void CrystalVstAudioProcessorEditor::paint(juce::Graphics &g) {
  // Strange Background: gradient mesh feel
  juce::ColourGradient grad(juce::Colour(0xFF0F0F1F), 0, 0,
                             juce::Colour(0xFF1A0A2A), getWidth(), getHeight(), true);
  g.setGradientFill(grad);
  g.fillAll();

  // Decorative "strange" crystals/shapes
  g.setColour(juce::Colours::cyan.withAlpha(0.05f));
  juce::Path p;
  p.addEllipse(getWidth() * 0.2f, getHeight() * 0.3f, 400, 400);
  p.addEllipse(getWidth() * 0.6f, getHeight() * 0.1f, 300, 300);
  g.fillPath(p);

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(32.0f, juce::Font::bold));
  g.drawText("CRYSTAL GRANULAR", 20, 20, 400, 40, juce::Justification::left);
  
  g.setColour(juce::Colours::magenta.withAlpha(0.5f));
  g.setFont(juce::FontOptions(14.0f));
  g.drawText("v14 KINETIC RANGES", 20, 55, 400, 20, juce::Justification::left);
}

void CrystalVstAudioProcessorEditor::resized() {
  
  // Input Source Selector at the top center
  sourceSelector.setBounds(getWidth() / 2 - 80, 70, 160, 24);
  sourceLabel.setBounds(getWidth() / 2 - 80, 45, 160, 20);

  // Meters on the sides
  inputMeter.setBounds(10, 100, 15, 400);
  outputMeter.setBounds(getWidth() - 25, 100, 15, 400);

  // Clustered Layout
  int cw = 110;
  int ch = 120;

  // --- CLUSTER 1: CORE (Center Left) ---
  int coreX = 60;
  int coreY = 180;
  densitySlider.setBounds(coreX, coreY, cw, ch);
  densityLabel.setBounds(densitySlider.getBounds().translated(0, ch - 20).withHeight(20));

  pitchMinSlider.setBounds(coreX + cw + 10, coreY, cw, ch);
  pitchMinLabel.setBounds(pitchMinSlider.getBounds().translated(0, ch - 20).withHeight(20));

  pitchMaxSlider.setBounds(coreX + (cw + 10) * 2, coreY, cw, ch);
  pitchMaxLabel.setBounds(pitchMaxSlider.getBounds().translated(0, ch - 20).withHeight(20));

  mixSlider.setBounds(coreX, coreY + ch + 10, cw, ch);
  mixLabel.setBounds(mixSlider.getBounds().translated(0, ch - 20).withHeight(20));

  lifeMinSlider.setBounds(coreX + cw + 10, coreY + ch + 10, cw, ch);
  lifeMinLabel.setBounds(lifeMinSlider.getBounds().translated(0, ch - 20).withHeight(20));

  lifeMaxSlider.setBounds(coreX + (cw + 10) * 2, coreY + ch + 10, cw, ch);
  lifeMaxLabel.setBounds(lifeMaxSlider.getBounds().translated(0, ch - 20).withHeight(20));

  // --- CLUSTER 2: MODULATION (Center Right) ---
  int modX = 500; // Shifted right to accommodate wider UI
  int modY = 180;
  revSlider.setBounds(modX, modY, cw, ch);
  revLabel.setBounds(revSlider.getBounds().translated(0, ch - 20).withHeight(20));

  loopCycleSlider.setBounds(modX + cw + 10, modY, cw, ch);
  loopCycleLabel.setBounds(loopCycleSlider.getBounds().translated(0, ch - 20).withHeight(20));

  grnFiltSlider.setBounds(modX + (cw + 10) * 2, modY, cw, ch);
  grnFiltLabel.setBounds(grnFiltSlider.getBounds().translated(0, ch - 20).withHeight(20));

  delayProbSlider.setBounds(modX, modY + ch + 10, cw, ch);
  delayProbLabel.setBounds(delayProbSlider.getBounds().translated(0, ch - 20).withHeight(20));

  delayMaxSlider.setBounds(modX + cw + 10, modY + ch + 10, cw, ch);
  delayMaxLabel.setBounds(delayMaxSlider.getBounds().translated(0, ch - 20).withHeight(20));

  grnResSlider.setBounds(modX + (cw + 10) * 2, modY + ch + 10, cw, ch);
  grnResLabel.setBounds(grnResSlider.getBounds().translated(0, ch - 20).withHeight(20));

  // --- CLUSTER 3: SPACE / ENVELOPE (Bottom Center) ---
  int spaceX = getWidth() / 2 - (cw * 3) / 2;
  int spaceY = 460;
  attackSlider.setBounds(spaceX, spaceY, cw, ch);
  attackLabel.setBounds(attackSlider.getBounds().translated(0, ch - 20).withHeight(20));

  decaySlider.setBounds(spaceX + cw + 10, spaceY, cw, ch);
  decayLabel.setBounds(decaySlider.getBounds().translated(0, ch - 20).withHeight(20));

  hpfFreqSlider.setBounds(spaceX + (cw + 10) * 2, spaceY, cw, ch);
  hpfFreqLabel.setBounds(hpfFreqSlider.getBounds().translated(0, ch - 20).withHeight(20));
}
