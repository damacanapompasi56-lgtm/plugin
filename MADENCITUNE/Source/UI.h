#pragma once
#include <JuceHeader.h>

#define TR(s) juce::String (juce::CharPointer_UTF8 (s))

namespace mdn
{
    inline juce::Font font (float h, int style = juce::Font::plain) { return juce::Font ("Segoe UI", h, style); }
    inline juce::Colour purple()  { return juce::Colour (0xff9b5cff); }
    inline juce::Colour violet()  { return juce::Colour (0xff7c3aed); }
    inline juce::Colour pink()    { return juce::Colour (0xffff4fd8); }
    inline juce::Colour lilac()   { return juce::Colour (0xffc9b6ff); }
}

// ---- MADENCI KAZMASI -------------------------------------------------------
inline void drawPickaxe (juce::Graphics& g, juce::Point<float> c, float size, float angle, bool glow = true)
{
    using namespace juce;
    Path handle; handle.addRoundedRectangle (-0.055f, -0.30f, 0.11f, 1.05f, 0.05f);
    Path head;
    head.startNewSubPath (-0.92f, 0.10f);
    head.quadraticTo (-0.55f, -0.62f, 0.f, -0.58f);
    head.quadraticTo (0.55f, -0.62f, 0.92f, 0.10f);
    head.quadraticTo (0.50f, -0.40f, 0.f, -0.36f);
    head.quadraticTo (-0.50f, -0.40f, -0.92f, 0.10f);
    head.closeSubPath();
    Path band; band.addRoundedRectangle (-0.10f, -0.40f, 0.20f, 0.16f, 0.04f);

    const auto tf = AffineTransform::scale (size).rotated (angle).translated (c.x, c.y);
    handle.applyTransform (tf); head.applyTransform (tf); band.applyTransform (tf);

    if (glow) { g.setColour (mdn::purple().withAlpha (0.28f)); g.strokePath (head, PathStrokeType (size * 0.14f)); }
    g.setGradientFill (ColourGradient (Colour (0xffa0702f), c.x, c.y, Colour (0xff4a2a10), c.x, c.y + size * 0.8f, false));
    g.fillPath (handle);
    g.setGradientFill (ColourGradient (Colour (0xfff3e9ff), c.x - size * 0.5f, c.y - size * 0.6f, Colour (0xff8b5cf6), c.x + size * 0.5f, c.y, false));
    g.fillPath (head);
    g.setColour (Colours::white.withAlpha (0.55f)); g.strokePath (head, PathStrokeType (1.f));
    g.setColour (Colour (0xff2a1450)); g.fillPath (band);
}

// ---- LOOK & FEEL -----------------------------------------------------------
class MadenciLnF : public juce::LookAndFeel_V4
{
public:
    MadenciLnF()
    {
        using namespace juce;
        setColour (ComboBox::backgroundColourId, Colour (0xff1b1030));
        setColour (ComboBox::textColourId, Colours::white);
        setColour (ComboBox::outlineColourId, mdn::violet());
        setColour (ComboBox::arrowColourId, mdn::lilac());
        setColour (PopupMenu::backgroundColourId, Colour (0xff140a26));
        setColour (PopupMenu::textColourId, Colours::white);
        setColour (PopupMenu::highlightedBackgroundColourId, mdn::violet());
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return mdn::font (14.f, juce::Font::bold); }
    juce::Font getPopupMenuFont() override { return mdn::font (14.f); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        using namespace juce;
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
        const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const bool pick = (bool) s.getProperties().getWithDefault ("pickaxe", false);

        g.setGradientFill (ColourGradient (mdn::purple().withAlpha (0.35f), c.x, c.y, Colours::transparentBlack, c.x, c.y - r * 1.4f, true));
        g.fillEllipse (c.x - r * 1.4f, c.y - r * 1.4f, r * 2.8f, r * 2.8f);

        const float ang = a0 + pos * (a1 - a0);
        Path track; track.addCentredArc (c.x, c.y, r, r, 0.f, a0, a1, true);
        g.setColour (Colour (0xff2a2040)); g.strokePath (track, PathStrokeType (5.f, PathStrokeType::curved, PathStrokeType::rounded));
        Path val; val.addCentredArc (c.x, c.y, r, r, 0.f, a0, ang, true);
        g.setGradientFill (ColourGradient (mdn::violet(), c.x - r, c.y, mdn::pink(), c.x + r, c.y, false));
        g.strokePath (val, PathStrokeType (5.f, PathStrokeType::curved, PathStrokeType::rounded));

        const float br = r * 0.78f;
        g.setGradientFill (ColourGradient (Colour (0xff3c2b60), c.x - br * 0.5f, c.y - br * 0.7f, Colour (0xff0c0916), c.x + br * 0.6f, c.y + br, false));
        g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
        g.setColour (mdn::purple().withAlpha (0.6f)); g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.5f);

