
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VocalTuneAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    explicit VocalTuneAudioProcessorEditor(
        VocalTuneAudioProcessor&);

    ~VocalTuneAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VocalTuneAudioProcessor& audioProcessor;

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail {
        512, formatManager, thumbnailCache
    };

    juce::TextButton importButton { "IMPORT AUDIO" };
    juce::TextButton analyseButton { "ANALYSE" };

    juce::Label titleLabel;
    juce::Label fileLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void importAudio();
    void drawPianoRoll(juce::Graphics&, juce::Rectangle<int>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        VocalTuneAudioProcessorEditor)
};
