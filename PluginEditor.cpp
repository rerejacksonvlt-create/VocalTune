#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    constexpr int analysisWindowSize = 1024;
    constexpr int analysisHopSize = 256;

    const juce::Colour backgroundColour  { 0xff10121a };
    const juce::Colour panelColour       { 0xff191d29 };
    const juce::Colour borderColour      { 0xff30364a };
    const juce::Colour accentColour      { 0xff9275ff };
    const juce::Colour textColour        { 0xfff2f3fa };
    const juce::Colour mutedColour       { 0xff9299ad };
    const juce::Colour noteColour        { 0xff8066e8 };

    juce::String getNoteName(int midiNote)
    {
        return juce::MidiMessage::getMidiNoteName(
            midiNote, true, true, 3);
    }
}

VocalTuneAudioProcessorEditor::VocalTuneAudioProcessorEditor(
    VocalTuneAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
{
    formatManager.registerBasicFormats();

    setSize(1200, 760);
    setResizable(true, true);
    setResizeLimits(900, 600, 1800, 1200);

    titleLabel.setText(
        "VocalTune",
        juce::dontSendNotification);

    titleLabel.setFont(
        juce::Font(juce::FontOptions(30.0f)));

    titleLabel.setColour(
        juce::Label::textColourId,
        textColour);

    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "VOCAL PITCH EDITOR",
        juce::dontSendNotification);

    subtitleLabel.setFont(
        juce::Font(juce::FontOptions(12.0f)));

    subtitleLabel.setColour(
        juce::Label::textColourId,
        mutedColour);

    addAndMakeVisible(subtitleLabel);

    importButton.setColour(
        juce::TextButton::buttonColourId,
        accentColour);

    importButton.setColour(
    juce::TextButton::buttonColourId,
    accentColour);

importButton.setColour(
    juce::TextButton::textColourOffId,
    textColour);

importButton.onClick = [this]
{
    openFileChooser();
};

addAndMakeVisible(importButton);

    statusLabel.setText(
        "IMPORT A VOCAL FILE TO BEGIN",
        juce::dontSendNotification);

    statusLabel.setFont(
        juce::Font(juce::FontOptions(13.0f)));

    statusLabel.setColour(
        juce::Label::textColourId,
        mutedColour);

    addAndMakeVisible(statusLabel);

    startTimerHz(10);
}

VocalTuneAudioProcessorEditor::~VocalTuneAudioProcessorEditor()
{
    stopTimer();

    if (analysisThread.joinable())
        analysisThread.join();
}

void VocalTuneAudioProcessorEditor::openFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a vocal audio file",
        juce::File{},
        "*.wav;*.aif;*.aiff;*.flac;*.ogg");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (file.existsAsFile())
                loadAudioFile(file);

            fileChooser.reset();
        });
}

bool VocalTuneAudioProcessorEditor::isInterestedInFileDrag(
    const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const juce::File file(path);

        if (file.hasFileExtension(
            ".wav;.wave;.aif;.aiff;.flac;.ogg"))
        {
            return true;
        }
    }

    return false;
}

void VocalTuneAudioProcessorEditor::filesDropped(
    const juce::StringArray& files,
    int x,
    int y)
{
    juce::ignoreUnused(x, y);

    if (files.isEmpty())
        return;

    const juce::File file(files[0]);

    if (file.existsAsFile())
        loadAudioFile(file);
}

