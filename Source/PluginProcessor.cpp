#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout MadenciProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    // SPEED: -10 .. 400 ms. Tus donusumunun ilk %20'si negatif (-10..0) = "sert" bolge.
    NormalisableRange<float> speedRange (-10.f, 400.f,
        [] (float, float, float v) { return v < 0.2f ? jmap (v, 0.f, 0.2f, -10.f, 0.f) : 400.f * square ((v - 0.2f) / 0.8f); },
        [] (float, float, float v) { return v < 0.f ? jmap (v, -10.f, 0.f, 0.f, 0.2f) : 0.2f + 0.8f * std::sqrt (v / 400.f); },
        [] (float s, float e, float v) { return jlimit (s, e, v); });

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "speed", 1 }, "Speed", speedRange, 10.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "transition", 1 }, "Note Transition", NormalisableRange<float> (0.f, 400.f, 0.1f), 40.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tolerance", 1 }, "Tolerance", NormalisableRange<float> (0.f, 100.f, 0.1f), 0.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "vibrato", 1 }, "Vibrato", NormalisableRange<float> (0.f, 200.f, 1.f), 100.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "formant", 1 }, "Formant", NormalisableRange<float> (-12.f, 12.f, 0.1f), 0.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "amount", 1 }, "Correction", NormalisableRange<float> (0.f, 100.f, 1.f), 100.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "reference", 1 }, "Reference Hz", NormalisableRange<float> (430.f, 450.f, 0.1f), 440.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.f, 100.f, 1.f), 100.f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "outgain", 1 }, "Output", NormalisableRange<float> (-12.f, 12.f, 0.1f), 0.f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "range", 1 }, "Range",
        StringArray { "Bass", "Baritone", "Tenor", "Alto", "Mezzo Soprano", "Soprano", "Generic" }, 6));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "madenci", 1 }, "Madenci Modu", false));
    for (int i = 0; i < 12; ++i)
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "n" + String (i), 1 }, "Note " + String (i), true));

    return { p.begin(), p.end() };
}

MadenciProcessor::MadenciProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& s : scope) s.store (0.f);
}

void MadenciProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    setLatencySamples (0); // yol A gecikmesi ~yarim periyot (2-5 ms), degisken oldugu icin 0 bildirilir
    held.fill (false);
}

bool MadenciProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != l.getMainInputChannelSet()) return false;
    return l.getMainOutputChannelSet() == AudioChannelSet::mono() || l.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void MadenciProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) held[(size_t) m.getNoteNumber()] = true;
        else if (m.isNoteOff()) held[(size_t) m.getNoteNumber()] = false;
        else if (m.isAllNotesOff() || m.isAllSoundOff()) held.fill (false);
    }
    int heldMask = 0;
    for (int k = 0; k < 128; ++k) if (held[(size_t) k]) heldMask |= 1 << (k % 12);

    static const float fMin[7] = { 65.f, 80.f, 100.f, 140.f, 160.f, 200.f, 70.f };
    static const float fMax[7] = { 330.f, 400.f, 520.f, 700.f, 880.f, 1100.f, 1000.f };
    const int rng = jlimit (0, 6, (int) apvts.getRawParameterValue ("range")->load());

    EngineParams ep;
    ep.speedMs = apvts.getRawParameterValue ("speed")->load();
    if (apvts.getRawParameterValue ("madenci")->load() < 0.5f) ep.speedMs = jmax (0.f, ep.speedMs); // negatif bolge sadece Madenci Modu aciksa
    ep.transitionMs = apvts.getRawParameterValue ("transition")->load();
    ep.toleranceCents = apvts.getRawParameterValue ("tolerance")->load();
    ep.vibrato = apvts.getRawParameterValue ("vibrato")->load() * 0.01f;
    ep.formantSemis = apvts.getRawParameterValue ("formant")->load();
    ep.amount = apvts.getRawParameterValue ("amount")->load() * 0.01f;
    ep.refHz = apvts.getRawParameterValue ("reference")->load();
    ep.mix = apvts.getRawParameterValue ("mix")->load() * 0.01f;
    ep.outGain = Decibels::decibelsToGain (apvts.getRawParameterValue ("outgain")->load());
    ep.fMin = fMin[rng]; ep.fMax = fMax[rng];
    ep.heldMask = heldMask;
    int mask = 0;
    for (int i = 0; i < 12; ++i) if (apvts.getRawParameterValue ("n" + String (i))->load() > 0.5f) mask |= 1 << i;
    ep.allowedMask = mask;

    const int nch = jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nch < 1 || n < 1) return;

    levelIn.store (buffer.getMagnitude (0, n));
    engine.process (buffer.getArrayOfWritePointers(), nch, n, ep);
    levelOut.store (buffer.getMagnitude (0, n));

    const float* o = buffer.getReadPointer (0);
    for (int i = 0; i < n; ++i)
    {
        scopeAcc = jmax (scopeAcc, std::abs (o[i]));
        if (++scopeCnt >= 192)
        {
            const int pos = scopePos.load();
            scope[(size_t) pos].store (scopeAcc);
            scopePos.store ((pos + 1) & 127);
            scopeAcc = 0.f; scopeCnt = 0;
        }
    }
}

void MadenciProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void MadenciProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* MadenciProcessor::createEditor() { return new MadenciEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MadenciProcessor(); }
