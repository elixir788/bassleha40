#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class BassProcessor : public juce::AudioProcessor
{
public:
    BassProcessor();
    ~BassProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Уровень баса для анимации интерфейса (0..1)
    std::atomic<float> level { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::dsp::LinkwitzRileyFilter<float> cross;
    juce::dsp::StateVariableTPTFilter<float> subA, subB; // чистка низа (2 фильтра = 24 дБ/окт)
    juce::dsp::StateVariableTPTFilter<float> harmHP, harmLP;

    juce::SmoothedValue<float> gSub, gDrive, gHarm, gMono, gMix, gOut;

    double sr = 44100.0;
    float envDb = -100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassProcessor)
};
