#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

MadenciEditor::MadenciEditor (MadenciProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      speed (p.apvts, "speed", "SPEED", [] (double v) { return String (v, 1) + " ms"; }, true, false,
             [] (double v) { return v < -0.05 ? String ("SERT MOD") : String(); }, {}, 0.2f),
      formant (p.apvts, "formant", "FORMANT",
               [] (double v) { return v <= -11.95 ? TR ("MADENCI MOD") : String (v, 1) + " st"; }, true, false,
               [] (double v) { return v <= -11.95 ? TR ("madencileştirme !!!") : v < -0.05 ? TR ("madencileştirme") : v > 0.05 ? TR ("teknoşlaştırma") : String ("formant"); }),
      transition (p.apvts, "transition", "NOTE TRANS.", [] (double v) { return String (v, 0) + " ms"; }),
      tolerance (p.apvts, "tolerance", "TOLERANCE", [] (double v) { return v < 0.5 ? String ("Off") : String (v, 1) + " ct"; }),
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

    // orb fotosu icin onceden blur seviyeleri (robotluk arttikca bulaniklasir)
    {
        const int S = 440;
        Image base (Image::ARGB, S, S, true);
        {
            Graphics gi (base);
            gi.setColour (Colours::black); gi.fillAll();
            if (photo2.isValid())
            {
                const int sw = photo2.getWidth(), sy = (int) ((float) photo2.getHeight() * 0.12f);
                const int sh = jmin (sw, photo2.getHeight() - sy);
                gi.drawImage (photo2, 0, 0, S, S, 0, sy, sw, sh);
            }
        }
        static const int div[8] = { 1, 2, 3, 4, 6, 9, 13, 19 };
        for (int i = 0; i < 8; ++i)
        {
            if (div[i] == 1) { blurLevels[(size_t) i] = base; continue; }
            const int sm = jmax (4, S / div[i]);
            blurLevels[(size_t) i] = base.rescaled (sm, sm, Graphics::highResamplingQuality).rescaled (S, S, Graphics::highResamplingQuality);
        }
    }

    for (auto* c : std::initializer_list<Component*> { &speed, &formant, &transition, &tolerance, &amount, &vibrato,
                                                        &refHz, &mixK, &outK, &keys, &rangeStrip, &scaleBox, &rootBox, &madenciBtn })
        addAndMakeVisible (c);

    madenciBtn.setButtonText ("MADENCI MODU");
    madenciAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "madenci", madenciBtn);
    madenciBtn.onClick = [this] { syncLock(); };

    // negatif speed bolgesi sadece Madenci Modu aciksa
    speed.getSlider().onValueChange = [this]
    {
        auto& sl = speed.getSlider();
        if (sl.getValue() < 0.0 && ! madenciOn()) sl.setValue (0.0, sendNotificationSync);
        speed.repaint();
    };

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

    setSize (1200, 736);
    syncLock();
    startTimerHz (30);
}

void MadenciEditor::syncLock()
{
    const bool on = madenciOn();
    auto& sl = speed.getSlider();
    if (! on && sl.getValue() < 0.0) sl.setValue (0.0, sendNotificationSync);
    speed.setLocked (! on);
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
    madenciBtn.setBounds (826, 104, 348, 40);
    speed.setBounds (826, 152, 172, 198);
    formant.setBounds (1002, 152, 172, 198);
    transition.setBounds (826, 356, 112, 108);
    tolerance.setBounds (944, 356, 112, 108);
    amount.setBounds (1062, 356, 112, 108);
    vibrato.setBounds (26, 376, 236, 50);
    keys.setBounds (26, 506, 1148, 98);
    rangeStrip.setBounds (26, 652, 520, 34);
    scaleBox.setBounds (572, 652, 130, 34);
    rootBox.setBounds (712, 652, 70, 34);
    refHz.setBounds (806, 626, 112, 92);
    mixK.setBounds (926, 626, 112, 92);
    outK.setBounds (1046, 626, 112, 92);

    bgCache = Image (Image::ARGB, getWidth(), getHeight(), true);
    Graphics bg (bgCache);
    drawBackground (bg);
}