void VocalTuneAudioProcessorEditor::loadAudioFile(
    const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        statusLabel.setText(
            "COULD NOT OPEN THIS AUDIO FILE",
            juce::dontSendNotification);

        return;
    }

    if (reader->lengthInSamples <= 0
        || reader->lengthInSamples > 2147483647)
    {
        statusLabel.setText(
            "AUDIO FILE HAS AN UNSUPPORTED LENGTH",
            juce::dontSendNotification);

        return;
    }

    const double newSampleRate = reader->sampleRate;

    if (newSampleRate <= 0.0)
    {
        statusLabel.setText(
            "INVALID AUDIO SAMPLE RATE",
            juce::dontSendNotification);

        return;
    }

    const int numberOfSamples =
        static_cast<int>(reader->lengthInSamples);

    const int numberOfChannels =
        juce::jlimit(1, 2, static_cast<int>(reader->numChannels));

    juce::AudioBuffer<float> newAudio;

    newAudio.setSize(
        numberOfChannels,
        numberOfSamples);

    newAudio.clear();

    const bool readSucceeded = reader->read(
        &newAudio,
        0,
        numberOfSamples,
        0,
        true,
        true);

    if (!readSucceeded)
    {
        statusLabel.setText(
            "ERROR READING AUDIO FILE",
            juce::dontSendNotification);

        return;
    }

    // Stop any previous analysis before replacing its data.
    if (analysisThread.joinable())
        analysisThread.join();

    audioData = std::move(newAudio);
    audioSampleRate = newSampleRate;
    loadedFile = file;

    {
        const std::lock_guard<std::mutex> lock(notesMutex);
        detectedNotes.clear();
    }

    // Reduce the sample rate for pitch analysis.
    // Averaging each group helps reduce aliasing.
    const int stride = juce::jmax(
        1,
        static_cast<int>(std::ceil(newSampleRate / 11025.0)));

    const int reducedSampleCount =
        (numberOfSamples + stride - 1) / stride;

    std::vector<float> monoSamples;
    monoSamples.reserve(
        static_cast<size_t>(reducedSampleCount));

    const float* left = audioData.getReadPointer(0);

    const float* right = numberOfChannels > 1
        ? audioData.getReadPointer(1)
        : nullptr;

    for (int start = 0; start < numberOfSamples; start += stride)
    {
        const int end = juce::jmin(
            numberOfSamples,
            start + stride);

        float sum = 0.0f;

        for (int i = start; i < end; ++i)
        {
            float sample = left[i];

            if (right != nullptr)
                sample = 0.5f * (sample + right[i]);

            sum += sample;
        }

        const float average =
            sum / static_cast<float>(end - start);

        monoSamples.push_back(average);
    }

    const double reducedSampleRate =
        newSampleRate / static_cast<double>(stride);

    statusLabel.setText(
        "ANALYSING VOCAL...",
        juce::dontSendNotification);

    startPitchAnalysis(
        std::move(monoSamples),
        reducedSampleRate);

    repaint();
}

void VocalTuneAudioProcessorEditor::startPitchAnalysis(
    std::vector<float> samples,
    double sampleRate)
{
    analysisComplete.store(false);

    analysisThread = std::thread(
        [this,
         samples = std::move(samples),
         sampleRate]() mutable
        {
            auto results = analysePitch(samples, sampleRate);

            {
                const std::lock_guard<std::mutex> lock(notesMutex);
                detectedNotes = std::move(results);
            }

            analysisComplete.store(true);
        });
}

