
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

static juce::AudioProcessorValueTreeState::ParameterLayout
createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "correction", "Correction",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "speed", "Retune Speed",
        juce::NormalisableRange<float>(1.0f, 500.0f, 1.0f, 0.35f),
        40.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "transition", "Transition",
        juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f),
        80.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "scale", "Scale",
        juce::StringArray {
            "Chromatic", "Major", "Natural Minor"
        },
        0));

    return { params.begin(), params.end() };
}

VocalTuneAudioProcessor::VocalTuneAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr,
                 "VocalTuneParameters", createParameterLayout())
{
}

void VocalTuneAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    maximumBlockSize = juce::jmax(512, samplesPerBlock);

    const int channels = getTotalNumInputChannels();

    inputScratch.setSize(channels, maximumBlockSize);
    outputScratch.setSize(channels, maximumBlockSize);

    inputScratch.clear();
    outputScratch.clear();

    pitchBuffer.fill(0.0f);

    pitchWritePosition = 0;
    samplesSinceAnalysis = 0;

    detectedFrequency.store(0.0f);
    detectedMidiNote.store(-1);
    pitchConfidence.store(0.0f);

    currentPitchShiftSemitones = 0.0f;

    // Configure the pitch-shifting engine.
    pitchShifter.presetDefault(
        channels,
        static_cast<float>(sampleRate),
        true);

    setLatencySamples(
        pitchShifter.inputLatency()
        + pitchShifter.outputLatency());
}

void VocalTuneAudioProcessor::releaseResources()
{
    pitchShifter.reset();
}

bool VocalTuneAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();

    return input == output
        && (output == juce::AudioChannelSet::mono()
            || output == juce::AudioChannelSet::stereo());
}

void VocalTuneAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    midiMessages.clear();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels == 0)
        return;

    // Analyse the original incoming vocal.
    const float* left = buffer.getReadPointer(0);

    const float* right = numChannels > 1
        ? buffer.getReadPointer(1)
        : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = left[i];

        if (right != nullptr)
            sample = 0.5f * (sample + right[i]);

        pitchBuffer[static_cast<size_t>(pitchWritePosition)] = sample;

        pitchWritePosition =
            (pitchWritePosition + 1) % analysisBufferSize;

        ++samplesSinceAnalysis;

        if (samplesSinceAnalysis >= 512)
        {
            samplesSinceAnalysis = 0;
            analysePitchBuffer();
        }
    }

    // Calculate the target pitch correction.
    const float targetShift = calculateTargetPitchShift();

    const float speedMs =
        parameters.getRawParameterValue("speed")->load();

    const float transitionMs =
        parameters.getRawParameterValue("transition")->load();

    const float blockDuration =
        static_cast<float>(numSamples / currentSampleRate);

    const float smoothingTime =
        juce::jmax(0.001f, speedMs * 0.001f);

    const float smoothing =
        1.0f - std::exp(-blockDuration / smoothingTime);

    currentPitchShiftSemitones +=
        (targetShift - currentPitchShiftSemitones) * smoothing;

    // Limit the rate at which notes glide between targets.
    if (transitionMs > 0.0f)
    {
        const float maximumChange =
            12.0f * blockDuration / (transitionMs * 0.001f);

        const float difference =
            targetShift - currentPitchShiftSemitones;

        currentPitchShiftSemitones +=
            juce::jlimit(
                -maximumChange,
                maximumChange,
                difference);
    }

    pitchShifter.setTransposeSemitones(
        currentPitchShiftSemitones);

    // Process in chunks to avoid exceeding prepared buffer sizes.
    int position = 0;

    while (position < numSamples)
    {
        const int chunkSize = juce::jmin(
            maximumBlockSize,
            numSamples - position);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            inputScratch.copyFrom(
                channel, 0,
                buffer, channel, position, chunkSize);
        }

        pitchShifter.process(
            inputScratch.getArrayOfReadPointers(),
            chunkSize,
            outputScratch.getArrayOfWritePointers(),
            chunkSize);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.copyFrom(
                channel, position,
                outputScratch, channel, 0, chunkSize);
        }

        position += chunkSize;
    }
}

