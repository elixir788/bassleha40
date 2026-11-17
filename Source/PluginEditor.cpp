#include "PluginEditor.h"

static juce::String ru (const char* s) { return juce::String::fromUTF8 (s); }

static juce::Font mkFont (float size, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (size, style));
}

//==============================================================================
struct KnobInfo
{
    const char* id;
    const char* name;
    const char* suffix;
    int decimals;
    const char* desc;
};

static const KnobInfo kInfo[9] =
{
    { "cross", "Граница басса", " Гц", 0,
      "Это линия, которая делит звук на «низ» и «всё остальное». Всё, что ниже неё, плагин считает басом и улучшает. Всё, что выше, остаётся как было.\n\nДля 808 и бочки ставь около 80–120 Гц, для бас-гитары 150–200 Гц." },

    { "sub", "Глубина", " дБ", 1,
      "Просто делает самый низ громче или тише, как ручка «побольше баса». Плюс — бас тяжелее и глубже, минус — легче.\n\nНе переборщи: на мощных колонках слишком много низа начнёт гудеть и забивать всё остальное." },

    { "drive", "Плотность", " %", 0,
      "Добавляет басу «мясо». Звук слегка перегружается и становится плотнее и жирнее, но не разваливается.\n\nМало — бас просто становится плотнее. Много — появляется заметный злой, грязноватый характер." },

    { "harm", "Гармоники", " %", 0,
      "Добавляет к басу тонкие «верхние» оттенки. Очень глубокий бас на телефоне и маленьких колонках почти не слышно, а с гармониками он слышен даже там.\n\nКрути, пока бас не начнёт читаться на любой системе." },

    { "tight", "Сжатие", " %", 0,
      "Делает бас ровнее: слишком громкие удары чуть прижимаются, тихие подтягиваются. Бас перестаёт прыгать по громкости и звучит собранно и мощно.\n\nБольше — ровнее и плотнее, меньше — живее и свободнее." },

    { "clean", "Чистка низа", " Гц", 0,
      "Убирает совсем глубокий гул, который почти не слышно, но он забирает энергию и громкость трека. Всё, что ниже выбранной частоты, срезается.\n\nЧистый низ — микс громче и без «болтанки». Обычно хватает 25–35 Гц. Если бас стал слабее, убавь." },

    { "mono", "Моно-бас", " %", 0,
      "Делает самый низ одинаковым в левой и правой колонке, то есть строго по центру. Так бас получается собраннее и мощнее, и хорошо звучит в клубе и на любой системе.\n\n100% — весь бас по центру. 0% — как в оригинале." },

    { "mix", "Микс", " %", 0,
      "Сколько обработанного звука смешать с оригиналом. 100% — только обработка, 0% — как было без плагина.\n\nЕсли кажется, что перебор, убавь, чтобы найти баланс." },

    { "out", "Громкость", " дБ", 1,
      "Общая громкость на выходе после обработки. Если плагин сделал звук громче, убавь здесь, чтобы честно сравнивать «до» и «после»." }
};

//==============================================================================
void GlassLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float startAngle, float endAngle,
                                         juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const auto circle = juce::Rectangle<float> (d, d).withCentre (c);

    // свечение, зависит от значения
    {
        const float glow = 0.2f + 0.8f * sliderPos;
        juce::ColourGradient gg (accent.withAlpha (0.55f * glow), c.x, c.y,
                                 accent.withAlpha (0.0f), c.x, c.y - d * 0.85f, true);
        g.setGradientFill (gg);
        g.fillEllipse (circle.expanded (d * 0.35f));
    }

    // внешнее стеклянное кольцо
    {
        juce::ColourGradient rg (juce::Colours::white.withAlpha (0.55f), c.x, circle.getY(),
                                 accent.withAlpha (0.18f), c.x, circle.getBottom(), false);
        g.setGradientFill (rg);
        g.fillEllipse (circle);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawEllipse (circle.reduced (0.75f), 1.5f);
    }

    // дорожка и дуга значения
    const float r = d * 0.5f - 6.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    {
        juce::Path track;
        track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);
        g.setColour (accent.brighter (0.4f));
        g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // стеклянный шарик
    const auto inner = circle.reduced (d * 0.2f);
    {
        juce::ColourGradient og (juce::Colours::white.withAlpha (0.8f), c.x, inner.getY(),
                                 accent.darker (0.4f).withAlpha (0.65f), c.x, inner.getBottom(), false);
        g.setGradientFill (og);
        g.fillEllipse (inner);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawEllipse (inner, 1.5f);

        // блик
        auto hl = juce::Rectangle<float> (inner.getWidth() * 0.78f, inner.getHeight() * 0.42f)
                      .withCentre ({ c.x, inner.getY() + inner.getHeight() * 0.27f });
        juce::ColourGradient hg (juce::Colours::white.withAlpha (0.9f), c.x, hl.getY(),
                                 juce::Colours::white.withAlpha (0.0f), c.x, hl.getBottom(), false);
        g.setGradientFill (hg);
        g.fillEllipse (hl);
    }

    // указатель
    {
        const float ir = inner.getWidth() * 0.5f;
        juce::Point<float> p1 (c.x + ir * 0.35f * std::sin (angle), c.y - ir * 0.35f * std::cos (angle));
        juce::Point<float> p2 (c.x + ir * 0.82f * std::sin (angle), c.y - ir * 0.82f * std::cos (angle));
        g.setColour (juce::Colour (0xff0a3a55).withAlpha (0.85f));
        g.drawLine (juce::Line<float> (p1, p2), 3.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (p2));
    }
}

