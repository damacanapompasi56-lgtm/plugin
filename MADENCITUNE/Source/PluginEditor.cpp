#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

MadenciEditor::MadenciEditor (MadenciProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      speed (p.apvts, "speed", "SPEED", [] (double v) { return String (v, 1) + " ms"; }, true, false,
             [] (double v) { return v < -0.05 ? String ("SERT MOD") : String(); }),
      formant (p.apvts, "formant", "FORMANT",
               [] (double v) { return v <= -11.95 ? TR ("MADENCI MOD") : String (v, 1) + " st"; }, true, false,
               [] (double v) { return v <= -11.95 ? TR ("madencileştirme !!!") : v < -0.05 ? TR ("madencileştirme") : v > 0.05 ? TR ("teknoşlaştırma") : String ("formant"); }),
      transition (p.apvts, "transition", "NOTE TRANS.", [] (double v) { return String (v, 0) + " ms"; }),
      tolerance (p.apvts, "tolerance", "TOLERANCE", [] (double v) { return v < 0.5 ? String ("Off") : String (v, 1) + " cents"; }),
      amount (p.apvts, "amount", "CORRECTION", [] (double v) { return String ((int) v) + " %"; }),
      vibrato (p.apvts, "vibrato", "VIBRATO", [] (double v) { return String ((int) v) + " %"; }, false, true, nullptr, TR ("orospu çocuğu")),
      refHz (p.apvts, "reference", "REF Hz", [] (double v) { return String (v, 1); }),
      mixK (p.apvts, "mix", "MIX", [] (double v) { return String ((int) v) + " %"; }),
      outK (p.apvts, "outgain", "OUTPUT", [] (double v) { return String (v, 1) + " dB"; }),
      keys (p.apvts),
      rangeStrip (p.apvts, "range", StringArray { "Bass", "Baritone", "Tenor", "Alto", "Mezzo", "Soprano", "Generic" })
{
    setLookAndFeel (&lnf);
    photo1 = ImageCache::getFromMemory (BinaryData::photo1_png, BinaryData::photo1_pngSize);
    photo2 = ImageCache::getFromMemory (BinaryData::photo2_png, BinaryData::photo2_pngSize);

    for (auto* c : std::initializer_list<Component*> { &speed, &formant, &transition, &tolerance, &amount, &vibrato,
                                                        &refHz, &mixK, &outK, &keys, &rangeStrip, &scaleBox, &rootBox })
        addAndMakeVisible (c);

    scaleBox.addItem ("Custom", 1); scaleBox.addItem ("Chromatic", 2); scaleBox.addItem ("Major", 3); scaleBox.addItem ("Minor", 4);
    bool allOn = true;
    for (int i = 0; i < 12; ++i) if (p.apvts.getRawParameterValue ("n" + String (i))->load() < 0.5f) allOn = false;
    scaleBox.setSelectedId (allOn ? 2 : 1, dontSendNotification);
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i) rootBox.addItem (names[i], i + 1);
    rootBox.setSelectedId (1, dontSendNotification);
    scaleBox.onChange = [this] { applyScale(); };
    rootBox.onChange = [this] { applyScale(); };
    keys.onUserEdit = [this] { scaleBox.setSelectedId (1, dontSendNotification); };

    setSize (1180, 700);
    startTimerHz (30);
}

void MadenciEditor::applyScale()
{
    const int type = scaleBox.getSelectedId();
    if (type == 1) return;
    static const int major[7] = { 0, 2, 4, 5, 7, 9, 11 }, minor[7] = { 0, 2, 3, 5, 7, 8, 10 };
    const int root = rootBox.getSelectedId() - 1;
    for (int pc = 0; pc < 12; ++pc)
    {
        bool on = true;
        if (type != 2)
        {
            on = false;
            const int* s = type == 3 ? major : minor;
            for (int k = 0; k < 7; ++k) if ((s[k] + root) % 12 == pc) on = true;
        }
        if (auto* prm = proc.apvts.getParameter ("n" + String (pc)))
        {
            prm->beginChangeGesture(); prm->setValueNotifyingHost (on ? 1.f : 0.f); prm->endChangeGesture();
        }
    }
    keys.repaint();
}

