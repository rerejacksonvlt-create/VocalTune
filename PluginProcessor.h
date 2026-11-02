
#pragma once

#include <JuceHeader.h>
#include "signalsmith-stretch.h"

#include <array>
#include <atomic>
#include <cmath>

class VocalTuneAudioProcessor final : public juce::AudioProcessor
{
public:
    VocalTuneAudioProcessor();
    ~VocalTuneAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(
        const BusesLayout& layouts) const override;

    void processBlock(
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
        return "VocalTune";
    }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override
    {
        return 0.0;
    }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    float getDetectedFrequency() const noexcept
    {
        return detectedFrequency.load();
    }

    int getDetectedMidiNote() const noexcept
    {
        return detectedMidiNote.load();
    }

    float getPitchConfidence() const noexcept
    {
        return pitchConfidence.load();
    }

private:
    static constexpr int analysisBufferSize = 2048;

    std::array<float, analysisBufferSize> pitchBuffer {};

    int pitchWritePosition = 0;
    int samplesSinceAnalysis = 0;

    double currentSampleRate = 44100.0;

    std::atomic<float> detectedFrequency { 0.0f };
    std::atomic<int> detectedMidiNote { -1 };
    std::atomic<float> pitchConfidence { 0.0f };

    // Pitch-shifting engine
    signalsmith::stretch::SignalsmithStretch<float> pitchShifter;

    // Separate buffers required by the pitch-shifting engine
    juce::AudioBuffer<float> inputScratch;
    juce::AudioBuffer<float> outputScratch;

    int maximumBlockSize = 512;

    // Smoothed pitch-shift amount
    float currentPitchShiftSemitones = 0.0f;

    void analysePitchBuffer() noexcept;
    float estimatePitch() noexcept;

    float calculateTargetPitchShift() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
        (VocalTuneAudioProcessor)
};
