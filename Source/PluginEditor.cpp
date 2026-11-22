#include "PluginEditor.h"

static const juce::Colour bg(0xff090e13), panel(0xff111a22), cyan(0xff69d8d2), muted(0xff82949f);

AbyssVocalAudioProcessorEditor::AbyssVocalAudioProcessorEditor(AbyssVocalAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), display(p)
{
    setSize(720, 650);
    brand.setText("ABYSS AUDIO  /  VOCAL EXPERIMENTS", juce::dontSendNotification);
    brand.setColour(juce::Label::textColourId, muted);
    brand.setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(brand);
    product.setText("A B Y S S", juce::dontSendNotification);
    product.setJustificationType(juce::Justification::centred);
    product.setColour(juce::Label::textColourId, juce::Colours::white);
    product.setFont(juce::Font(23.0f, juce::Font::bold));
    addAndMakeVisible(product);
    amountLabel.setText("UNDERWATER CHARACTER", juce::dontSendNotification);
    amountLabel.setJustificationType(juce::Justification::centred);
    amountLabel.setColour(juce::Label::textColourId, cyan);
    amountLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    addAndMakeVisible(amountLabel);

    addAndMakeVisible(display);
    macro.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    macro.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    macro.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                              juce::MathConstants<float>::pi * 2.75f, true);
    macro.setColour(juce::Slider::rotarySliderFillColourId, cyan);
    macro.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff263641));
    macro.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(macro);
    macroAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "amount", macro);

    expert.setClickingTogglesState(true);
    expert.setColour(juce::TextButton::buttonColourId, panel);
    expert.setColour(juce::TextButton::textColourOffId, muted);
    expert.onClick = [this]
    {
        expertOpen = expert.getToggleState();
        expert.setButtonText(expertOpen ? "EXPERT MODE  -" : "EXPERT MODE  +");
        setSize(720, expertOpen ? 820 : 650);
        resized();
    };
    addAndMakeVisible(expert);

    for (int i = 0; i < paramIDs.size(); ++i)
    {
        auto* s = expertSliders.add(new juce::Slider());
        s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
        s->setColour(juce::Slider::rotarySliderFillColourId, cyan);
        s->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff263641));
        s->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        s->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addChildComponent(s);
        auto* l = expertLabels.add(new juce::Label());
        l->setText(paramNames[i], juce::dontSendNotification);
        l->setJustificationType(juce::Justification::centred);
        l->setColour(juce::Label::textColourId, muted);
        l->setFont(juce::Font(10.0f, juce::Font::bold));
        addChildComponent(l);
        attachments.add(new juce::AudioProcessorValueTreeState::SliderAttachment(processor.parameters, paramIDs[i], *s));
    }
    startTimerHz(20);
    resized();
}

void AbyssVocalAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(28);
    brand.setBounds(r.removeFromTop(22));
    product.setBounds(0, 42, getWidth(), 34);
    display.setBounds(85, 88, getWidth() - 170, 190);
    amountLabel.setBounds(0, 286, getWidth(), 18);
    macro.setBounds((getWidth() - 230) / 2, 300, 230, 230);
    expert.setBounds((getWidth() - 150) / 2, 548, 150, 30);
    if (expertOpen)
    {
        const int startY = 600;
        const int cellW = (getWidth() - 80) / 5;
        for (int i = 0; i < expertSliders.size(); ++i)
        {
            const int row = i / 5, col = i % 5;
            expertLabels[i]->setBounds(40 + col * cellW, startY + row * 92, cellW, 18);
            expertSliders[i]->setBounds(40 + col * cellW, startY + row * 92 + 16, cellW, 78);
            expertLabels[i]->setVisible(true);
            expertSliders[i]->setVisible(true);
        }
    }
    else
        for (int i = 0; i < expertSliders.size(); ++i)
        {
            expertLabels[i]->setVisible(false);
            expertSliders[i]->setVisible(false);
        }
}

void AbyssVocalAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(juce::Colour(0xff263640));
    g.drawRoundedRectangle(bounds, 14.0f, 1.0f);
    g.setColour(panel);
    g.fillRoundedRectangle(28.0f, 82.0f, (float)getWidth() - 56.0f, 202.0f, 10.0f);
    g.setColour(juce::Colour(0xff1d2a34));
    g.drawEllipse((getWidth()-246)*0.5f, 294.0f, 246.0f, 246.0f, 1.0f);
}

void AbyssVocalAudioProcessorEditor::Display::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(panel); g.fillRoundedRectangle(b, 9.0f);
    const float amount = processor.parameters.getRawParameterValue("amount")->load();
    phase += 0.025f + amount * 0.08f;
    juce::Path wave;
    const float mid = b.getCentreY();
    for (int x = 0; x < (int)b.getWidth(); x += 2)
    {
        const float nx = x / b.getWidth();
        const float envelope = 0.18f + amount * 0.30f;
        const float y = mid + std::sin(nx * 23.0f + phase) * b.getHeight() * envelope
            + std::sin(nx * 57.0f - phase * 0.7f) * b.getHeight() * amount * 0.10f;
        if (x == 0) wave.startNewSubPath((float)x, y); else wave.lineTo((float)x, y);
    }
    g.setColour(juce::Colour(0x2269d8d2));
    for (int i = 0; i < 5; ++i)
    {
        auto glow = wave;
        g.strokePath(glow, juce::PathStrokeType(7.0f + i * 2.0f));
    }
    g.setColour(cyan);
    g.strokePath(wave, juce::PathStrokeType(1.7f));
    juce::Random rng(42);
    for (int i = 0; i < 34; ++i)
    {
        const float x = std::fmod((float)i * 31.7f + phase * (8.0f + i % 4), b.getWidth());
        const float y = std::fmod((float)i * 17.3f + std::sin(phase + i) * 30.0f + b.getHeight() * 0.5f, b.getHeight());
        g.setColour(juce::Colour(0x8869d8d2).withAlpha(0.15f + amount * 0.55f));
        g.fillEllipse(x, y, 1.5f + amount * 2.0f, 1.5f + amount * 2.0f);
    }
    g.setColour(muted);
    g.setFont(10.0f);
    g.drawText(amount < 0.08f ? "SIGNAL / CLEAN" : amount < 0.45f ? "SIGNAL / SUBMERGING" : "SIGNAL / DEEP ABYSS",
               14, 10, getWidth()-28, 16, juce::Justification::centredLeft);
}