void MadenciEditor::resized()
{
    speed.setBounds (780, 84, 185, 205);
    formant.setBounds (975, 84, 185, 205);
    transition.setBounds (780, 300, 125, 128);
    tolerance.setBounds (905, 300, 125, 128);
    amount.setBounds (1030, 300, 125, 128);
    vibrato.setBounds (24, 352, 242, 48);
    keys.setBounds (24, 468, 1132, 104);
    rangeStrip.setBounds (24, 614, 540, 34);
    scaleBox.setBounds (590, 614, 130, 32);
    rootBox.setBounds (730, 614, 70, 32);
    refHz.setBounds (820, 592, 110, 92);
    mixK.setBounds (940, 592, 110, 92);
    outK.setBounds (1050, 592, 110, 92);

    bgCache = Image (Image::ARGB, getWidth(), getHeight(), true);
    Graphics bg (bgCache);
    drawBackground (bg);
}

void MadenciEditor::drawBackground (Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();
    g.setGradientFill (ColourGradient (Colour (0xff1a0b33), W * 0.5f, 0.f, Colour (0xff06030b), W * 0.5f, H, false));
    g.fillAll();
    g.setGradientFill (ColourGradient (mdn::violet().withAlpha (0.35f), orbC.x, orbC.y, Colours::transparentBlack, orbC.x + 460.f, orbC.y, true));
    g.fillRect (0.f, 0.f, W, H);
    g.setColour (Colours::white.withAlpha (0.025f));
    for (int x = 0; x < (int) W; x += 40) g.fillRect (x, 0, 1, (int) H);
    for (int y = 0; y < (int) H; y += 40) g.fillRect (0, y, (int) W, 1);

    // header
    g.setGradientFill (ColourGradient (Colour (0xff24104a), 0.f, 0.f, Colour (0xff0d0620), W, 0.f, false));
    g.fillRect (0.f, 0.f, W, 64.f);
    g.setGradientFill (ColourGradient (mdn::violet(), 0.f, 0.f, mdn::pink(), W, 0.f, false));
    g.fillRect (0.f, 63.f, W, 2.f);

    Path hex; hex.addPolygon (Point<float> (46.f, 32.f), 6, 25.f, 0.f);
    g.setColour (mdn::purple().withAlpha (0.3f)); g.strokePath (hex, PathStrokeType (7.f));
    g.setColour (mdn::purple()); g.strokePath (hex, PathStrokeType (2.f));
    drawPickaxe (g, { 46.f, 34.f }, 17.f, 0.75f, true);

    auto f = mdn::font (32.f, Font::bold); f.setExtraKerningFactor (0.12f);
    g.setFont (f);
    g.setGradientFill (ColourGradient (Colours::white, 90.f, 0.f, mdn::purple(), 420.f, 0.f, false));
    g.drawText ("MADENCITUNE", 88, 8, 400, 48, Justification::centredLeft);
    g.setFont (mdn::font (12.f, Font::bold)); g.setColour (mdn::lilac().withAlpha (0.8f));
    g.drawText (TR ("MADEN OCAĞI EDITION  ·  AUTO-TUNE  ·  VST3"), getWidth() - 440, 8, 420, 48, Justification::centredRight);

    // paneller
    auto panel = [&] (Rectangle<float> r)
    {
        g.setColour (Colour (0xff120a22).withAlpha (0.78f)); g.fillRoundedRectangle (r, 14.f);
        g.setColour (mdn::violet().withAlpha (0.45f)); g.drawRoundedRectangle (r, 14.f, 1.2f);
    };
    panel ({ 14.f, 76.f, 262.f, 360.f });
    panel ({ 284.f, 76.f, 486.f, 360.f });
    panel ({ 778.f, 76.f, 388.f, 360.f });
    panel ({ 14.f, 446.f, 1152.f, 136.f });
    panel ({ 14.f, 588.f, 1152.f, 100.f });

    g.setFont (mdn::font (12.f, Font::bold)); g.setColour (mdn::lilac());
    g.drawText ("NOTES  -  " + TR ("tuşlara tıkla: nota aç / kapat"), 24, 448, 500, 18, Justification::centredLeft);
    g.drawText ("RANGE", 24, 592, 100, 18, Justification::centredLeft);
    g.drawText ("SCALE", 590, 592, 100, 18, Justification::centredLeft);
    g.drawText ("KEY", 730, 592, 60, 18, Justification::centredLeft);
}

void MadenciEditor::paint (Graphics& g)
{
    g.drawImageAt (bgCache, 0, 0);
    paintPhotoCard (g);
    paintMeters (g);
    paintOrb (g);
}