void MadenciEditor::drawBackground (Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight();
    g.setGradientFill (ColourGradient (Colour (0xff1b0d38), W * 0.5f, 0.f, Colour (0xff05020a), W * 0.5f, H, false));
    g.fillAll();
    g.setGradientFill (ColourGradient (mdn::violet().withAlpha (0.38f), orbC.x, orbC.y, Colours::transparentBlack, orbC.x + 520.f, orbC.y, true));
    g.fillRect (0.f, 0.f, W, H);

    // petek (hex) dokusu
    {
        Path hexes; const float rr = 17.f, dx = rr * 1.7320508f, dy = rr * 1.5f;
        for (int row = 0; row * dy < H + rr; ++row)
            for (int col = 0; col * dx < W + rr; ++col)
                hexes.addPolygon (Point<float> ((float) col * dx + ((row & 1) ? dx * 0.5f : 0.f), (float) row * dy), 6, rr, 0.5235988f);
        g.setColour (Colours::white.withAlpha (0.028f)); g.strokePath (hexes, PathStrokeType (1.f));
    }
    // vinyet
    g.setGradientFill (ColourGradient (Colours::transparentBlack, W * 0.5f, H * 0.5f, Colours::black.withAlpha (0.55f), 0.f, 0.f, true));
    g.fillRect (0.f, 0.f, W, H);

    // header
    g.setGradientFill (ColourGradient (Colour (0xff2a1258), 0.f, 0.f, Colour (0xff0a0518), W, 0.f, false));
    g.fillRect (0.f, 0.f, W, 68.f);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), 0.f, 0.f, Colours::transparentWhite, 0.f, 34.f, false));
    g.fillRect (0.f, 0.f, W, 34.f);
    g.setGradientFill (ColourGradient (mdn::violet(), 0.f, 0.f, mdn::pink(), W, 0.f, false));
    g.fillRect (0.f, 66.f, W, 2.f);

    Path hex; hex.addPolygon (Point<float> (48.f, 34.f), 6, 26.f, 0.f);
    g.setColour (mdn::purple().withAlpha (0.28f)); g.strokePath (hex, PathStrokeType (8.f));
    g.setGradientFill (ColourGradient (mdn::lilac(), 30.f, 10.f, mdn::violet(), 66.f, 58.f, false)); g.strokePath (hex, PathStrokeType (2.2f));
    drawPickaxe (g, { 48.f, 36.f }, 18.f, 0.75f, true);

    auto f = mdn::font (34.f, Font::bold); f.setExtraKerningFactor (0.14f);
    g.setFont (f);
    g.setColour (mdn::purple().withAlpha (0.35f)); g.drawText ("MADENCITUNE", 92, 10, 420, 52, Justification::centredLeft);
    g.setGradientFill (ColourGradient (Colours::white, 92.f, 12.f, mdn::purple(), 92.f, 56.f, false));
    g.drawText ("MADENCITUNE", 90, 8, 420, 52, Justification::centredLeft);
    g.setFont (mdn::font (11.5f, Font::bold)); g.setColour (mdn::lilac().withAlpha (0.75f));
    g.drawText (TR ("MADEN OCAĞI EDITION   ·   PITCH CORRECTION   ·   VST3"), getWidth() - 520, 8, 500, 52, Justification::centredRight);

    // paneller
    drawPanel (g, { 14.f, 80.f, 260.f, 390.f });
    drawPanel (g, { 284.f, 80.f, 520.f, 390.f });
    drawPanel (g, { 814.f, 80.f, 372.f, 390.f });
    drawPanel (g, { 14.f, 482.f, 1172.f, 128.f });
    drawPanel (g, { 14.f, 620.f, 1172.f, 102.f });

    auto title = mdn::font (11.5f, Font::bold); title.setExtraKerningFactor (0.18f);
    g.setFont (title); g.setColour (mdn::lilac().withAlpha (0.8f));
    g.drawText (TR ("MADENCİ KAYDI"), 28, 86, 230, 18, Justification::centredLeft);
    g.drawText ("PITCH MONITOR", 298, 86, 230, 18, Justification::centredLeft);
    g.drawText ("CORRECTION", 828, 86, 230, 18, Justification::centredLeft);
    g.drawText (TR ("NOTES   ·   tuşlara tıkla: nota aç / kapat"), 28, 486, 600, 18, Justification::centredLeft);
    g.drawText ("RANGE", 28, 628, 100, 18, Justification::centredLeft);
    g.drawText ("SCALE", 574, 628, 100, 18, Justification::centredLeft);
    g.drawText ("KEY", 714, 628, 60, 18, Justification::centredLeft);
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
    const Rectangle<float> card (26.f, 106.f, 236.f, 172.f);
    Path clip; clip.addRoundedRectangle (card, 12.f);
    DropShadow (Colours::black.withAlpha (0.7f), 14, { 0, 5 }).drawForPath (g, clip);
    g.saveState();
    g.reduceClipRegion (clip, {});
    g.setColour (Colours::black); g.fillRect (card);
    if (photo1.isValid()) g.drawImage (photo1, card.withTrimmedBottom (24.f), RectanglePlacement::fillDestination);
    g.setGradientFill (ColourGradient (mdn::violet().withAlpha (0.40f), card.getX(), card.getY(), Colours::transparentBlack, card.getX(), card.getBottom(), false));
    g.fillRect (card);
    g.setColour (Colours::black.withAlpha (0.14f));
    for (float y = card.getY(); y < card.getBottom(); y += 3.f) g.fillRect (card.getX(), y, card.getWidth(), 1.f);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.18f), card.getX(), card.getY(), Colours::transparentWhite, card.getX(), card.getY() + 60.f, false));
    g.fillRect (card.withHeight (60.f));
    const auto cap = card.withTrimmedTop (card.getHeight() - 24.f);
    g.setGradientFill (ColourGradient (Colour (0xff1a0d36), cap.getX(), cap.getY(), Colour (0xff0a0518), cap.getX(), cap.getBottom(), false));
    g.fillRect (cap);
    g.setColour (mdn::pink()); g.setFont (mdn::font (11.5f, Font::bold));
    g.drawText (TR ("MADENCİ 01  ·  VARDİYADA"), cap, Justification::centred);
    g.restoreState();
    // koseler
    g.setColour (mdn::pink().withAlpha (0.9f));
    const float L = 12.f; const auto r = card.reduced (6.f);
    g.drawLine (r.getX(), r.getY(), r.getX() + L, r.getY(), 1.6f); g.drawLine (r.getX(), r.getY(), r.getX(), r.getY() + L, 1.6f);
    g.drawLine (r.getRight(), r.getY(), r.getRight() - L, r.getY(), 1.6f); g.drawLine (r.getRight(), r.getY(), r.getRight(), r.getY() + L, 1.6f);
    g.setGradientFill (ColourGradient (mdn::purple(), card.getX(), card.getY(), mdn::pink(), card.getRight(), card.getBottom(), false));
    g.drawRoundedRectangle (card, 12.f, 1.6f);
}

