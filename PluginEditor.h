
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VocalTuneAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit VocalTuneAudioProcessorEditor(VocalTuneAudioProcessor&);
    ~VocalTuneAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VocalTuneAudioProcessor& audioProcessor;

    void timerCallback() override;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::Label pitchLabel;
    juce::Label frequencyLabel;
    juce::Label statusLabel;

    juce::Label correctionLabel;
    juce::Label speedLabel;
    juce::Label transitionLabel;
    juce::Label scaleLabel;

    juce::Slider correctionSlider;
    juce::Slider speedSlider;
    juce::Slider transitionSlider;

    juce::ComboBox scaleBox;

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    using ComboAttachment =
        juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> correctionAttachment;
    std::unique_ptr<SliderAttachment> speedAttachment;
    std::unique_ptr<SliderAttachment> transitionAttachment;

    std::unique_ptr<ComboAttachment> scaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
        (VocalTuneAudioProcessorEditor)
};