void MadenciEditor::paintPhotoCard (Graphics& g)
{
    const Rectangle<float> card (24.f, 88.f, 242.f, 172.f);
    Path clip; clip.addRoundedRectangle (card, 12.f);
    g.saveState();
    g.reduceClipRegion (clip, {});
    g.setColour (Colours::black); g.fillRect (card);
    if (photo1.isValid()) g.drawImage (photo1, card.withTrimmedBottom (24.f), RectanglePlacement::fillDestination);
    g.setGradientFill (ColourGradient (mdn::violet().withAlpha (0.35f), card.getX(), card.getY(), Colours::transparentBlack, card.getX(), card.getBottom(), false));
    g.fillRect (card);
    g.setColour (Colours::black.withAlpha (0.12f));
    for (float y = card.getY(); y < card.getBottom(); y += 3.f) g.fillRect (card.getX(), y, card.getWidth(), 1.f);
    const auto cap = card.removeFromBottom (24.f);
    g.setColour (Colour (0xff0d0620).withAlpha (0.9f)); g.fillRect (cap);
    g.setColour (mdn::pink()); g.setFont (mdn::font (12.f, Font::bold));
    g.drawText (TR ("MADENCİ 01  ·  VARDİYADA"), cap, Justification::centred);
    g.restoreState();
    g.setColour (mdn::purple().withAlpha (0.3f)); g.drawRoundedRectangle (card.expanded (0.f, 12.f).withTrimmedBottom (0.f), 12.f, 4.f);
    g.setColour (mdn::purple()); g.drawRoundedRectangle (Rectangle<float> (24.f, 88.f, 242.f, 172.f), 12.f, 1.6f);
}

void MadenciEditor::paintMeters (Graphics& g)
{
    auto bar = [&] (float y, const String& label, float v)
    {
        g.setColour (mdn::lilac()); g.setFont (mdn::font (11.f, Font::bold));
        g.drawText (label, 24, (int) y - 4, 34, 18, Justification::centredLeft);
        Rectangle<float> r (62.f, y, 204.f, 10.f);
        g.setColour (Colour (0xff241a3a)); g.fillRoundedRectangle (r, 5.f);
        const float w = jlimit (0.f, 1.f, std::sqrt (v)) * r.getWidth();
        g.setGradientFill (ColourGradient (mdn::violet(), r.getX(), 0.f, mdn::pink(), r.getRight(), 0.f, false));
        g.fillRoundedRectangle (r.withWidth (jmax (w, 0.f)), 5.f);
    };
    bar (282.f, "IN", dispIn);
    bar (306.f, "OUT", dispOut);
}

