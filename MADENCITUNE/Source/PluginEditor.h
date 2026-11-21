#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI.h"

class MadenciEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MadenciEditor (MadenciProcessor&);
    ~MadenciEditor() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawBackground (juce::Graphics&);
    void paintPhotoCard (juce::Graphics&);
    void paintMeters (juce::Graphics&);
    void paintOrb (juce::Graphics&);
    void applyScale();

    MadenciProcessor& proc;
    MadenciLnF lnf;
    juce::Image photo1, photo2, bgCache;

    KnobBox speed, formant, transition, tolerance, amount, vibrato, refHz, mixK, outK;
    NoteKeyboard keys;
    ChoiceStrip rangeStrip;
    juce::ComboBox scaleBox, rootBox;

    juce::Point<float> orbC { 520.f, 258.f };
    float dispCents = 0.f, dispLevel = 0.f, dispIn = 0.f, dispOut = 0.f, phase = 0.f, tgtShown = -1.f;
    bool voiced = false;
    int activePc = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MadenciEditor)
};