        if (pick) drawPickaxe (g, c, br * 1.15f, ang, true);
        else
        {
            Path ind; ind.addRoundedRectangle (-2.f, -br + 4.f, 4.f, br * 0.45f, 2.f);
            g.setColour (mdn::lilac());
            g.fillPath (ind, AffineTransform::rotation (ang).translated (c.x, c.y));
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override
    {
        using namespace juce;
        const float ty = (float) y + (float) h * 0.5f;
        Rectangle<float> track ((float) x + 8.f, ty - 4.f, (float) w - 16.f, 8.f);
        g.setColour (Colour (0xff241a3a)); g.fillRoundedRectangle (track, 4.f);
        g.setColour (mdn::lilac().withAlpha (0.35f));
        for (int i = 0; i <= 2; ++i) g.fillRect (track.getX() + track.getWidth() * (float) i * 0.5f - 0.5f, ty + 7.f, 1.f, 5.f);
        auto fill = track.withRight (pos);
        g.setGradientFill (ColourGradient (Colour (0xff6d28d9), track.getX(), 0.f, Colour (0xffe879f9), track.getRight(), 0.f, false));
        g.fillRoundedRectangle (fill, 4.f);
        drawPickaxe (g, { pos, ty }, 13.f, 0.55f, true);
    }
};

// ---- KNOB KUTUSU -----------------------------------------------------------
class KnobBox : public juce::Component
{
public:
    KnobBox (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
             std::function<juce::String (double)> fmt, bool pickaxe = false, bool linear = false,
             std::function<juce::String (double)> captionFn = nullptr, const juce::String& side = {})
        : name (title), format (std::move (fmt)), caption (std::move (captionFn)), sideNote (side), isLinear (linear)
    {
        slider.setSliderStyle (linear ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        slider.getProperties().set ("pickaxe", pickaxe);
        slider.setMouseDragSensitivity (220);
        addAndMakeVisible (slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
        slider.onValueChange = [this] { repaint(); };
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop (18);
        if (isLinear) { slider.setBounds (b.removeFromTop (28)); return; }
        if (caption) b.removeFromBottom (16);
        b.removeFromBottom (18);
        slider.setBounds (b);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const double v = slider.getValue();
        g.setFont (mdn::font (12.5f, Font::bold));
        g.setColour (mdn::lilac());
        const auto head = getLocalBounds().removeFromTop (18);
        if (isLinear)
        {
            g.drawText (name, head, Justification::centredLeft);
            if (sideNote.isNotEmpty())
            {
                const int nameW = (int) mdn::font (12.5f, Font::bold).getStringWidthFloat (name) + 8;
                g.setFont (mdn::font (12.5f, Font::bold | Font::italic)); g.setColour (mdn::pink());
                g.drawText (sideNote, head.withTrimmedLeft (nameW), Justification::centredLeft);
            }
            g.setFont (mdn::font (12.5f, Font::bold)); g.setColour (Colours::white);
            g.drawText (format (v), head, Justification::centredRight);
            return;
        }
        g.drawText (name, head, Justification::centred);
        const int capH = caption ? 16 : 0;
        g.setColour (Colours::white); g.setFont (mdn::font (14.f, Font::bold));
        g.drawText (format (v), 0, getHeight() - 18 - capH, getWidth(), 18, Justification::centred);
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
        for (int i = 0; i < 14; ++i)
        {
            const int pc = whitePc[i % 7];
            Rectangle<float> k ((float) i * ww + 1.f, 0.f, ww - 2.f, H);
            if (pc == activePc) g.setGradientFill (ColourGradient (mdn::pink(), 0.f, 0.f, Colour (0xff9d1fb0), 0.f, H, false));
            else if (isOn (pc)) g.setGradientFill (ColourGradient (Colour (0xffd9c6ff), 0.f, 0.f, Colour (0xff7c3aed), 0.f, H, false));
            else g.setGradientFill (ColourGradient (Colour (0xff35323f), 0.f, 0.f, Colour (0xff1c1a24), 0.f, H, false));
            g.fillRoundedRectangle (k, 4.f);
            if (pc == 0) { g.setColour (Colours::black.withAlpha (0.55f)); g.setFont (mdn::font (11.f, Font::bold)); g.drawText ("C" + String (3 + i / 7), k.withTrimmedBottom (6.f), Justification::centredBottom); }
        }
        static const int blackAfter[5] = { 0, 1, 3, 4, 5 }, blackPc[5] = { 1, 3, 6, 8, 10 };
        for (int o = 0; o < 2; ++o)
            for (int j = 0; j < 5; ++j)
            {
                const int pc = blackPc[j];
                Rectangle<float> k (((float) (o * 7 + blackAfter[j] + 1)) * ww - bw * 0.5f, 0.f, bw, bh);
                if (pc == activePc) g.setGradientFill (ColourGradient (mdn::pink(), 0.f, 0.f, Colour (0xff7a1a8a), 0.f, bh, false));
                else if (isOn (pc)) g.setGradientFill (ColourGradient (Colour (0xff6d28d9), 0.f, 0.f, Colour (0xff2a1060), 0.f, bh, false));
                else g.setGradientFill (ColourGradient (Colour (0xff17151d), 0.f, 0.f, Colour (0xff08070b), 0.f, bh, false));
                g.fillRoundedRectangle (k, 3.f);
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
        const float w = (float) getWidth() / (float) n;
        const int cur = (int) apvts.getRawParameterValue (paramId)->load();
        for (int i = 0; i < n; ++i)
        {
            Rectangle<float> r ((float) i * w + 2.f, 0.f, w - 4.f, (float) getHeight());
            if (i == cur)
            {
                g.setGradientFill (ColourGradient (mdn::violet(), r.getX(), r.getY(), Colour (0xffc026d3), r.getRight(), r.getBottom(), false));
                g.fillRoundedRectangle (r, 6.f);
            }
            else { g.setColour (Colour (0xff1b1030)); g.fillRoundedRectangle (r, 6.f); }
            g.setColour (mdn::violet().withAlpha (0.7f)); g.drawRoundedRectangle (r, 6.f, 1.f);
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
