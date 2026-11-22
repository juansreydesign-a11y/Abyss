#pragma once
#include <JuceHeader.h>

class AbyssVocalAudioProcessor : public juce::AudioProcessor
{
public:
    AbyssVocalAudioProcessor();
    ~AbyssVocalAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ABYSS"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    float knob = 0.0f;
    float phaseL = 0.0f, phaseR = 1.7f;
    double sr = 44100.0;
    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;
    juce::dsp::IIR::Filter<float> toneL, toneR;
    // Four modulated all-pass stages per channel for the stereo phaser.
    float phaserState[2][4] {};
    juce::dsp::IIR::Coefficients<float>::Ptr toneCoeffs;
    juce::Random random;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AbyssVocalAudioProcessor)
};
