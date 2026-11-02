#include "PluginEditor.h"

VocalTuneAudioProcessorEditor::VocalTuneAudioProcessorEditor (VocalTuneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (900, 560);

    titleLabel.setText ("VocalTune", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions ( 30.0f )));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff4f5fb));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("VOCAL PITCH EDITOR  •  STAGE 1 BUILD", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9299ad));
    addAndMakeVisible (subtitleLabel);

    auto setupSlider = [this] (juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, juce::Colour (0xffcbd0df));
        label.setFont (juce::Font (juce::FontOptions (14.0f)));
        addAndMakeVisible (label);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 22);
        slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff8a70ff));
        slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff4f5fb));
        slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xfff4f5fb));
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        addAndMakeVisible (slider);
    };

    setupSlider (correctionSlider, correctionLabel, "Correction Amount");
    setupSlider (speedSlider, speedLabel, "Retune Speed");
    setupSlider (transitionSlider, transitionLabel, "Transition");

    scaleLabel.setText ("KEY / SCALE", juce::dontSendNotification);
    scaleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffcbd0df));
    addAndMakeVisible (scaleLabel);
    scaleBox.addItem ("Chromatic", 1);
    scaleBox.addItem ("Major", 2);
    scaleBox.addItem ("Natural Minor", 3);
    addAndMakeVisible (scaleBox);

    correctionAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "correction", correctionSlider);
    speedAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "speed", speedSlider);
    transitionAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "transition", transitionSlider);
    scaleAttachment = std::make_unique<ComboAttachment> (audioProcessor.parameters, "scale", scaleBox);
}

void VocalTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff11131b));

    auto area = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1e2a));
    g.fillRoundedRectangle (area.reduced (22.0f).withTrimmedTop (105.0f), 16.0f);

    g.setColour (juce::Colour (0xff282d3d));
    g.drawRoundedRectangle (area.reduced (22.0f).withTrimmedTop (105.0f), 16.0f, 1.0f);

    auto panel = getLocalBounds().reduced (42);
    g.setColour (juce::Colour (0xff8a70ff));
    g.fillRoundedRectangle (panel.removeFromTop (3).toFloat(), 2.0f);

    g.setColour (juce::Colour (0xff72798e));
    g.setFont (13.0f);
    g.drawFittedText ("PITCH DISPLAY / NOTE EDITOR", 48, 150, getWidth() - 96, 24,
                      juce::Justification::left, 1);
    g.setColour (juce::Colour (0xff282d3d));
    g.drawRoundedRectangle (48.0f, 180.0f, (float) getWidth() - 96.0f, 190.0f, 10.0f, 1.0f);
    g.setColour (juce::Colour (0xff626a80));
    g.drawFittedText ("Audio analysis and editable note blocks are coming in Stage 2.",
                      70, 250, getWidth() - 140, 50, juce::Justification::centred, 2);

    g.setColour (juce::Colour (0xff72798e));
    g.drawFittedText ("PROCESSING CONTROLS", 48, 390, getWidth() - 96, 20,
                      juce::Justification::left, 1);
}

void VocalTuneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    titleLabel.setBounds (42, 26, 360, 42);
    subtitleLabel.setBounds (44, 70, 420, 22);

    const int yLabel = 420;
    const int ySlider = 438;
    const int sliderW = 115;
    correctionLabel.setBounds (48, yLabel, 160, 22);
    correctionSlider.setBounds (60, ySlider, sliderW, 100);
    speedLabel.setBounds (240, yLabel, 140, 22);
    speedSlider.setBounds (250, ySlider, sliderW, 100);
    transitionLabel.setBounds (430, yLabel, 140, 22);
    transitionSlider.setBounds (440, ySlider, sliderW, 100);
    scaleLabel.setBounds (640, yLabel, 150, 22);
    scaleBox.setBounds (640, ySlider + 25, 180, 30);
}
