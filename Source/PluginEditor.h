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
    void syncLock();
    bool madenciOn() const { return proc.apvts.getRawParameterValue ("madenci")->load() > 0.5f; }

    MadenciProcessor& proc;
    MadenciLnF lnf;
    juce::Image photo1, photo2, bgCache;
    std::array<juce::Image, 8> blurLevels;

    KnobBox speed, formant, transition, tolerance, amount, vibrato, refHz, mixK, outK;
    juce::ToggleButton madenciBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> madenciAtt;
    NoteKeyboard keys;
    ChoiceStrip rangeStrip;
    juce::ComboBox scaleBox, rootBox;

    const juce::Point<float> orbC { 544.f, 284.f };
    std::array<float, 64> scopeDisp {};
    float dispCents = 0.f, dispIn = 0.f, dispOut = 0.f, phase = 0.f, tgtShown = -1.f, dispRobot = 0.f, detHz = 0.f, tgtHz = 0.f;
    bool voiced = false;
    int activePc = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MadenciEditor)
};