std::vector<VocalTuneAudioProcessorEditor::DetectedNote>
VocalTuneAudioProcessorEditor::analysePitch(
    const std::vector<float>& samples,
    double sampleRate)
{
    std::vector<DetectedNote> results;

    if (samples.size() < analysisWindowSize
        || sampleRate <= 0.0)
    {
        return results;
    }

    const int minimumLag = juce::jmax(
        2,
        static_cast<int>(sampleRate / 1000.0));

    const int maximumLag = juce::jmin(
        analysisWindowSize / 2,
        static_cast<int>(sampleRate / 65.0));

    int currentNote = -1;
    double noteStart = 0.0;
    double noteEnd = 0.0;
    float noteConfidence = 0.0f;

    std::vector<float> frame(analysisWindowSize);

    auto finishNote = [&]()
    {
        if (currentNote >= 0
            && noteEnd - noteStart >= 0.07)
        {
            results.push_back({
                currentNote,
                noteStart,
                noteEnd,
                noteConfidence
            });
        }

        currentNote = -1;
        noteStart = 0.0;
        noteEnd = 0.0;
        noteConfidence = 0.0f;
    };

    for (size_t start = 0;
         start + analysisWindowSize <= samples.size();
         start += analysisHopSize)
    {
        float mean = 0.0f;

        for (int i = 0; i < analysisWindowSize; ++i)
        {
            frame[static_cast<size_t>(i)] =
                samples[start + static_cast<size_t>(i)];

            mean += frame[static_cast<size_t>(i)];
        }

        mean /= static_cast<float>(analysisWindowSize);

        float energy = 0.0f;

        for (auto& sample : frame)
        {
            sample -= mean;
            energy += sample * sample;
        }

        const float rms = std::sqrt(
            energy / static_cast<float>(analysisWindowSize));

        int detectedNote = -1;
        float bestCorrelation = 0.0f;

        if (rms >= 0.008f)
        {
            for (int lag = minimumLag;
                 lag <= maximumLag;
                 ++lag)
            {
                float correlation = 0.0f;
                float energyA = 0.0f;
                float energyB = 0.0f;

                for (int i = 0;
                     i < analysisWindowSize - lag;
                     ++i)
                {
                    const float a =
                        frame[static_cast<size_t>(i)];

                    const float b =
                        frame[static_cast<size_t>(i + lag)];

                    correlation += a * b;
                    energyA += a * a;
                    energyB += b * b;
                }

                const float denominator =
                    std::sqrt(energyA * energyB);

                if (denominator > 0.000001f)
                    correlation /= denominator;

                if (correlation > bestCorrelation)
                    bestCorrelation = correlation;
            }

            // Find the lag corresponding to the strongest
            // normalised correlation.
            int bestLag = 0;
            float strongestCorrelation = 0.0f;

            for (int lag = minimumLag;
                 lag <= maximumLag;
                 ++lag)
            {
                float correlation = 0.0f;
                float energyA = 0.0f;
                float energyB = 0.0f;

                for (int i = 0;
                     i < analysisWindowSize - lag;
                     ++i)
                {
                    const float a =
                        frame[static_cast<size_t>(i)];

                    const float b =
                        frame[static_cast<size_t>(i + lag)];

                    correlation += a * b;
                    energyA += a * a;
                    energyB += b * b;
                }

                const float denominator =
                    std::sqrt(energyA * energyB);

                if (denominator > 0.000001f)
                    correlation /= denominator;

                if (correlation > strongestCorrelation)
                {
                    strongestCorrelation = correlation;
                    bestLag = lag;
                }
            }

            if (bestLag > 0 && strongestCorrelation >= 0.72f)
            {
                const float frequency =
                    static_cast<float>(sampleRate)
                    / static_cast<float>(bestLag);

                const float midi =
                    69.0f + 12.0f * std::log2(frequency / 440.0f);

                detectedNote = juce::roundToInt(midi);
                bestCorrelation = strongestCorrelation;
            }
        }

        const double frameStart =
            static_cast<double>(start) / sampleRate;

        const double frameEnd =
            static_cast<double>(start + analysisWindowSize)
            / sampleRate;

        if (detectedNote == currentNote && detectedNote >= 0)
        {
            noteEnd = frameEnd;

            noteConfidence =
                juce::jmax(noteConfidence, bestCorrelation);
        }
        else
        {
            finishNote();

            if (detectedNote >= 0)
            {
                currentNote = detectedNote;
                noteStart = frameStart;
                noteEnd = frameEnd;
                noteConfidence = bestCorrelation;
            }
        }
    }

    finishNote();

    return results;
}

void VocalTuneAudioProcessorEditor::timerCallback()
{
    if (analysisComplete.load()
        && statusLabel.getText() == "ANALYSING VOCAL...")
    {
        size_t noteCount = 0;

        {
            const std::lock_guard<std::mutex> lock(notesMutex);
            noteCount = detectedNotes.size();
        }

        statusLabel.setText(
            "ANALYSIS COMPLETE  |  "
                + juce::String(static_cast<int>(noteCount))
                + " NOTES DETECTED",
            juce::dontSendNotification);
    }

    repaint();
}

