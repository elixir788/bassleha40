#pragma once
#include "PluginProcessor.h"
#include <array>

//==============================================================================
// Стеклянная крутилка в стиле Fruity Aero
class GlassLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    juce::Colour accent { 0xff35d6ff };
};

//==============================================================================
// Маленькая стрелка рядом с крутилкой: наведи — появится подсказка, клик — закрепить
class InfoArrow : public juce::Component
{
public:
    InfoArrow() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    std::function<void (bool)> onHover;
    std::function<void()> onClick;
    bool pinned = false;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { hovered = true;  repaint(); if (onHover) onHover (true); }
    void mouseExit  (const juce::MouseEvent&) override { hovered = false; repaint(); if (onHover) onHover (false); }
    void mouseDown  (const juce::MouseEvent&) override { if (onClick) onClick(); }

private:
    bool hovered = false;
};

//==============================================================================
class BassEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BassEditor (BassProcessor&);
    ~BassEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        InfoArrow arrow;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        juce::Rectangle<int> cell;
    };

    struct Bubble { float x, y, r, speed, phase; };

    BassProcessor& proc;
    GlassLookAndFeel laf;
    std::array<Knob, 9> knobs;
    std::array<Bubble, 48> bubbles;
    juce::Random rng;

    int hoverIdx = -1, pinnedIdx = -1;

    // состояние, от которого зависит внешний вид
    float drive = 0.0f, harm = 0.0f, tight = 0.0f, subN = 0.5f, lvl = 0.0f;
    float hue = 0.52f, wavePhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassEditor)
};
