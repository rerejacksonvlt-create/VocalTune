#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "correction", "Correction",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "speed", "Retune Speed",
        juce::NormalisableRange<float> (1.0f, 500.0f, 1.0f, 0.35f), 80.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "transition", "Transition",
        juce::NormalisableRange<float> (0.0f, 500.0f, 1.0f), 80.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        "scale", "Scale",
        juce::StringArray { "Chromatic", "Major", "Natural Minor" }, 0));

    return { params.begin(), params.end() };
}

VocalTuneAudioProcessor::VocalTuneAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "VocalTuneParameters", createParameterLayout())
{
}

void VocalTuneAudioProcessor::prepareToPlay (double, int) {}
void VocalTuneAudioProcessor::releaseResources() {}

bool VocalTuneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono()
                             || output == juce::AudioChannelSet::stereo());
}

void VocalTuneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear();

    // Stage 1 is a transparent, safe audio pass-through.
    // Pitch analysis and correction DSP will be added in later stages.
    juce::ignoreUnused (buffer);
}

juce::AudioProcessorEditor* VocalTuneAudioProcessor::createEditor()
{
    return new VocalTuneAudioProcessorEditor (*this);
}

void VocalTuneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VocalTuneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalTuneAudioProcessor();
}
