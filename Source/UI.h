#pragma once
#include <JuceHeader.h>

#define TR(s) juce::String (juce::CharPointer_UTF8 (s))

namespace mdn
{
    inline juce::Font font (float h, int style = juce::Font::plain) { return juce::Font ("Segoe UI", h, style); }
    inline juce::Colour purple() { return juce::Colour (0xffa66bff); }
    inline juce::Colour violet() { return juce::Colour (0xff7c3aed); }
    inline juce::Colour pink()   { return juce::Colour (0xffff4fd8); }
    inline juce::Colour lilac()  { return juce::Colour (0xffd4c4ff); }

    inline juce::Point<float> polar (juce::Point<float> c, float a, float r)
    {
        return { c.x + r * std::sin (a), c.y - r * std::cos (a) };
    }
}

// ---- MADENCI KAZMASI -------------------------------------------------------
inline void drawPickaxe (juce::Graphics& g, juce::Point<float> c, float size, float angle, bool glow = true)
{
    using namespace juce;
    Path handle; handle.addRoundedRectangle (-0.06f, -0.30f, 0.12f, 1.08f, 0.05f);
    Path head;
    head.startNewSubPath (-0.92f, 0.10f);
    head.quadraticTo (-0.55f, -0.62f, 0.f, -0.58f);
    head.quadraticTo (0.55f, -0.62f, 0.92f, 0.10f);
    head.quadraticTo (0.50f, -0.40f, 0.f, -0.36f);
    head.quadraticTo (-0.50f, -0.40f, -0.92f, 0.10f);
    head.closeSubPath();
    Path band; band.addRoundedRectangle (-0.11f, -0.40f, 0.22f, 0.17f, 0.04f);
    Path shine;
    shine.startNewSubPath (-0.78f, 0.02f);
    shine.quadraticTo (-0.50f, -0.54f, 0.f, -0.52f);

    const auto tf = AffineTransform::scale (size).rotated (angle).translated (c.x, c.y);
    handle.applyTransform (tf); head.applyTransform (tf); band.applyTransform (tf); shine.applyTransform (tf);

    if (glow) { g.setColour (mdn::purple().withAlpha (0.30f)); g.strokePath (head, PathStrokeType (size * 0.16f)); }
    g.setGradientFill (ColourGradient (Colour (0xffcf9650), c.x - size * 0.1f, c.y, Colour (0xff4a2a10), c.x + size * 0.1f, c.y + size * 0.8f, false));
    g.fillPath (handle);
    g.setGradientFill (ColourGradient (Colour (0xfffaf3ff), c.x - size * 0.6f, c.y - size * 0.6f, Colour (0xff7c4df0), c.x + size * 0.5f, c.y + size * 0.1f, false));
    g.fillPath (head);
    g.setColour (Colours::black.withAlpha (0.45f)); g.strokePath (head, PathStrokeType (1.f));
    g.setColour (Colours::white.withAlpha (0.8f)); g.strokePath (shine, PathStrokeType (jmax (1.f, size * 0.035f)));
    g.setGradientFill (ColourGradient (Colour (0xff3b1d78), c.x, c.y - size * 0.4f, Colour (0xff170a33), c.x, c.y - size * 0.2f, false));
    g.fillPath (band);
}

// ---- PANEL -----------------------------------------------------------------
inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float cr = 14.f)
{
    using namespace juce;
    Path p; p.addRoundedRectangle (r, cr);
    DropShadow (Colours::black.withAlpha (0.6f), 20, { 0, 7 }).drawForPath (g, p);
    g.setGradientFill (ColourGradient (Colour (0xff1e1339), r.getX(), r.getY(), Colour (0xff0b0716), r.getX(), r.getBottom(), false));
    g.fillPath (p);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawLine (r.getX() + cr, r.getY() + 1.f, r.getRight() - cr, r.getY() + 1.f, 1.f);
    g.setGradientFill (ColourGradient (mdn::violet().withAlpha (0.7f), r.getX(), r.getY(), mdn::violet().withAlpha (0.12f), r.getX(), r.getBottom(), false));
    g.strokePath (p, PathStrokeType (1.2f));
}