void VocalTuneAudioProcessorEditor::drawWaveform(
    juce::Graphics& g)
{
    auto area = juce::Rectangle<int>(
        32, 125, getWidth() - 64, 165);

    g.setColour(panelColour);
    g.fillRoundedRectangle(area.toFloat(), 10.0f);

    g.setColour(borderColour);
    g.drawRoundedRectangle(area.toFloat(), 10.0f, 1.0f);

    g.setColour(mutedColour);
    g.setFont(12.0f);

    g.drawText(
        "AUDIO WAVEFORM",
        area.getX() + 14,
        area.getY() + 8,
        180,
        18,
        juce::Justification::left);

    if (audioData.getNumSamples() == 0)
    {
        g.setColour(mutedColour);

        g.drawText(
            "Drag and drop a vocal file here, or select IMPORT VOCAL",
            area.reduced(20),
            juce::Justification::centred);

        return;
    }

    const int waveformTop = area.getY() + 32;
    const int waveformHeight = area.getHeight() - 43;
    const float centreY =
        static_cast<float>(waveformTop)
        + static_cast<float>(waveformHeight) * 0.5f;

    const float halfHeight =
        static_cast<float>(waveformHeight) * 0.45f;

    const int width = area.getWidth() - 24;

    const int numberOfSamples = audioData.getNumSamples();

    const float* left = audioData.getReadPointer(0);

    const float* right = audioData.getNumChannels() > 1
        ? audioData.getReadPointer(1)
        : nullptr;

    g.setColour(juce::Colour(0xff34394b));

    g.drawHorizontalLine(
        juce::roundToInt(centreY),
        static_cast<float>(area.getX() + 12),
        static_cast<float>(area.getRight() - 12));

    g.setColour(accentColour);

    for (int x = 0; x < width; ++x)
    {
        const int start =
            x * numberOfSamples / width;

        const int end =
            juce::jmax(
                start + 1,
                (x + 1) * numberOfSamples / width);

        float minimum = 1.0f;
        float maximum = -1.0f;

        for (int i = start;
             i < juce::jmin(end, numberOfSamples);
             ++i)
        {
            float sample = left[i];

            if (right != nullptr)
                sample = 0.5f * (sample + right[i]);

            minimum = juce::jmin(minimum, sample);
            maximum = juce::jmax(maximum, sample);
        }

        const float top =
            centreY - maximum * halfHeight;

        const float bottom =
            centreY - minimum * halfHeight;

        const float screenX =
            static_cast<float>(area.getX() + 12 + x);

        g.drawVerticalLine(
            juce::roundToInt(screenX),
            top,
            juce::jmax(top + 1.0f, bottom));
    }

    g.setColour(mutedColour);
    g.setFont(11.0f);

    g.drawText(
        loadedFile.getFileName(),
        area.getX() + 14,
        area.getBottom() - 19,
        area.getWidth() - 28,
        14,
        juce::Justification::left);
}

