#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String ru (const char* s) { return juce::String::fromUTF8 (s); }

static inline float softClip (float y)
{
    const float a = std::abs (y);
    if (a <= 0.9f) return y;
    const float c = 0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f);
    return y < 0.0f ? -c : c;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout BassProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    using ID = juce::ParameterID;

    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<F> (ID { "cross", 1 }, ru ("Граница басса"), R (40.0f, 250.0f, 1.0f, 0.5f), 120.0f));
    l.add (std::make_unique<F> (ID { "sub",   1 }, ru ("Глубина"),       R (-12.0f, 12.0f, 0.1f),       3.0f));
    l.add (std::make_unique<F> (ID { "drive", 1 }, ru ("Плотность"),     R (0.0f, 100.0f, 1.0f),        35.0f));
    l.add (std::make_unique<F> (ID { "harm",  1 }, ru ("Гармоники"),     R (0.0f, 100.0f, 1.0f),        25.0f));
    l.add (std::make_unique<F> (ID { "tight", 1 }, ru ("Сжатие"),        R (0.0f, 100.0f, 1.0f),        30.0f));
    l.add (std::make_unique<F> (ID { "clean", 1 }, ru ("Чистка низа"),   R (15.0f, 60.0f, 1.0f),        25.0f));
    l.add (std::make_unique<F> (ID { "mono",  1 }, ru ("Моно-бас"),      R (0.0f, 100.0f, 1.0f),        70.0f));
    l.add (std::make_unique<F> (ID { "mix",   1 }, ru ("Микс"),          R (0.0f, 100.0f, 1.0f),        100.0f));
    l.add (std::make_unique<F> (ID { "out",   1 }, ru ("Громкость"),     R (-12.0f, 12.0f, 0.1f),       0.0f));
    return l;
}

BassProcessor::BassProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool BassProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void BassProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    cross.prepare (spec);
    cross.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);

    subA.prepare (spec);  subA.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    subB.prepare (spec);  subB.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    harmHP.prepare (spec); harmHP.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    harmLP.prepare (spec); harmLP.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    cross.reset(); subA.reset(); subB.reset(); harmHP.reset(); harmLP.reset();
    envDb = -100.0f;

    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    for (auto* s : { &gSub, &gDrive, &gHarm, &gMono, &gMix, &gOut })
        s->reset (sampleRate, 0.03);

    gSub.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p ("sub")));
    gDrive.setCurrentAndTargetValue (p ("drive") / 100.0f);
    gHarm.setCurrentAndTargetValue (p ("harm") / 100.0f);
    gMono.setCurrentAndTargetValue (p ("mono") / 100.0f);
    gMix.setCurrentAndTargetValue (p ("mix") / 100.0f);
    gOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p ("out")));
}

void BassProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin (2, buffer.getNumChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numCh < 1)
        return;

    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float cf    = p ("cross");
    const float clean = p ("clean");
    const float tight = p ("tight") / 100.0f;

    cross.setCutoffFrequency (cf);
    subA.setCutoffFrequency (clean);
    subB.setCutoffFrequency (clean);
    harmHP.setCutoffFrequency (juce::jmax (80.0f, cf * 0.8f));
    harmLP.setCutoffFrequency (3000.0f);

    gSub.setTargetValue (juce::Decibels::decibelsToGain (p ("sub")));
    gDrive.setTargetValue (p ("drive") / 100.0f);
    gHarm.setTargetValue (p ("harm") / 100.0f);
    gMono.setTargetValue (p ("mono") / 100.0f);
    gMix.setTargetValue (p ("mix") / 100.0f);
    gOut.setTargetValue (juce::Decibels::decibelsToGain (p ("out")));

    // Компрессор басса: чем больше "Сжатие", тем ниже порог и выше степень
    const float thr    = -4.0f - 26.0f * tight;
    const float ratio  = 1.0f + 5.0f * tight;
    const float slope  = 1.0f - 1.0f / ratio;
    const float makeup = juce::Decibels::decibelsToGain (0.25f * (-thr) * slope);
    const float atk    = 1.0f - std::exp (-1.0f / (float) (sr * 0.005));
    const float rel    = 1.0f - std::exp (-1.0f / (float) (sr * 0.120));

    float* l = buffer.getWritePointer (0);
    float* r = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float blockPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float sub  = gSub.getNextValue();
        const float dr   = gDrive.getNextValue();
        const float hm   = gHarm.getNextValue();
        const float mono = gMono.getNextValue();
        const float mx   = gMix.getNextValue();
        const float og   = gOut.getNextValue();

        const float in[2] = { l[i], r != nullptr ? r[i] : l[i] };
        float low[2], high[2];

        for (int c = 0; c < 2; ++c)
        {
            cross.processSample (c, in[c], low[c], high[c]);
            low[c] = subB.processSample (c, subA.processSample (c, low[c]));
        }

        // Моно-бас
        const float m = 0.5f * (low[0] + low[1]);
        low[0] += (m - low[0]) * mono;
        low[1] += (m - low[1]) * mono;

        // Глубина
        low[0] *= sub;
        low[1] *= sub;

        // Сжатие
        const float peak = juce::jmax (std::abs (low[0]), std::abs (low[1]));
        const float lvDb = juce::Decibels::gainToDecibels (peak, -100.0f);
        envDb += (lvDb - envDb) * (lvDb > envDb ? atk : rel);
        const float over = envDb - thr;
        const float gr   = over > 0.0f ? -over * slope : 0.0f;
        const float g    = juce::Decibels::decibelsToGain (gr) * makeup;
        low[0] *= g;
        low[1] *= g;

        // Плотность + гармоники
        const float k    = 1.0f + dr * 4.0f;
        const float norm = 1.0f / std::tanh (k);

        float outS[2];
        for (int c = 0; c < 2; ++c)
        {
            const float x = low[c];
            const float s = std::tanh (k * x) * norm;
            const float dense = x + (s - x) * dr;

            const float hs = std::tanh (4.0f * x);
            const float hb = harmLP.processSample (c, harmHP.processSample (c, hs));

            const float wet = dense + hb * hm * 0.6f + high[c];
            outS[c] = softClip ((in[c] + (wet - in[c]) * mx) * og);

            blockPeak = juce::jmax (blockPeak, std::abs (dense));
        }

        l[i] = outS[0];
        if (r != nullptr)
            r[i] = outS[1];
    }

    level.store (juce::jmin (1.0f, juce::jmax (blockPeak * 1.5f, level.load() * 0.85f)));
}

//==============================================================================
juce::AudioProcessorEditor* BassProcessor::createEditor()
{
    return new BassEditor (*this);
}

void BassProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BassProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassProcessor();
}