void MadenciEditor::paintMeters (Graphics& g)
{
    auto bar = [&] (float y, const String& label, float v, bool hot)
    {
        g.setColour (mdn::lilac()); g.setFont (mdn::font (10.5f, Font::bold));
        g.drawText (label, 26, (int) y - 4, 46, 18, Justification::centredLeft);
        const Rectangle<float> r (76.f, y, 186.f, 10.f);
        g.setGradientFill (ColourGradient (Colour (0xff05030a), 0.f, r.getY(), Colour (0xff231a3d), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.f);
        const float w = jlimit (0.f, 1.f, v) * r.getWidth();
        if (w > 1.f)
        {
            g.setGradientFill (ColourGradient (hot ? Colour (0xffff4fd8) : mdn::violet(), r.getX(), 0.f, hot ? Colour (0xffffb0f0) : mdn::pink(), r.getRight(), 0.f, false));
            g.fillRoundedRectangle (r.withWidth (w), 5.f);
        }
        g.setColour (Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (r, 5.f, 1.f);
    };
    bar (298.f, "IN", std::sqrt (dispIn), false);
    bar (320.f, "OUT", std::sqrt (dispOut), false);
    bar (342.f, "ROBOT", dispRobot, true);
}

void MadenciEditor::paintOrb (Graphics& g)
{
    const auto c = orbC;
    const float pr = 108.f;      // foto yaricapi
    const bool mm = madenciOn();
    g.saveState();
    g.reduceClipRegion (Rectangle<int> (284, 80, 520, 390));

    const float pulse = 0.5f + 0.5f * std::sin (phase * 1.5f);
    g.setGradientFill (ColourGradient ((mm ? mdn::pink() : mdn::purple()).withAlpha (0.16f + 0.30f * dispIn + (mm ? 0.10f * pulse : 0.f)),
                                       c.x, c.y, Colours::transparentBlack, c.x + 250.f, c.y, true));
    g.fillEllipse (c.x - 250.f, c.y - 250.f, 500.f, 500.f);

    // osiloskop halkasi (sol-sag simetrik, en yeni ses ustte)
    {
        Path sp; const int n = 128; const float base = pr + 26.f;
        for (int i = 0; i < n; ++i)
        {
            const int j = i < 64 ? i : 127 - i;
            const float a = (float) i / (float) n * MathConstants<float>::twoPi;
            const float rr = base + jmin (1.f, scopeDisp[(size_t) j] * 3.f) * 26.f;
            const auto p = mdn::polar (c, a, rr);
            if (i == 0) sp.startNewSubPath (p); else sp.lineTo (p);
        }
        sp.closeSubPath();
        g.setColour (mdn::pink().withAlpha (0.16f)); g.strokePath (sp, PathStrokeType (7.f, PathStrokeType::curved));
        g.setGradientFill (ColourGradient (mdn::pink(), c.x, c.y - base, mdn::violet(), c.x, c.y + base, false));
        g.strokePath (sp, PathStrokeType (2.2f, PathStrokeType::curved));
        g.setColour (mdn::lilac().withAlpha (0.18f)); g.drawEllipse (c.x - base, c.y - base, base * 2.f, base * 2.f, 1.f);
    }

    // krom cerceve
    {
        Path ring; ring.addEllipse (c.x - pr - 10.f, c.y - pr - 10.f, 2.f * (pr + 10.f), 2.f * (pr + 10.f));
        ring.addEllipse (c.x - pr, c.y - pr, 2.f * pr, 2.f * pr);
        ring.setUsingNonZeroWinding (false);
        Path outer; outer.addEllipse (c.x - pr - 10.f, c.y - pr - 10.f, 2.f * (pr + 10.f), 2.f * (pr + 10.f));
        DropShadow (Colours::black.withAlpha (0.8f), 18, { 0, 6 }).drawForPath (g, outer);
        g.setGradientFill (ColourGradient (Colour (0xffcfc6ee), c.x - pr, c.y - pr, Colour (0xff1a1230), c.x + pr, c.y + pr, false));
        g.fillPath (ring);
        g.setColour (Colours::black.withAlpha (0.5f)); g.drawEllipse (c.x - pr, c.y - pr, 2.f * pr, 2.f * pr, 1.5f);
    }

    // foto (robotluk arttikca blur)
    {
        Path cp; cp.addEllipse (c.x - pr, c.y - pr, 2.f * pr, 2.f * pr);
        g.saveState();
        g.reduceClipRegion (cp, {});
        g.setColour (Colours::black); g.fillEllipse (c.x - pr, c.y - pr, 2.f * pr, 2.f * pr);
        const float lv = jlimit (0.f, 1.f, dispRobot) * 7.f;
        const int lo = (int) std::floor (lv), hi = jmin (7, lo + 1);
        const float fr = lv - (float) lo;
        const Rectangle<float> dest (c.x - pr, c.y - pr, 2.f * pr, 2.f * pr);
        g.setOpacity (1.f);
        if (blurLevels[(size_t) lo].isValid()) g.drawImage (blurLevels[(size_t) lo], dest, RectanglePlacement::stretchToFit);
        if (fr > 0.01f && blurLevels[(size_t) hi].isValid())
        {
            g.setOpacity (fr);
            g.drawImage (blurLevels[(size_t) hi], dest, RectanglePlacement::stretchToFit);
            g.setOpacity (1.f);
        }
        g.setGradientFill (ColourGradient (Colours::transparentBlack, c.x, c.y, mdn::violet().withAlpha (0.60f), c.x, c.y - pr, true));
        g.fillEllipse (dest);
        if (dispRobot > 0.02f)
        {
            g.setColour (mdn::pink().withAlpha (0.22f * dispRobot)); g.fillEllipse (dest);
            g.setColour (Colours::black.withAlpha (0.22f * dispRobot));
            const float off = std::fmod (phase * 30.f, 6.f);
            for (float y = dest.getY() + off; y < dest.getBottom(); y += 6.f) g.fillRect (dest.getX(), y, dest.getWidth(), 2.f);
        }
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.22f), c.x, c.y - pr, Colours::transparentWhite, c.x, c.y - pr * 0.3f, false));
        g.fillEllipse (c.x - pr * 0.82f, c.y - pr * 0.96f, pr * 1.64f, pr * 0.8f);
        g.restoreState();
    }

    // cent yayi
    {
        const float ar = pr + 66.f;
        for (int ct = -50; ct <= 50; ct += 10)
        {
            const float a = (float) ct / 50.f * 0.95f;
            g.setColour (ct == 0 ? Colours::white : mdn::lilac().withAlpha (0.5f));
            g.drawLine (Line<float> (mdn::polar (c, a, ar - (ct == 0 ? 10.f : 5.f)), mdn::polar (c, a, ar + 3.f)), ct == 0 ? 2.2f : 1.2f);
        }
        Path base; base.addCentredArc (c.x, c.y, ar - 14.f, ar - 14.f, 0.f, -0.95f, 0.95f, true);
        g.setColour (Colour (0xff261b40)); g.strokePath (base, PathStrokeType (4.f, PathStrokeType::curved, PathStrokeType::rounded));
        if (voiced)
        {
            const float a = jlimit (-50.f, 50.f, dispCents) / 50.f * 0.95f;
            Path arc; arc.addCentredArc (c.x, c.y, ar - 14.f, ar - 14.f, 0.f, jmin (0.f, a), jmax (0.f, a), true);
            g.setColour (mdn::pink().withAlpha (0.9f)); g.strokePath (arc, PathStrokeType (4.f));
            const auto n = mdn::polar (c, a, ar - 14.f);
            g.setColour (mdn::pink().withAlpha (0.35f)); g.fillEllipse (n.x - 10.f, n.y - 10.f, 20.f, 20.f);
            g.setColour (Colours::white); g.fillEllipse (n.x - 5.f, n.y - 5.f, 10.f, 10.f);
        }
    }

    // nota ekrani
    {
        static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const Rectangle<float> pill (c.x - 74.f, c.y + 62.f, 148.f, 76.f);
        Path pp; pp.addRoundedRectangle (pill, 16.f);
        DropShadow (Colours::black.withAlpha (0.8f), 14, { 0, 5 }).drawForPath (g, pp);
        g.setGradientFill (ColourGradient (Colour (0xff1a1030), pill.getX(), pill.getY(), Colour (0xff06030d), pill.getX(), pill.getBottom(), false));
        g.fillPath (pp);
        g.setGradientFill (ColourGradient (mdn::purple(), pill.getX(), pill.getY(), mdn::violet().withAlpha (0.4f), pill.getX(), pill.getBottom(), false));
        g.strokePath (pp, PathStrokeType (1.6f));
        g.setColour (Colours::white.withAlpha (0.08f)); g.drawLine (pill.getX() + 12.f, pill.getY() + 2.f, pill.getRight() - 12.f, pill.getY() + 2.f, 1.f);
        if (voiced)
        {
            const int n = roundToInt (tgtShown);
            const String nm = String (names[((n % 12) + 12) % 12]) + String (n / 12 - 1);
            const auto area = pill.withTrimmedBottom (22.f).toNearestInt();
            g.setFont (mdn::font (40.f, Font::bold));
            g.setColour (mdn::pink().withAlpha (0.30f));
            g.drawText (nm, area.translated (0, 2), Justification::centred);
            g.drawText (nm, area.translated (2, 0), Justification::centred);
            g.setColour (Colours::white); g.drawText (nm, area, Justification::centred);
            g.setFont (mdn::font (11.5f, Font::bold)); g.setColour (mdn::pink());
            g.drawText (String (dispCents >= 0 ? "+" : "") + String (roundToInt (dispCents)) + " cent", pill.withTrimmedTop (pill.getHeight() - 24.f).toNearestInt(), Justification::centred);
        }
        else
        {
            g.setFont (mdn::font (34.f, Font::bold)); g.setColour (mdn::lilac().withAlpha (0.55f));
            g.drawText ("--", pill.toNearestInt(), Justification::centred);
        }

        // yan okumalar
        g.setFont (mdn::font (10.5f, Font::bold)); g.setColour (mdn::lilac().withAlpha (0.65f));
        g.drawText ("DETECTED", 296, (int) c.y - 24, 90, 14, Justification::centredLeft);
        g.drawText ("TARGET", 718, (int) c.y - 24, 76, 14, Justification::centredRight);
        g.setFont (mdn::font (16.f, Font::bold)); g.setColour (Colours::white);
        g.drawText (voiced ? String (detHz, 1) + " Hz" : String ("--"), 296, (int) c.y - 8, 100, 22, Justification::centredLeft);
        g.drawText (voiced ? String (tgtHz, 1) + " Hz" : String ("--"), 698, (int) c.y - 8, 96, 22, Justification::centredRight);
        if (madenciOn())
        {
            g.setColour (mdn::pink().withAlpha (0.55f + 0.45f * pulse)); g.setFont (mdn::font (11.f, Font::bold | Font::italic));
            g.drawText ("MADENCI MODU", 296, 100, 140, 16, Justification::centredLeft);
        }
    }
    g.restoreState();
}

