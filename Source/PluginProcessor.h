#pragma once
#include <JuceHeader.h>
#include "PitchEngine.h"

class MadenciProcessor : public juce::AudioProcessor
{
public:
    MadenciProcessor();
    ~MadenciProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MADENCITUNE"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    PitchEngine engine;
    std::atomic<float> levelIn { 0.f }, levelOut { 0.f };
    std::array<std::atomic<float>, 128> scope;
    std::atomic<int> scopePos { 0 };

private:
    std::array<bool, 128> held {};
    float scopeAcc = 0.f; int scopeCnt = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MadenciProcessor)
};