void MadenciEditor::paintOrb (Graphics& g)
{
    const auto c = orbC; const float R = 150.f;
    g.saveState();
    g.reduceClipRegion (Rectangle<int> (284, 76, 486, 360));

    g.setGradientFill (ColourGradient (mdn::purple().withAlpha (0.20f + 0.35f * dispIn), c.x, c.y, Colours::transparentBlack, c.x + R * 1.6f, c.y, true));
    g.fillEllipse (c.x - R * 1.6f, c.y - R * 1.6f, R * 3.2f, R * 3.2f);

    // donen noktalar
    for (int i = 0; i < 72; ++i)
    {
        const float a = (float) i / 72.f * MathConstants<float>::twoPi + phase;
        const float br = 0.3f + 0.7f * (0.5f + 0.5f * std::sin (a * 3.f + phase * 2.f));
        const float rr = R - 2.f + 5.f * dispIn * std::sin (a * 5.f + phase * 4.f);
        const float s = 1.6f + 2.6f * br * (0.4f + dispIn);
        g.setColour (mdn::purple().withAlpha (0.25f + 0.75f * br));
        g.fillEllipse (c.x + rr * std::sin (a) - s * 0.5f, c.y - rr * std::cos (a) - s * 0.5f, s, s);
    }

    // foto (orb icinde)
    const float pr = R - 26.f;
    Path cp; cp.addEllipse (c.x - pr, c.y - pr, pr * 2.f, pr * 2.f);
    g.saveState();
    g.reduceClipRegion (cp, {});
    g.setColour (Colours::black); g.fillEllipse (c.x - pr, c.y - pr, pr * 2.f, pr * 2.f);
    if (photo2.isValid())
    {
        const int sw = photo2.getWidth(), sy = (int) ((float) photo2.getHeight() * 0.12f);
        const int sh = jmin (sw, photo2.getHeight() - sy);
        g.drawImage (photo2, (int) (c.x - pr), (int) (c.y - pr), (int) (pr * 2.f), (int) (pr * 2.f), 0, sy, sw, sh);
    }
    g.setGradientFill (ColourGradient (Colours::transparentBlack, c.x, c.y, mdn::violet().withAlpha (0.55f), c.x, c.y - pr, true));
    g.fillEllipse (c.x - pr, c.y - pr, pr * 2.f, pr * 2.f);
    g.restoreState();
    g.setColour (mdn::purple().withAlpha (0.35f)); g.drawEllipse (c.x - pr, c.y - pr, pr * 2.f, pr * 2.f, 7.f);
    g.setColour (Colours::white.withAlpha (0.8f)); g.drawEllipse (c.x - pr, c.y - pr, pr * 2.f, pr * 2.f, 1.5f);

    // cent yayi
    const float ar = R + 24.f;
    auto pt = [&] (float a, float r) { return Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };
    for (int ct = -50; ct <= 50; ct += 10)
    {
        const float a = (float) ct / 50.f * 0.95f;
        g.setColour (ct == 0 ? Colours::white : mdn::lilac().withAlpha (0.55f));
        g.drawLine (Line<float> (pt (a, ar - (ct == 0 ? 8.f : 4.f)), pt (a, ar + 4.f)), ct == 0 ? 2.f : 1.2f);
    }
    if (voiced)
    {
        const float a = jlimit (-50.f, 50.f, dispCents) / 50.f * 0.95f;
        Path arc; arc.addCentredArc (c.x, c.y, ar, ar, 0.f, jmin (0.f, a), jmax (0.f, a), true);
        g.setColour (mdn::pink().withAlpha (0.8f)); g.strokePath (arc, PathStrokeType (4.f));
        const auto n = pt (a, ar);
        g.setColour (mdn::pink().withAlpha (0.4f)); g.fillEllipse (n.x - 9.f, n.y - 9.f, 18.f, 18.f);
        g.setColour (Colours::white); g.fillEllipse (n.x - 5.f, n.y - 5.f, 10.f, 10.f);
    }

    // nota hapi
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const Rectangle<float> pill (c.x - 64.f, c.y + 76.f, 128.f, 64.f);
    g.setColour (Colour (0xff0b0614).withAlpha (0.85f)); g.fillRoundedRectangle (pill, 16.f);
    g.setColour (mdn::purple()); g.drawRoundedRectangle (pill, 16.f, 1.6f);
    g.setColour (Colours::white);
    if (voiced)
    {
        const int n = roundToInt (tgtShown);
        g.setFont (mdn::font (36.f, Font::bold));
        g.drawText (String (names[((n % 12) + 12) % 12]) + String (n / 12 - 1), pill.withTrimmedBottom (18.f).toNearestInt(), Justification::centred);
        g.setFont (mdn::font (11.f, Font::bold)); g.setColour (mdn::pink());
        g.drawText (String (dispCents >= 0 ? "+" : "") + String (roundToInt (dispCents)) + " cent", pill.removeFromBottom (20.f).toNearestInt(), Justification::centred);
    }
    else
    {
        g.setFont (mdn::font (30.f, Font::bold)); g.setColour (mdn::lilac().withAlpha (0.6f));
        g.drawText ("--", pill.toNearestInt(), Justification::centred);
    }
    g.restoreState();
}

void MadenciEditor::timerCallback()
{
    const float det = proc.engine.detMidi.load(), tgt = proc.engine.tgtMidi.load();
    voiced = det > 0.f;
    const float li = jlimit (0.f, 1.f, proc.levelIn.load()), lo = jlimit (0.f, 1.f, proc.levelOut.load());
    dispIn += (li - dispIn) * (li > dispIn ? 0.6f : 0.15f);
    dispOut += (lo - dispOut) * (lo > dispOut ? 0.6f : 0.15f);
    phase += 0.02f + 0.08f * dispIn;
    if (voiced)
    {
        dispCents += (jlimit (-60.f, 60.f, (det - tgt) * 100.f) - dispCents) * 0.4f;
        tgtShown = tgt;
        activePc = ((roundToInt (tgt) % 12) + 12) % 12;
    }
    else { dispCents *= 0.8f; activePc = -1; }
    keys.setActivePc (activePc);
    rangeStrip.refresh();
    repaint (284, 76, 486, 360);
    repaint (24, 270, 242, 52);
}