void MadenciEditor::timerCallback()
{
    const float det = proc.engine.detMidi.load(), tgt = proc.engine.tgtMidi.load();
    voiced = det > 0.f;
    const float ref = proc.apvts.getRawParameterValue ("reference")->load();
    const float li = jlimit (0.f, 1.f, proc.levelIn.load()), lo = jlimit (0.f, 1.f, proc.levelOut.load());
    dispIn += (li - dispIn) * (li > dispIn ? 0.6f : 0.15f);
    dispOut += (lo - dispOut) * (lo > dispOut ? 0.6f : 0.15f);
    phase += 0.03f + 0.10f * dispIn;

    if (voiced)
    {
        dispCents += (jlimit (-60.f, 60.f, (det - tgt) * 100.f) - dispCents) * 0.4f;
        tgtShown = tgt;
        activePc = ((roundToInt (tgt) % 12) + 12) % 12;
        detHz = ref * std::pow (2.f, (det - 69.f) / 12.f);
        tgtHz = ref * std::pow (2.f, (tgt - 69.f) / 12.f);
    }
    else { dispCents *= 0.8f; activePc = -1; }

    // osiloskop
    const int start = proc.scopePos.load();
    for (int j = 0; j < 64; ++j)
    {
        const float tg = proc.scope[(size_t) ((start - 1 - j * 2 + 256) & 127)].load();
        scopeDisp[(size_t) j] += (tg - scopeDisp[(size_t) j]) * 0.5f;
    }

    // robotluk -> foto blur
    const bool mm = madenciOn();
    const float sp = proc.apvts.getRawParameterValue ("speed")->load();
    const float fo = proc.apvts.getRawParameterValue ("formant")->load();
    const float kSpeed = mm ? jlimit (0.f, 1.f, -sp / 10.f) : 0.f;
    const float kForm = fo < -6.f ? (-fo - 6.f) / 6.f : 0.f;
    const float kTech = fo > 0.f ? fo / 12.f * 0.6f : 0.f;
    const float robotT = jmax (kSpeed, 0.85f * kForm, kTech);
    dispRobot += (robotT - dispRobot) * 0.12f;

    syncLock();
    keys.setActivePc (activePc);
    rangeStrip.refresh();
    repaint (284, 80, 520, 390);
    repaint (26, 290, 236, 60);
}
