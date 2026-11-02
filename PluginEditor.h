#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VocalTuneAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VocalTuneAudioProcessorEditor (VocalTuneAudioProcessor&);
    ~VocalTuneAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalTuneAudioProcessor& audioProcessor;

    juce::Label titleLabel, subtitleLabel, correctionLabel, speedLabel, transitionLabel, scaleLabel;
    juce::Slider correctionSlider, speedSlider, transitionSlider;
    juce::ComboBox scaleBox;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> correctionAttachment, speedAttachment, transitionAttachment;
    std::unique_ptr<ComboAttachment> scaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalTuneAudioProcessorEditor)
};