//==============================================================================
void InfoArrow::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.5f);
    const bool lit = hovered || pinned;

    juce::ColourGradient gr (juce::Colours::white.withAlpha (lit ? 0.9f : 0.6f), b.getCentreX(), b.getY(),
                             juce::Colour (0xff35d6ff).withAlpha (lit ? 0.7f : 0.3f), b.getCentreX(), b.getBottom(), false);
    g.setGradientFill (gr);
    g.fillEllipse (b);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawEllipse (b, 1.2f);

    juce::Path tri;
    const float cx = b.getCentreX() + 1.0f, cy = b.getCentreY();
    tri.addTriangle (cx - 3.5f, cy - 5.0f, cx - 3.5f, cy + 5.0f, cx + 4.5f, cy);
    g.setColour (juce::Colour (0xff0a3a55).withAlpha (0.9f));
    g.fillPath (tri);
}

//==============================================================================
BassEditor::BassEditor (BassProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setSize (960, 640);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setLookAndFeel (&laf);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k.slider);

        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kInfo[i].id, k.slider);

        const int idx = (int) i;
        k.arrow.onHover = [this, idx] (bool h)
        {
            if (h) hoverIdx = idx;
            else if (hoverIdx == idx) hoverIdx = -1;
            repaint();
        };
        k.arrow.onClick = [this, idx]
        {
            pinnedIdx = (pinnedIdx == idx) ? -1 : idx;
            for (size_t j = 0; j < knobs.size(); ++j)
                knobs[j].arrow.pinned = ((int) j == pinnedIdx);
            repaint();
        };
        addAndMakeVisible (k.arrow);
    }

    for (auto& b : bubbles)
    {
        b.x = rng.nextFloat() * 960.0f;
        b.y = rng.nextFloat() * 640.0f;
        b.r = 4.0f + rng.nextFloat() * 14.0f;
        b.speed = 0.3f + rng.nextFloat() * 0.9f;
        b.phase = rng.nextFloat() * 6.28f;
    }

    startTimerHz (30);
}

BassEditor::~BassEditor()
{
    stopTimer();
    for (auto& k : knobs)
        k.slider.setLookAndFeel (nullptr);
}

void BassEditor::resized()
{
    const int cellW = 180, cellH = 200;
    const int topY = 100;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        int row = i < 5 ? 0 : 1;
        int col = i < 5 ? (int) i : (int) i - 5;
        int rowW = (row == 0 ? 5 : 4) * cellW;
        int x0 = (getWidth() - rowW) / 2;

        auto& k = knobs[i];
        k.cell = { x0 + col * cellW, topY + row * cellH, cellW, cellH };
        k.slider.setBounds (k.cell.getCentreX() - 58, k.cell.getY() + 8, 116, 116);
        k.arrow.setBounds (k.cell.getRight() - 40, k.cell.getY() + 10, 24, 24);
    }
}