void VocalTuneAudioProcessorEditor::drawPianoRoll(
    juce::Graphics& g)
{
    auto area = juce::Rectangle<int>(
        32, 330, getWidth() - 64, getHeight() - 360);

    g.setColour(panelColour);
    g.fillRoundedRectangle(area.toFloat(), 10.0f);

    g.setColour(borderColour);
    g.drawRoundedRectangle(area.toFloat(), 10.0f, 1.0f);

    g.setColour(mutedColour);
    g.setFont(12.0f);

    g.drawText(
        "DETECTED NOTES",
        area.getX() + 14,
        area.getY() + 8,
        200,
        18,
        juce::Justification::left);

    const int gridTop = area.getY() + 32;
    const int gridBottom = area.getBottom() - 12;
    const int gridHeight = gridBottom - gridTop;

    const int keyWidth = 64;
    const int gridLeft = area.getX() + keyWidth;
    const int gridWidth = area.getRight() - gridLeft - 12;

    std::vector<DetectedNote> notes;

    {
        const std::lock_guard<std::mutex> lock(notesMutex);
        notes = detectedNotes;
    }

    int minimumNote = 48;
    int maximumNote = 72;

    if (!notes.empty())
    {
        minimumNote = 127;
        maximumNote = 0;

        for (const auto& note : notes)
        {
            minimumNote = juce::jmin(
                minimumNote, note.midiNote);

            maximumNote = juce::jmax(
                maximumNote, note.midiNote);
        }

        minimumNote = juce::jmax(24, minimumNote - 3);
        maximumNote = juce::jmin(108, maximumNote + 3);

        if (maximumNote - minimumNote < 24)
        {
            const int missing =
                24 - (maximumNote - minimumNote);

            minimumNote = juce::jmax(
                24, minimumNote - missing / 2);

            maximumNote = juce::jmin(
                108, minimumNote + 24);
        }
    }

    const int numberOfRows =
        maximumNote - minimumNote + 1;

    const float rowHeight =
        static_cast<float>(gridHeight)
        / static_cast<float>(numberOfRows);

    // Piano-roll background and horizontal note lines.
    for (int note = minimumNote;
         note <= maximumNote;
         ++note)
    {
        const int row = maximumNote - note;

        const float y =
            static_cast<float>(gridTop)
            + static_cast<float>(row) * rowHeight;

        const int pitchClass = (note % 12 + 12) % 12;

        const bool isC = pitchClass == 0;

        g.setColour(
            isC
                ? juce::Colour(0xff252b3b)
                : juce::Colour(0xff1d2230));

        g.fillRect(
            gridLeft,
            juce::roundToInt(y),
            gridWidth,
            juce::roundToInt(rowHeight) + 1);

        g.setColour(
            isC
                ? juce::Colour(0xff454d64)
                : juce::Colour(0xff30364a));

        g.drawHorizontalLine(
            juce::roundToInt(y),
            static_cast<float>(gridLeft),
            static_cast<float>(gridLeft + gridWidth));

        g.setColour(mutedColour);
        g.setFont(10.0f);

        g.drawText(
            getNoteName(note),
            area.getX() + 5,
            juce::roundToInt(y),
            keyWidth - 12,
            juce::roundToInt(rowHeight),
            juce::Justification::centredRight);
    }

    const double duration =
        audioData.getNumSamples() > 0
            ? static_cast<double>(audioData.getNumSamples())
                / audioSampleRate
            : 0.0;

    if (duration > 0.0)
    {
        const int numberOfSeconds =
            static_cast<int>(std::ceil(duration));

        for (int second = 0;
             second <= numberOfSeconds;
             ++second)
        {
            const float x =
                static_cast<float>(gridLeft)
                + static_cast<float>(second / duration)
                    * static_cast<float>(gridWidth);

            if (x > static_cast<float>(gridLeft + gridWidth))
                break;

            g.setColour(juce::Colour(0xff34394b));

            g.drawVerticalLine(
                juce::roundToInt(x),
                static_cast<float>(gridTop),
                static_cast<float>(gridBottom));
        }
    }

    if (duration <= 0.0 || notes.empty())
    {
        g.setColour(mutedColour);

        g.drawText(
            "Your detected vocal notes will appear here",
            gridLeft + 10,
            gridTop + 10,
            gridWidth - 20,
            30,
            juce::Justification::centred);

        return;
    }

    // Draw one block for each detected note.
    for (const auto& note : notes)
    {
        if (note.midiNote < minimumNote
            || note.midiNote > maximumNote)
        {
            continue;
        }

        const float x =
            static_cast<float>(gridLeft)
            + static_cast<float>(
                note.startSeconds / duration)
                * static_cast<float>(gridWidth);

        const float noteWidth =
            static_cast<float>(
                (note.endSeconds - note.startSeconds)
                / duration)
            * static_cast<float>(gridWidth);

        const int row = maximumNote - note.midiNote;

        const float y =
            static_cast<float>(gridTop)
            + static_cast<float>(row) * rowHeight;

        auto noteBounds = juce::Rectangle<float>(
            x + 1.0f,
            y + 1.0f,
            juce::jmax(3.0f, noteWidth - 2.0f),
            juce::jmax(3.0f, rowHeight - 2.0f));

        g.setColour(noteColour);

        g.fillRoundedRectangle(noteBounds, 4.0f);

        g.setColour(juce::Colour(0xffc7baff));

        g.drawRoundedRectangle(
            noteBounds,
            4.0f,
            1.0f);

        if (noteBounds.getWidth() > 35.0f
            && noteBounds.getHeight() > 13.0f)
        {
            g.setColour(textColour);
            g.setFont(10.0f);

            g.drawText(
                getNoteName(note.midiNote),
                noteBounds.toNearestInt().reduced(3),
                juce::Justification::centredLeft);
        }
    }
}

void VocalTuneAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    g.setColour(textColour);
    g.setFont(juce::Font(juce::FontOptions(15.0f)));

    g.drawText(
        "VocalTune",
        32,
        20,
        250,
        35,
        juce::Justification::left);

    g.setColour(borderColour);

    g.drawHorizontalLine(
        105,
        32.0f,
        static_cast<float>(getWidth() - 32));

    drawWaveform(g);
    drawPianoRoll(g);
}

void VocalTuneAudioProcessorEditor::resized()
{
    titleLabel.setBounds(32, 17, 250, 38);
    subtitleLabel.setBounds(34, 57, 300, 22);

    importButton.setBounds(
        getWidth() - 205,
        27,
        173,
        42);

    statusLabel.setBounds(
        34,
        88,
        getWidth() - 68,
        22);
}