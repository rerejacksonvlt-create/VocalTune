
#include "PluginEditor.h"

VocalTuneAudioProcessorEditor::VocalTuneAudioProcessorEditor(
    VocalTuneAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
{
    setSize(900, 560);

    titleLabel.setText("VocalTune", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(30.0f)));
    titleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xfff4f5fb));
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "PROFESSIONAL VOCAL PITCH EDITOR",
        juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
    subtitleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xff9299ad));
    addAndMakeVisible(subtitleLabel);

    pitchLabel.setText("--", juce::dontSendNotification);
    pitchLabel.setFont(juce::Font(juce::FontOptions(54.0f)));
    pitchLabel.setJustificationType(juce::Justification::centred);
    pitchLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xfff4f5fb));
    addAndMakeVisible(pitchLabel);

    frequencyLabel.setText(
        "Waiting for vocal input",
        juce::dontSendNotification);
    frequencyLabel.setFont(juce::Font(juce::FontOptions(16.0f)));
    frequencyLabel.setJustificationType(
        juce::Justification::centred);
    frequencyLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xffa8aec0));
    addAndMakeVisible(frequencyLabel);

    statusLabel.setText(
        "PITCH ANALYSIS READY",
        juce::dontSendNotification);
    statusLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    statusLabel.setJustificationType(
        juce::Justification::centred);
    statusLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xff8a70ff));
    addAndMakeVisible(statusLabel);

    auto setupSlider = [this](
        juce::Slider& slider,
        juce::Label& label,
        const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setColour(
            juce::Label::textColourId,
            juce::Colour(0xffcbd0df));
        label.setFont(juce::Font(juce::FontOptions(14.0f)));
        addAndMakeVisible(label);

        slider.setSliderStyle(
            juce::Slider::RotaryHorizontalVerticalDrag);

        slider.setTextBoxStyle(
            juce::Slider::TextBoxBelow,
            false, 80, 22);

        slider.setColour(
            juce::Slider::rotarySliderFillColourId,
            juce::Colour(0xff8a70ff));

        slider.setColour(
            juce::Slider::thumbColourId,
            juce::Colour(0xfff4f5fb));

        slider.setColour(
            juce::Slider::textBoxTextColourId,
            juce::Colour(0xfff4f5fb));

        slider.setColour(
            juce::Slider::textBoxOutlineColourId,
            juce::Colour(0x00000000));

        addAndMakeVisible(slider);
    };

    setupSlider(
        correctionSlider,
        correctionLabel,
        "Correction Amount");

    setupSlider(
        speedSlider,
        speedLabel,
        "Retune Speed");

    setupSlider(
        transitionSlider,
        transitionLabel,
        "Transition");

    scaleLabel.setText(
        "KEY / SCALE",
        juce::dontSendNotification);

    scaleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xffcbd0df));

    addAndMakeVisible(scaleLabel);

    scaleBox.addItem("Chromatic", 1);
    scaleBox.addItem("Major", 2);
    scaleBox.addItem("Natural Minor", 3);

    addAndMakeVisible(scaleBox);

    correctionAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "correction",
            correctionSlider);

    speedAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "speed",
            speedSlider);

    transitionAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "transition",
            transitionSlider);

    scaleAttachment =
        std::make_unique<ComboAttachment>(
            audioProcessor.parameters,
            "scale",
            scaleBox);

    startTimerHz(15);
}

void VocalTuneAudioProcessorEditor::timerCallback()
{
    const int note = audioProcessor.getDetectedMidiNote();

    if (note < 0)
    {
        pitchLabel.setText(
            "--",
            juce::dontSendNotification);

        frequencyLabel.setText(
            "Waiting for vocal input",
            juce::dontSendNotification);

        statusLabel.setText(
            "NO PITCH DETECTED",
            juce::dontSendNotification);

        return;
    }

    const auto noteName =
        juce::MidiMessage::getMidiNoteName(
            note, true, true, 3);

    const float frequency =
        audioProcessor.getDetectedFrequency();

    pitchLabel.setText(
        noteName,
        juce::dontSendNotification);

    frequencyLabel.setText(
        juce::String(frequency, 1) + " Hz",
        juce::dontSendNotification);

    statusLabel.setText(
        "PITCH DETECTED",
        juce::dontSendNotification);
}

void VocalTuneAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff11131b));

    auto area = getLocalBounds().toFloat();

    g.setColour(juce::Colour(0xff1a1e2a));

    g.fillRoundedRectangle(
        area.reduced(22.0f).withTrimmedTop(105.0f),
        16.0f);

    g.setColour(juce::Colour(0xff282d3d));

    g.drawRoundedRectangle(
        area.reduced(22.0f).withTrimmedTop(105.0f),
        16.0f, 1.0f);

    g.setColour(juce::Colour(0xff8a70ff));

    g.fillRoundedRectangle(
        42.0f, 126.0f,
        (float)getWidth() - 84.0f,
        3.0f, 2.0f);

    g.setColour(juce::Colour(0xff72798e));

    g.setFont(13.0f);

    g.drawText(
        "LIVE PITCH DETECTION",
        48, 145,
        getWidth() - 96, 25,
        juce::Justification::centred);

    g.setColour(juce::Colour(0xff282d3d));

    g.drawRoundedRectangle(
        48.0f, 180.0f,
        (float)getWidth() - 96.0f,
        180.0f,
        12.0f, 1.0f);

    g.setColour(juce::Colour(0xff72798e));

    g.drawText(
        "PROCESSING CONTROLS",
        48, 382,
        getWidth() - 96, 22,
        juce::Justification::left);
}

void VocalTuneAudioProcessorEditor::resized()
{
    titleLabel.setBounds(42, 26, 360, 42);
    subtitleLabel.setBounds(44, 70, 420, 22);

    pitchLabel.setBounds(
        100, 205,
        getWidth() - 200, 70);

    frequencyLabel.setBounds(
        100, 275,
        getWidth() - 200, 30);

    statusLabel.setBounds(
        100, 320,
        getWidth() - 200, 22);

    const int yLabel = 420;
    const int ySlider = 438;
    const int sliderW = 115;

    correctionLabel.setBounds(48, yLabel, 160, 22);
    correctionSlider.setBounds(60, ySlider, sliderW, 100);

    speedLabel.setBounds(240, yLabel, 140, 22);
    speedSlider.setBounds(250, ySlider, sliderW, 100);

    transitionLabel.setBounds(430, yLabel, 140, 22);
    transitionSlider.setBounds(440, ySlider, sliderW, 100);

    scaleLabel.setBounds(640, yLabel, 150, 22);
    scaleBox.setBounds(640, ySlider + 25, 180, 30);
}
