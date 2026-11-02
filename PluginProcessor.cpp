#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalTuneAudioProcessor::VocalTuneAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void VocalTuneAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void VocalTuneAudioProcessor::releaseResources()
{
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

    // This first editor stage does not process or tune live audio.
    // Leave the incoming audio unchanged.
    juce::ignoreUnused(buffer);
}

juce::AudioProcessorEditor*
VocalTuneAudioProcessor::createEditor()
{
    return new VocalTuneAudioProcessorEditor(*this);
}

void VocalTuneAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    destData.reset();
}

void VocalTuneAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalTuneAudioProcessor();
}