void BassEditor::timerCallback()
{
    auto get = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };

    drive = get ("drive") / 100.0f;
    harm  = get ("harm")  / 100.0f;
    tight = get ("tight") / 100.0f;
    subN  = (get ("sub") + 12.0f) / 24.0f;

    lvl = juce::jmax (proc.level.load(), lvl * 0.88f);

    // интерфейс "дышит" вместе с крутилками
    hue = 0.52f + 0.09f * drive - 0.10f * harm + 0.03f * tight;
    laf.accent = juce::Colour::fromHSV (hue, 0.75f, 1.0f, 1.0f);

    wavePhase += 0.05f + lvl * 0.15f;

    const float activity = juce::jlimit (0.0f, 1.0f, 0.5f * drive + 0.3f * subN + 0.6f * lvl);
    for (auto& b : bubbles)
    {
        b.y -= b.speed * (0.5f + activity * 2.5f);
        b.phase += 0.03f;
        b.x += std::sin (b.phase) * 0.35f;
        if (b.y < -b.r)
        {
            b.y = (float) getHeight() + b.r;
            b.x = rng.nextFloat() * (float) getWidth();
        }
    }

    repaint();
}

//==============================================================================
void BassEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    const auto accent = laf.accent;

    // фон: небо/вода, цвет зависит от крутилок
    {
        const auto top = juce::Colour::fromHSV (hue, 0.45f, 0.98f, 1.0f);
        const auto bot = juce::Colour::fromHSV (hue + 0.04f, 0.9f, 0.45f + 0.25f * subN, 1.0f);
        g.setGradientFill (juce::ColourGradient (top, 0.0f, 0.0f, bot, 0.0f, h, false));
        g.fillAll();

        // мягкие световые пятна
        juce::ColourGradient s1 (juce::Colours::white.withAlpha (0.35f), w * 0.2f, 0.0f,
                                 juce::Colours::white.withAlpha (0.0f), w * 0.2f, h * 0.6f, true);
        g.setGradientFill (s1);
        g.fillEllipse (-w * 0.2f, -h * 0.4f, w * 0.9f, h * 0.9f);
    }

    // пузырьки
    {
        const int count = (int) (14 + 34 * juce::jlimit (0.0f, 1.0f, 0.5f * drive + 0.3f * subN + 0.6f * lvl));
        for (int i = 0; i < count && i < (int) bubbles.size(); ++i)
        {
            const auto& b = bubbles[(size_t) i];
            juce::Rectangle<float> r (b.r * 2.0f, b.r * 2.0f);
            r.setCentre (b.x, b.y);
            juce::ColourGradient bg (juce::Colours::white.withAlpha (0.05f), r.getCentreX(), r.getCentreY(),
                                     juce::Colours::white.withAlpha (0.45f), r.getRight(), r.getCentreY(), true);
            g.setGradientFill (bg);
            g.fillEllipse (r);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.drawEllipse (r, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillEllipse (juce::Rectangle<float> (b.r * 0.5f, b.r * 0.4f).withCentre ({ b.x - b.r * 0.35f, b.y - b.r * 0.4f }));
        }
    }

    // стеклянная панель
    {
        auto panel = juce::Rectangle<float> (14.0f, 92.0f, w - 28.0f, 430.0f);
        const float frost = 0.10f + 0.12f * tight;

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f + frost), 0.0f, panel.getY(),
                                                 juce::Colours::white.withAlpha (frost * 0.5f), 0.0f, panel.getBottom(), false));
        g.fillRoundedRectangle (panel, 24.0f);

        g.saveState();
        juce::Path clip;
        clip.addRoundedRectangle (panel, 24.0f);
        g.reduceClipRegion (clip);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.3f), 0.0f, panel.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), 0.0f, panel.getY() + 150.0f, false));
        g.fillEllipse (panel.getX() - 80.0f, panel.getY() - 190.0f, panel.getWidth() + 160.0f, 300.0f);
        g.restoreState();

        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawRoundedRectangle (panel, 24.0f, 1.6f);
    }

    // вода внизу: волна растёт с уровнем баса
    {
        const float baseY = h - 78.0f - lvl * 40.0f - drive * 10.0f;
        const float amp = 6.0f + lvl * 18.0f + drive * 8.0f;

        for (int layer = 0; layer < 2; ++layer)
        {
            juce::Path wave;
            wave.startNewSubPath (0.0f, h);
            for (float x = 0.0f; x <= w; x += 6.0f)
            {
                const float ph = wavePhase * (layer == 0 ? 1.0f : -0.8f) + layer * 1.7f;
                const float y = baseY + layer * 8.0f + amp * std::sin (x * 0.012f * (layer == 0 ? 1.0f : 1.4f) + ph);
                wave.lineTo (x, y);
            }
            wave.lineTo (w, h);
            wave.closeSubPath();

            g.setGradientFill (juce::ColourGradient (accent.withAlpha (layer == 0 ? 0.55f : 0.35f), 0.0f, baseY - amp,
                                                     accent.darker (0.7f).withAlpha (0.9f), 0.0f, h, false));
            g.fillPath (wave);
        }
    }

    // заголовки
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (mkFont (22.0f, juce::Font::bold));
    g.drawText (ru ("Обработка басса"), 30, 28, 320, 30, juce::Justification::centredLeft);

    // надпись «Басс от лехи40» — в правом верхнем углу
    {
        const juce::Rectangle<int> t (getWidth() - 480, 12, 450, 64);
        const juce::String title = ru ("Басс от лехи40");
        g.setFont (mkFont (44.0f, juce::Font::bold));

        g.setColour (accent.withAlpha (0.10f));
        for (int dx = -4; dx <= 4; dx += 2)
            for (int dy = -4; dy <= 4; dy += 2)
                g.drawText (title, t.translated (dx, dy), juce::Justification::centredRight);

        g.setColour (juce::Colour (0xff053047).withAlpha (0.45f));
        g.drawText (title, t.translated (2, 3), juce::Justification::centredRight);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0.0f, (float) t.getY(),
                                                 accent.brighter (0.3f), 0.0f, (float) t.getBottom(), false));
        g.drawText (title, t, juce::Justification::centredRight);
    }

    // подписи крутилок и значения
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        const auto nameR = juce::Rectangle<int> (k.cell.getX(), k.cell.getY() + 128, k.cell.getWidth(), 22);
        const auto valR  = juce::Rectangle<int> (k.cell.getX(), k.cell.getY() + 150, k.cell.getWidth(), 20);

        g.setFont (mkFont (16.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff053047).withAlpha (0.35f));
        g.drawText (ru (kInfo[i].name), nameR.translated (1, 1), juce::Justification::centred);
        g.setColour (juce::Colours::white);
        g.drawText (ru (kInfo[i].name), nameR, juce::Justification::centred);

        g.setFont (mkFont (14.0f));
        g.setColour (juce::Colours::white.withAlpha (0.88f));
        g.drawText (juce::String (k.slider.getValue(), kInfo[i].decimals) + ru (kInfo[i].suffix),
                    valR, juce::Justification::centred);
    }
}

