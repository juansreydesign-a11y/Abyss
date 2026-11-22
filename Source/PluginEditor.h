#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AbyssVocalAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit AbyssVocalAudioProcessorEditor(AbyssVocalAudioProcessor&);
    ~AbyssVocalAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class Display : public juce::Component, private juce::Timer
    {
    public:
        Display(AbyssVocalAudioProcessor& p) : processor(p) { startTimerHz(30); }
        void paint(juce::Graphics&) override;
    private:
        void timerCallback() override { repaint(); }
        AbyssVocalAudioProcessor& processor;
        float phase = 0.0f;
    };

    void timerCallback() override { display.repaint(); }
    AbyssVocalAudioProcessor& processor;
    Display display;
    juce::Slider macro;
    juce::Label brand, product, amountLabel;
    juce::TextButton expert { "EXPERT MODE  +" };
    juce::OwnedArray<juce::Slider> expertSliders;
    juce::OwnedArray<juce::Label> expertLabels;
    juce::Array<juce::String> paramIDs { "warmth", "dark", "chorus", "detune", "wobble", "flanger", "phaser", "space", "width", "mix" };
    juce::Array<juce::String> paramNames { "WARMTH", "DARK", "CHORUS", "DETUNE", "WOBBLE", "FLANGER", "PHASER", "SPACE", "WIDTH", "WET" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroAttachment;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
    bool expertOpen = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AbyssVocalAudioProcessorEditor)
};
