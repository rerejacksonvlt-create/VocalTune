
#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalTuneAudioProcessor::VocalTuneAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void VocalTuneAudioProcessor::prepareToPlay(
    double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void VocalTuneAudioProcessor::releaseResources()
{
}

void VocalTuneAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // Initial version: clean audio pass-through.
    // Pitch processing will be added to this stage.
    juce::ScopedNoDenormals noDenormals;

    const int inputChannels =
        getTotalNumInputChannels();

    const int outputChannels =
        getTotalNumOutputChannels();

    for (int channel = inputChannels;
         channel < outputChannels; ++channel)
    {
        buffer.clear(channel, 0, buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor*
VocalTuneAudioProcessor::createEditor()
{
    return new VocalTuneAudioProcessorEditor(*this);
}

bool VocalTuneAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String VocalTuneAudioProcessor::getName() const
{
    return "VocalTune";
}

bool VocalTuneAudioProcessor::acceptsMidi() const
{
    return false;
}

bool VocalTuneAudioProcessor::producesMidi() const
{
    return false;
}

bool VocalTuneAudioProcessor::isMidiEffect() const
{
    return false;
}

double VocalTuneAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VocalTuneAudioProcessor::getNumPrograms()
{
    return 1;
}

int VocalTuneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VocalTuneAudioProcessor::setCurrentProgram(int)
{
}

const juce::String
VocalTuneAudioProcessor::getProgramName(int)
{
    return {};
}

void VocalTuneAudioProcessor::changeProgramName(
    int, const juce::String&)
{
}

void VocalTuneAudioProcessor::getStateInformation(
    juce::MemoryBlock& destinationData)
{
    juce::ignoreUnused(destinationData);
}

void VocalTuneAudioProcessor::setStateInformation(
    const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalTuneAudioProcessor();
}