void BassEditor::paintOverChildren (juce::Graphics& g)
{
    const int active = hoverIdx >= 0 ? hoverIdx : pinnedIdx;
    if (active < 0)
        return;

    const auto& k = knobs[(size_t) active];
    const int panelW = 300;

    juce::AttributedString as;
    as.setJustification (juce::Justification::topLeft);
    as.append (ru (kInfo[active].name) + "\n\n", mkFont (17.0f, juce::Font::bold), juce::Colours::white);
    as.append (ru (kInfo[active].desc), mkFont (14.5f), juce::Colours::white.withAlpha (0.95f));

    juce::TextLayout tl;
    tl.createLayout (as, (float) (panelW - 32));
    const int panelH = (int) tl.getHeight() + 32;

    int x = k.arrow.getRight() + 8;
    if (x + panelW > getWidth() - 10)
        x = k.arrow.getX() - panelW - 8;
    int y = juce::jlimit (8, getHeight() - panelH - 8, k.arrow.getY() - 6);

    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) panelW, (float) panelH);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (r.translated (0.0f, 4.0f), 18.0f);

    g.setGradientFill (juce::ColourGradient (laf.accent.darker (0.3f).withAlpha (0.93f), 0.0f, r.getY(),
                                             laf.accent.darker (1.4f).withAlpha (0.95f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 18.0f);

    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle (r, 18.0f);
    g.reduceClipRegion (clip);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.35f), 0.0f, r.getY(),
                                             juce::Colours::white.withAlpha (0.0f), 0.0f, r.getY() + r.getHeight() * 0.5f, false));
    g.fillEllipse (r.getX() - 40.0f, r.getY() - r.getHeight() * 0.6f, r.getWidth() + 80.0f, r.getHeight() * 1.1f);
    g.restoreState();

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.drawRoundedRectangle (r, 18.0f, 1.6f);

    tl.draw (g, r.reduced (16.0f));
}
