#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

class VocalTuneAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer,
      private juce::FileDragAndDropTarget
{
public:
    explicit VocalTuneAudioProcessorEditor(
        VocalTuneAudioProcessor&);

    ~VocalTuneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(
        const juce::StringArray& files) override;

    void filesDropped(
        const juce::StringArray& files,
        int x,
        int y) override;

private:
    struct DetectedNote
    {
        int midiNote = -1;
        double startSeconds = 0.0;
        double endSeconds = 0.0;
        float confidence = 0.0f;
    };

    VocalTuneAudioProcessor& audioProcessor;

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> audioData;

    double audioSampleRate = 44100.0;
    juce::File loadedFile;

    juce::TextButton importButton { "IMPORT VOCAL" };
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label statusLabel;

    std::vector<DetectedNote> detectedNotes;
    std::mutex notesMutex;

    std::thread analysisThread;
    std::atomic<bool> analysisComplete { true };

    std::unique_ptr<juce::FileChooser> fileChooser;

    void timerCallback() override;

    void openFileChooser();
    void loadAudioFile(const juce::File& file);
    void startPitchAnalysis(std::vector<float> samples,
                            double sampleRate);

    static std::vector<DetectedNote> analysePitch(
        const std::vector<float>& samples,
        double sampleRate);

    void drawWaveform(juce::Graphics& g);
    void drawPianoRoll(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        VocalTuneAudioProcessorEditor)
};