// ---- LOOK & FEEL -----------------------------------------------------------
class MadenciLnF : public juce::LookAndFeel_V4
{
public:
    MadenciLnF()
    {
        using namespace juce;
        setColour (ComboBox::textColourId, Colours::white);
        setColour (PopupMenu::backgroundColourId, Colour (0xff140a26));
        setColour (PopupMenu::textColourId, Colours::white);
        setColour (PopupMenu::highlightedBackgroundColourId, mdn::violet());
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return mdn::font (14.f, juce::Font::bold); }
    juce::Font getPopupMenuFont() override { return mdn::font (14.f); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        using namespace juce;
        const auto r = Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        g.setGradientFill (ColourGradient (Colour (0xff2c1f4d), 0.f, 0.f, Colour (0xff120a24), 0.f, (float) h, false));
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (Colours::white.withAlpha (0.07f)); g.drawLine (8.f, 1.5f, (float) w - 8.f, 1.5f, 1.f);
        g.setColour (mdn::violet().withAlpha (box.isMouseOver() ? 1.f : 0.7f)); g.drawRoundedRectangle (r, 8.f, 1.2f);
        Path a; a.addTriangle ((float) w - 22.f, (float) h * 0.5f - 3.f, (float) w - 12.f, (float) h * 0.5f - 3.f, (float) w - 17.f, (float) h * 0.5f + 3.f);
        g.setColour (mdn::lilac()); g.fillPath (a);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        using namespace juce;
        const auto r = b.getLocalBounds().toFloat().reduced (3.f);
        const bool on = b.getToggleState();
        if (on)
        {
            g.setColour (mdn::pink().withAlpha (0.22f)); g.fillRoundedRectangle (r.expanded (3.f), 14.f);
        }
        g.setGradientFill (on ? ColourGradient (Colour (0xffd01fa0), r.getX(), r.getY(), Colour (0xff4a1a96), r.getX(), r.getBottom(), false)
                              : ColourGradient (Colour (0xff2c2145), r.getX(), r.getY(), Colour (0xff110a20), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 11.f);
        g.setColour (Colours::white.withAlpha (on ? 0.25f : 0.08f)); g.drawLine (r.getX() + 10.f, r.getY() + 1.5f, r.getRight() - 10.f, r.getY() + 1.5f, 1.f);
        g.setColour ((on ? mdn::pink() : mdn::violet()).withAlpha (hover ? 1.f : 0.8f)); g.drawRoundedRectangle (r, 11.f, 1.4f);

        drawPickaxe (g, { r.getX() + 24.f, r.getCentreY() + 2.f }, 13.f, 0.7f, on);
        g.setFont (mdn::font (14.f, Font::bold)); g.setColour (on ? Colours::white : mdn::lilac());
        g.drawText ("MADENCI MODU", r.withTrimmedLeft (46.f), Justification::centredLeft);

        const auto led = Rectangle<float> (r.getRight() - 62.f, r.getCentreY() - 5.f, 10.f, 10.f);
        if (on) { g.setColour (Colour (0xff7dffb0).withAlpha (0.35f)); g.fillEllipse (led.expanded (4.f)); }
        g.setColour (on ? Colour (0xff7dffb0) : Colour (0xff3a3150)); g.fillEllipse (led);
        g.setFont (mdn::font (11.5f, Font::bold)); g.setColour (on ? Colours::white : mdn::lilac().withAlpha (0.6f));
        g.drawText (on ? TR ("AÇIK") : TR ("KAPALI"), Rectangle<float> (led.getRight() + 5.f, r.getY(), 44.f, r.getHeight()), Justification::centredLeft);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        using namespace juce;
        const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
        const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const bool pick = (bool) s.getProperties().getWithDefault ("pickaxe", false);
        const bool locked = (bool) s.getProperties().getWithDefault ("locked", false);
        const float negFrac = (float) (double) s.getProperties().getWithDefault ("negFrac", 0.0);
        const float ang = a0 + pos * (a1 - a0);
        const float ra = r * 0.80f, rb = r * 0.64f, rc = r * 0.55f;

        // ortam isigi
        g.setGradientFill (ColourGradient (mdn::purple().withAlpha (0.10f + 0.30f * pos), c.x, c.y, Colours::transparentBlack, c.x, c.y - (r + 3.f), true));
        g.fillEllipse (c.x - r - 3.f, c.y - r - 3.f, 2.f * r + 6.f, 2.f * r + 6.f);

        // dis cetvel (tick)
        for (int i = 0; i <= 20; ++i)
        {
            const float f = (float) i / 20.f, a = a0 + f * (a1 - a0);
            const bool lit = f <= pos + 0.001f, inNeg = f < negFrac - 0.001f;
            Colour col = inNeg ? (locked ? Colour (0xff4a1a3a) : mdn::pink().withAlpha (lit ? 1.f : 0.5f))
                               : mdn::lilac().withAlpha (lit ? 0.95f : 0.28f);
            g.setColour (col);
            g.drawLine (Line<float> (mdn::polar (c, a, r * (i % 5 == 0 ? 0.90f : 0.94f)), mdn::polar (c, a, r)), i % 5 == 0 ? 1.8f : 1.1f);
        }

        // kuyu
        g.setGradientFill (ColourGradient (Colour (0xff05030a), c.x, c.y - ra, Colour (0xff1a1230), c.x, c.y + ra, false));
        g.fillEllipse (c.x - ra - 4.f, c.y - ra - 4.f, 2.f * ra + 8.f, 2.f * ra + 8.f);

        // yay
        Path track; track.addCentredArc (c.x, c.y, ra, ra, 0.f, a0, a1, true);
        g.setColour (Colour (0xff261b40)); g.strokePath (track, PathStrokeType (5.f, PathStrokeType::curved, PathStrokeType::rounded));
        if (negFrac > 0.f)
        {
            Path neg; neg.addCentredArc (c.x, c.y, ra, ra, 0.f, a0, a0 + negFrac * (a1 - a0), true);
            if (locked)
            {
                Path dashed; const float dl[] = { 2.5f, 4.f };
                PathStrokeType (5.f).createDashedStroke (dashed, neg, dl, 2);
                g.setColour (Colour (0xff6a2457)); g.fillPath (dashed);
            }
            else { g.setColour (mdn::pink().withAlpha (0.28f)); g.strokePath (neg, PathStrokeType (5.f)); }
        }
        Path val; val.addCentredArc (c.x, c.y, ra, ra, 0.f, a0, jmax (a0 + 0.001f, ang), true);
        g.setColour (mdn::pink().withAlpha (0.16f)); g.strokePath (val, PathStrokeType (13.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (mdn::violet().withAlpha (0.35f)); g.strokePath (val, PathStrokeType (9.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill (ColourGradient (mdn::violet(), c.x - ra, c.y, mdn::pink(), c.x + ra, c.y, false));
        g.strokePath (val, PathStrokeType (4.5f, PathStrokeType::curved, PathStrokeType::rounded));

        // govde golgesi + metal cerceve
        Path cp; cp.addEllipse (c.x - rb, c.y - rb, 2.f * rb, 2.f * rb);
        DropShadow (Colours::black.withAlpha (0.8f), (int) (r * 0.32f), { 0, (int) (r * 0.10f) }).drawForPath (g, cp);
        g.setGradientFill (ColourGradient (Colour (0xff8a82a8), c.x - rb * 0.7f, c.y - rb * 0.8f, Colour (0xff0e0a18), c.x + rb * 0.6f, c.y + rb * 0.9f, false));
        g.fillEllipse (c.x - rb, c.y - rb, 2.f * rb, 2.f * rb);
        g.setGradientFill (ColourGradient (Colour (0xff120c22), c.x - rb * 0.5f, c.y - rb * 0.6f, Colour (0xff3a2f58), c.x + rb * 0.5f, c.y + rb * 0.7f, false));
        g.fillEllipse (c.x - rb * 0.93f, c.y - rb * 0.93f, rb * 1.86f, rb * 1.86f);

        // kapak
        g.setGradientFill (ColourGradient (Colour (0xff5a4590), c.x - rc * 0.4f, c.y - rc * 0.5f, Colour (0xff140d26), c.x + rc * 0.9f, c.y + rc * 1.1f, true));
        g.fillEllipse (c.x - rc, c.y - rc, 2.f * rc, 2.f * rc);
        for (int i = 0; i < 40; ++i)
        {
            const float a = ang + (float) i * MathConstants<float>::twoPi / 40.f;
            g.setColour (i % 2 ? Colours::white.withAlpha (0.13f) : Colours::black.withAlpha (0.35f));
            g.drawLine (Line<float> (mdn::polar (c, a, rc * 0.88f), mdn::polar (c, a, rc * 0.985f)), 1.f);
        }
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.20f), c.x, c.y - rc, Colours::transparentWhite, c.x, c.y, false));
        g.fillEllipse (c.x - rc * 0.8f, c.y - rc * 0.97f, rc * 1.6f, rc * 0.9f);
        g.setColour (Colours::white.withAlpha (0.28f)); g.drawEllipse (c.x - rc, c.y - rc, 2.f * rc, 2.f * rc, 1.f);

        // gosterge
        if (pick) drawPickaxe (g, c, rb * 0.98f, ang, true);
        else
        {
            const auto p1 = mdn::polar (c, ang, rc * 0.30f), p2 = mdn::polar (c, ang, rc * 0.86f);
            g.setColour (mdn::purple().withAlpha (0.45f)); g.drawLine (Line<float> (p1, p2), 6.f);
            g.setColour (Colours::white); g.drawLine (Line<float> (p1, p2), 2.5f);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override
    {
        using namespace juce;
        const float ty = (float) y + (float) h * 0.5f;
        const Rectangle<float> track ((float) x + 10.f, ty - 5.f, (float) w - 20.f, 10.f);
        g.setGradientFill (ColourGradient (Colour (0xff05030a), 0.f, track.getY(), Colour (0xff231a3d), 0.f, track.getBottom(), false));
        g.fillRoundedRectangle (track, 5.f);
        g.setColour (Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (track, 5.f, 1.f);
        for (int i = 0; i <= 4; ++i)
        {
            g.setColour (mdn::lilac().withAlpha (i % 2 == 0 ? 0.5f : 0.25f));
            g.fillRect (track.getX() + track.getWidth() * (float) i * 0.25f - 0.5f, ty + 8.f, 1.f, i % 2 == 0 ? 6.f : 4.f);
        }
        const auto fill = track.withRight (jmax (track.getX() + 2.f, pos));
        g.setColour (mdn::pink().withAlpha (0.18f)); g.fillRoundedRectangle (fill.expanded (0.f, 3.f), 7.f);
        g.setGradientFill (ColourGradient (Colour (0xff6d28d9), track.getX(), 0.f, Colour (0xffe879f9), track.getRight(), 0.f, false));
        g.fillRoundedRectangle (fill, 5.f);
        g.setColour (Colours::white.withAlpha (0.25f)); g.fillRoundedRectangle (fill.withHeight (3.f).translated (0.f, 1.f), 1.5f);
        drawPickaxe (g, { pos, ty }, 15.f, 0.55f, true);
    }
};

// ---- KNOB KUTUSU -----------------------------------------------------------
class KnobBox : public juce::Component
{
public:
    KnobBox (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
             std::function<juce::String (double)> fmt, bool pickaxe = false, bool linear = false,
             std::function<juce::String (double)> captionFn = nullptr, const juce::String& side = {}, float negFrac = 0.f)
        : name (title), format (std::move (fmt)), caption (std::move (captionFn)), sideNote (side), isLinear (linear)
    {
        slider.setSliderStyle (linear ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        slider.getProperties().set ("pickaxe", pickaxe);
        slider.getProperties().set ("negFrac", (double) negFrac);
        slider.getProperties().set ("locked", false);
        slider.setMouseDragSensitivity (220);
        addAndMakeVisible (slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
        slider.onValueChange = [this] { repaint(); };
    }

    juce::Slider& getSlider() { return slider; }

    void setLocked (bool l)
    {
        if ((bool) slider.getProperties().getWithDefault ("locked", false) != l)
        {
            slider.getProperties().set ("locked", l);
            slider.repaint(); repaint();
        }
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop (18);
        if (isLinear) { slider.setBounds (b.removeFromTop (30)); return; }
        if (caption) b.removeFromBottom (16);
        b.removeFromBottom (24);
        slider.setBounds (b);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const double v = slider.getValue();
        auto tf = mdn::font (12.f, Font::bold); tf.setExtraKerningFactor (0.08f);
        g.setFont (tf); g.setColour (mdn::lilac());
        const auto head = getLocalBounds().removeFromTop (18);
        if (isLinear)
        {
            g.drawText (name, head, Justification::centredLeft);
            if (sideNote.isNotEmpty())
            {
                const int nameW = (int) tf.getStringWidthFloat (name) + 10;
                g.setFont (mdn::font (12.5f, Font::bold | Font::italic)); g.setColour (mdn::pink());
                g.drawText (sideNote, head.withTrimmedLeft (nameW), Justification::centredLeft);
            }
            g.setFont (mdn::font (12.5f, Font::bold)); g.setColour (Colours::white);
            g.drawText (format (v), head, Justification::centredRight);
            return;
        }
        g.drawText (name, head, Justification::centred);

        const int capH = caption ? 16 : 0;
        const String vt = format (v);
        const auto vf = mdn::font (14.f, Font::bold);
        const float tw = vf.getStringWidthFloat (vt) + 22.f;
        const Rectangle<float> well (((float) getWidth() - tw) * 0.5f, (float) (getHeight() - capH - 22), tw, 20.f);
        g.setColour (Colour (0xff07040f)); g.fillRoundedRectangle (well, 6.f);
        g.setColour (mdn::violet().withAlpha (0.55f)); g.drawRoundedRectangle (well, 6.f, 1.f);
        g.setFont (vf); g.setColour (Colours::white);
        g.drawText (vt, well, Justification::centred);
        if (caption)
        {
            const String c = caption (v);
            const bool hot = c.startsWith ("SERT") || c.endsWith ("!!!");
            g.setColour (hot ? mdn::pink() : mdn::purple());
            g.setFont (mdn::font (12.f, Font::bold | Font::italic));
            g.drawText (c, 0, getHeight() - 16, getWidth(), 16, Justification::centred);
        }
    }

private:
    juce::String name;
    std::function<juce::String (double)> format, caption;
    juce::String sideNote;
    bool isLinear;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

// ---- NOTA KLAVYESI ---------------------------------------------------------
class NoteKeyboard : public juce::Component
{
public:
    explicit NoteKeyboard (juce::AudioProcessorValueTreeState& a) : apvts (a) {}
    std::function<void()> onUserEdit;

    void setActivePc (int pc) { if (pc != activePc) { activePc = pc; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const float ww = (float) getWidth() / 14.f, bw = ww * 0.62f, bh = (float) getHeight() * 0.62f, H = (float) getHeight();
        static const int whitePc[7] = { 0, 2, 4, 5, 7, 9, 11 };
        static const char* nm[7] = { "C", "D", "E", "F", "G", "A", "B" };
        for (int i = 0; i < 14; ++i)
        {
            const int pc = whitePc[i % 7];
            const Rectangle<float> k ((float) i * ww + 1.5f, 0.f, ww - 3.f, H - 2.f);
            const bool act = pc == activePc, on = isOn (pc);
            if (act) { g.setColour (mdn::pink().withAlpha (0.28f)); g.fillRoundedRectangle (k.expanded (3.f, 3.f), 7.f); }
            if (act)      g.setGradientFill (ColourGradient (Colour (0xffff9be8), 0.f, 0.f, Colour (0xffc01fb0), 0.f, H, false));
            else if (on)  g.setGradientFill (ColourGradient (Colour (0xffe6d8ff), 0.f, 0.f, Colour (0xff7c3aed), 0.f, H, false));
            else          g.setGradientFill (ColourGradient (Colour (0xff3a3646), 0.f, 0.f, Colour (0xff17141f), 0.f, H, false));
            g.fillRoundedRectangle (k, 5.f);
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (on || act ? 0.28f : 0.08f), 0.f, 0.f, Colours::transparentWhite, 0.f, H * 0.45f, false));
            g.fillRoundedRectangle (k.withHeight (H * 0.45f), 5.f);
            g.setColour (Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (k, 5.f, 1.f);
            g.setColour ((on || act) ? Colours::black.withAlpha (0.55f) : Colours::white.withAlpha (0.35f));
            g.setFont (mdn::font (11.f, Font::bold));
            g.drawText (String (nm[i % 7]) + (pc == 0 ? String (3 + i / 7) : String()), k.withTrimmedBottom (7.f), Justification::centredBottom);
        }
        static const int blackAfter[5] = { 0, 1, 3, 4, 5 }, blackPc[5] = { 1, 3, 6, 8, 10 };
        for (int o = 0; o < 2; ++o)
            for (int j = 0; j < 5; ++j)
            {
                const int pc = blackPc[j];
                const Rectangle<float> k (((float) (o * 7 + blackAfter[j] + 1)) * ww - bw * 0.5f, 0.f, bw, bh);
                const bool act = pc == activePc, on = isOn (pc);
                Path kp; kp.addRoundedRectangle (k, 3.f);
                DropShadow (Colours::black.withAlpha (0.6f), 7, { 0, 3 }).drawForPath (g, kp);
                if (act)      g.setGradientFill (ColourGradient (Colour (0xffff62dc), 0.f, 0.f, Colour (0xff7a1a8a), 0.f, bh, false));
                else if (on)  g.setGradientFill (ColourGradient (Colour (0xff7a3df0), 0.f, 0.f, Colour (0xff25105a), 0.f, bh, false));
                else          g.setGradientFill (ColourGradient (Colour (0xff1c1925), 0.f, 0.f, Colour (0xff050409), 0.f, bh, false));
                g.fillPath (kp);
                g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.22f), 0.f, 0.f, Colours::transparentWhite, 0.f, bh * 0.35f, false));
                g.fillRoundedRectangle (k.reduced (2.f, 0.f).withHeight (bh * 0.35f), 3.f);
                g.setColour (Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (k, 3.f, 1.f);
            }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int pc = pcAt (e.position);
        if (pc < 0) return;
        if (auto* prm = apvts.getParameter ("n" + juce::String (pc)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (isOn (pc) ? 0.f : 1.f);
            prm->endChangeGesture();
        }
        repaint();
        if (onUserEdit) onUserEdit();
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    int activePc = -1;

    bool isOn (int pc) const { return apvts.getRawParameterValue ("n" + juce::String (pc))->load() > 0.5f; }

    int pcAt (juce::Point<float> p) const
    {
        const float ww = (float) getWidth() / 14.f, bw = ww * 0.62f, bh = (float) getHeight() * 0.62f;
        static const int whitePc[7] = { 0, 2, 4, 5, 7, 9, 11 }, blackAfter[5] = { 0, 1, 3, 4, 5 }, blackPc[5] = { 1, 3, 6, 8, 10 };
        if (p.y < bh)
            for (int o = 0; o < 2; ++o)
                for (int j = 0; j < 5; ++j)
                {
                    const float bx = ((float) (o * 7 + blackAfter[j] + 1)) * ww - bw * 0.5f;
                    if (p.x >= bx && p.x <= bx + bw) return blackPc[j];
                }
        const int i = juce::jlimit (0, 13, (int) (p.x / ww));
        return whitePc[i % 7];
    }
};

// ---- RANGE SECIM SERIDI ----------------------------------------------------
class ChoiceStrip : public juce::Component
{
public:
    ChoiceStrip (juce::AudioProcessorValueTreeState& a, const juce::String& id, juce::StringArray items)
        : apvts (a), paramId (id), names (std::move (items)) {}

    void refresh()
    {
        const int i = (int) apvts.getRawParameterValue (paramId)->load();
        if (i != shown) { shown = i; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const int n = names.size();
        const float w = (float) getWidth() / (float) n, H = (float) getHeight();
        const int cur = (int) apvts.getRawParameterValue (paramId)->load();
        for (int i = 0; i < n; ++i)
        {
            const Rectangle<float> r ((float) i * w + 2.f, 0.f, w - 4.f, H);
            if (i == cur)
            {
                g.setColour (mdn::pink().withAlpha (0.20f)); g.fillRoundedRectangle (r.expanded (2.f), 9.f);
                g.setGradientFill (ColourGradient (Colour (0xff9b5cff), r.getX(), r.getY(), Colour (0xffc026d3), r.getRight(), r.getBottom(), false));
            }
            else g.setGradientFill (ColourGradient (Colour (0xff2a1d48), r.getX(), r.getY(), Colour (0xff120a24), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 7.f);
            g.setColour (Colours::white.withAlpha (i == cur ? 0.30f : 0.07f)); g.drawLine (r.getX() + 7.f, r.getY() + 1.5f, r.getRight() - 7.f, r.getY() + 1.5f, 1.f);
            g.setColour (mdn::violet().withAlpha (0.75f)); g.drawRoundedRectangle (r, 7.f, 1.f);
            g.setColour (i == cur ? Colours::white : mdn::lilac());
            g.setFont (mdn::font (12.f, Font::bold));
            g.drawText (names[i], r, Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, names.size() - 1, (int) (e.position.x / ((float) getWidth() / (float) names.size())));
        if (auto* prm = apvts.getParameter (paramId))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
            prm->endChangeGesture();
        }
        repaint();
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String paramId;
    juce::StringArray names;
    int shown = -1;
};
