
#include "PluginEditor.h"

VocalTuneAudioProcessorEditor::
VocalTuneAudioProcessorEditor(
    VocalTuneAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    formatManager.registerBasicFormats();

    setSize(1100, 700);
    setResizable(true, true);
    setResizeLimits(800, 550, 1800, 1200);

    titleLabel.setText(
        "VOCALTUNE", juce::dontSendNotification);

    titleLabel.setFont(
        juce::Font(27.0f, juce::Font::bold));

    titleLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xfff2f2f2));

    addAndMakeVisible(titleLabel);

    fileLabel.setText(
        "No audio loaded",
        juce::dontSendNotification);

    fileLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(0xffb8bdc7));

    addAndMakeVisible(fileLabel);

    importButton.onClick = [this]
    {
        importAudio();
    };

    importButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour(0xff394b68));

    addAndMakeVisible(importButton);

    analyseButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colour(0xff267d70));

    analyseButton.onClick = [this]
    {
        repaint();
    };

    addAndMakeVisible(analyseButton);
}

void VocalTuneAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff171a20));

    auto bounds = getLocalBounds();

    // Header
    auto header = bounds.removeFromTop(75);

    g.setColour(juce::Colour(0xff202630));
    g.fillRect(header);

    g.setColour(juce::Colour(0xff39414e));
    g.drawHorizontalLine(75, 0.0f,
                         (float)getWidth());

    // Main workspace
    auto workspace = bounds.reduced(18);

    auto pianoArea = workspace.removeFromTop(
        (int)(workspace.getHeight() * 0.55f));

    g.setColour(juce::Colour(0xff202630));
    g.fillRoundedRectangle(
        pianoArea.toFloat(), 8.0f);

    drawPianoRoll(g, pianoArea.reduced(10));

    // Waveform section
    auto waveformArea = workspace.reduced(0, 12);

    g.setColour(juce::Colour(0xff202630));
    g.fillRoundedRectangle(
        waveformArea.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff9ca6b5));
    g.setFont(14.0f);

    g.drawText(
        "AUDIO WAVEFORM",
        waveformArea.removeFromTop(28),
        juce::Justification::centredLeft);

    auto waveform = waveformArea.reduced(10);

    if (thumbnail.getNumChannels() > 0)
    {
        g.setColour(juce::Colour(0xff45b7a0));

        thumbnail.drawChannels(
            &g,
            waveform,
            0.0,
            thumbnail.getTotalLength(),
            1.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff586170));

        g.drawText(
            "Import an audio file to display its waveform",
            waveform,
            juce::Justification::centred);
    }
}

void VocalTuneAudioProcessorEditor::drawPianoRoll(
    juce::Graphics& g,
    juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xffaeb8c6));
    g.setFont(14.0f);

    g.drawText(
        "PITCH EDITOR",
        area.removeFromTop(25),
        juce::Justification::centredLeft);

    const int keyboardWidth = 55;
    const int keyHeight = 22;

    auto grid = area;

    auto keyboard = grid.removeFromLeft(keyboardWidth);

    for (int note = 0; note < 24; ++note)
    {
        auto key = keyboard.removeFromTop(keyHeight);

        const bool blackKey =
            note % 12 == 1 ||
            note % 12 == 3 ||
            note % 12 == 6 ||
            note % 12 == 8 ||
            note % 12 == 10;

        g.setColour(
            blackKey
                ? juce::Colour(0xff303743)
                : juce::Colour(0xff454e5b));

        g.fillRect(key);

        g.setColour(juce::Colour(0xff9da6b2));

        g.drawRect(key);

        g.setFont(10.0f);

        g.drawText(
            juce::String(60 - note),
            key,
            juce::Justification::centred);
    }

    const int rows = juce::jmax(1, grid.getHeight() / keyHeight);

    for (int row = 0; row <= rows; ++row)
    {
        int y = grid.getY() + row * keyHeight;

        g.setColour(
            row % 2 == 0
                ? juce::Colour(0xff3b4655)
                : juce::Colour(0xff303a48));

        g.drawHorizontalLine(
            y,
            (float)grid.getX(),
            (float)grid.getRight());
    }

    for (int column = 0; column < 16; ++column)
    {
        int x = grid.getX() + column * grid.getWidth() / 16;

        g.setColour(juce::Colour(0xff343d49));

        g.drawVerticalLine(
            x,
            (float)grid.getY(),
            (float)grid.getBottom());
    }

    // Placeholder note blocks for the editor layout.
    // Actual detected notes will replace these.
    g.setColour(juce::Colour(0xff319b89));

    g.fillRoundedRectangle(
        (float)grid.getX() + 80.0f,
        (float)grid.getY() + 65.0f,
        95.0f, 17.0f, 4.0f);

    g.fillRoundedRectangle(
        (float)grid.getX() + 190.0f,
        (float)grid.getY() + 110.0f,
        125.0f, 17.0f, 4.0f);

    g.fillRoundedRectangle(
        (float)grid.getX() + 335.0f,
        (float)grid.getY() + 87.0f,
        85.0f, 17.0f, 4.0f);
}

void VocalTuneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop(75);

    titleLabel.setBounds(
        header.removeFromLeft(230).reduced(18, 0));

    fileLabel.setBounds(
        header.removeFromLeft(350).reduced(5, 0));

    analyseButton.setBounds(
        header.removeFromRight(110).reduced(5, 18));

    importButton.setBounds(
        header.removeFromRight(145).reduced(5, 18));
}

void VocalTuneAudioProcessorEditor::importAudio()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select a vocal recording",
        juce::File{},
        "*.wav;*.mp3");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();

            if (file.existsAsFile())
            {
                thumbnail.setSource(
                    new juce::FileInputSource(file));

                fileLabel.setText(
                    file.getFileName(),
                    juce::dontSendNotification);

                repaint();
            }
        });
}