float VocalTuneAudioProcessor::calculateTargetPitchShift() const noexcept
{
    const float frequency = detectedFrequency.load();

    if (frequency <= 0.0f)
        return 0.0f;

    const float midi =
        69.0f + 12.0f * std::log2(frequency / 440.0f);

    const int scaleIndex = juce::roundToInt(
        parameters.getRawParameterValue("scale")->load());

    // Chromatic: every semitone is allowed.
    if (scaleIndex == 0)
        return (std::round(midi) - midi)
            * parameters.getRawParameterValue("correction")->load()
            / 100.0f;

    // Major scale: C major.
    constexpr int majorNotes[] = {
        0, 2, 4, 5, 7, 9, 11
    };

    // Natural minor scale: A minor.
    constexpr int minorNotes[] = {
        9, 11, 0, 2, 4, 5, 7
    };

    const int* allowedNotes =
        scaleIndex == 1 ? majorNotes : minorNotes;

    float bestTarget = std::round(midi);
    float smallestDistance = 1000.0f;

    const int baseNote = static_cast<int>(std::floor(midi));

    for (int note = baseNote - 12;
         note <= baseNote + 12;
         ++note)
    {
        const int pitchClass = (note % 12 + 12) % 12;

        bool allowed = false;

        for (int i = 0; i < 7; ++i)
        {
            if (pitchClass == allowedNotes[i])
            {
                allowed = true;
                break;
            }
        }

        if (allowed)
        {
            const float distance =
                std::abs(static_cast<float>(note) - midi);

            if (distance < smallestDistance)
            {
                smallestDistance = distance;
                bestTarget = static_cast<float>(note);
            }
        }
    }

    const float correction =
        parameters.getRawParameterValue("correction")->load()
        / 100.0f;

    return (bestTarget - midi) * correction;
}

void VocalTuneAudioProcessor::analysePitchBuffer() noexcept
{
    const float frequency = estimatePitch();

    if (frequency <= 0.0f)
    {
        pitchConfidence.store(0.0f);
        detectedFrequency.store(0.0f);
        detectedMidiNote.store(-1);
        return;
    }

    const float midi =
        69.0f + 12.0f * std::log2(frequency / 440.0f);

    detectedFrequency.store(frequency);
    detectedMidiNote.store(juce::roundToInt(midi));
    pitchConfidence.store(1.0f);
}

float VocalTuneAudioProcessor::estimatePitch() noexcept
{
    constexpr int minPeriod = 40;
    constexpr int maxPeriod = 1024;

    std::array<float, analysisBufferSize> samples {};

    float mean = 0.0f;

    for (int i = 0; i < analysisBufferSize; ++i)
    {
        const int index =
            (pitchWritePosition + i) % analysisBufferSize;

        samples[static_cast<size_t>(i)] =
            pitchBuffer[static_cast<size_t>(index)];

        mean += samples[static_cast<size_t>(i)];
    }

    mean /= static_cast<float>(analysisBufferSize);

    float energy = 0.0f;

    for (auto& sample : samples)
    {
        sample -= mean;
        energy += sample * sample;
    }

    if (energy < 0.00001f)
        return 0.0f;

    float bestCorrelation = 0.0f;
    int bestPeriod = 0;

    for (int period = minPeriod;
         period <= maxPeriod;
         ++period)
    {
        float correlation = 0.0f;
        float energyA = 0.0f;
        float energyB = 0.0f;

        for (int i = 0;
             i < analysisBufferSize - period;
             ++i)
        {
            const float a = samples[static_cast<size_t>(i)];
            const float b =
                samples[static_cast<size_t>(i + period)];

            correlation += a * b;
            energyA += a * a;
            energyB += b * b;
        }

        const float denominator =
            std::sqrt(energyA * energyB);

        if (denominator > 0.0f)
            correlation /= denominator;

        if (correlation > bestCorrelation)
        {
            bestCorrelation = correlation;
            bestPeriod = period;
        }
    }

    if (bestPeriod == 0 || bestCorrelation < 0.80f)
        return 0.0f;

    return static_cast<float>(currentSampleRate)
        / static_cast<float>(bestPeriod);
}

juce::AudioProcessorEditor*
VocalTuneAudioProcessor::createEditor()
{
    return new VocalTuneAudioProcessorEditor(*this);
}

void VocalTuneAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    copyXmlToBinary(*xml, destData);
}

void VocalTuneAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr
        && xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(
            juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalTuneAudioProcessor();
}
