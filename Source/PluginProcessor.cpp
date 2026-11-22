#include "PluginProcessor.h"
#include "PluginEditor.h"

AbyssVocalAudioProcessor::AbyssVocalAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "ABYSS_STATE", createParameterLayout())
{}

juce::AudioProcessorValueTreeState::ParameterLayout AbyssVocalAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<APF>("amount", "Abyss", juce::NormalisableRange<float>(0, 1, 0.001f), 0));
    p.push_back(std::make_unique<APF>("warmth", "Warmth", 0.0f, 1.0f, 0.65f));
    p.push_back(std::make_unique<APF>("dark", "Dark", 0.0f, 1.0f, 0.55f));
    p.push_back(std::make_unique<APF>("chorus", "Chorus", 0.0f, 1.0f, 0.45f));
    p.push_back(std::make_unique<APF>("detune", "Detune", 0.0f, 1.0f, 0.35f));
    p.push_back(std::make_unique<APF>("wobble", "Wobble", 0.0f, 1.0f, 0.25f));
    p.push_back(std::make_unique<APF>("flanger", "Flanger", 0.0f, 1.0f, 0.20f));
    p.push_back(std::make_unique<APF>("phaser", "Phaser", 0.0f, 1.0f, 0.18f));
    p.push_back(std::make_unique<APF>("space", "Atmosphere", 0.0f, 1.0f, 0.60f));
    p.push_back(std::make_unique<APF>("width", "Width", 0.0f, 1.0f, 0.75f));
    p.push_back(std::make_unique<APF>("mix", "Wet", 0.0f, 1.0f, 1.0f));
    return { p.begin(), p.end() };
}

bool AbyssVocalAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && in == out;
}

void AbyssVocalAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    delayBuffer.setSize(2, juce::jmax(4096, (int)(sr * 0.12) + samplesPerBlock + 8));
    delayBuffer.clear();
    writePos = 0;
    phaseL = 0.0f; phaseR = 1.7f;
    for (auto& channel : phaserState) for (float& state : channel) state = 0.0f;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumOutputChannels() };
    reverb.reset();
    reverb.setSampleRate(sr);
    toneCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 18000.0);
    toneL.coefficients = toneCoeffs;
    toneR.coefficients = toneCoeffs;
    toneL.reset(); toneR.reset();
}

void AbyssVocalAudioProcessor::releaseResources() {}

void AbyssVocalAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (channels == 0) return;

    const float raw = parameters.getRawParameterValue("amount")->load();
    // Deliberately front-loaded macro curve: meaningful colour arrives early.
    const float macro = juce::jlimit(0.0f, 1.0f, std::pow(raw, 0.48f));
    const auto get = [this](const char* id) { return parameters.getRawParameterValue(id)->load(); };
    const float warmth = get("warmth"), dark = get("dark"), chorus = get("chorus");
    const float detune = get("detune"), wobble = get("wobble"), space = get("space");
    const float flanger = get("flanger"), phaser = get("phaser");
    const float width = get("width"), wet = get("mix");

    const float drive = 1.0f + macro * (1.0f + 8.0f * warmth);
    const float cutoff = 18000.0f - macro * dark * 14500.0f;
    toneCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, juce::jlimit(900.0f, 18000.0f, cutoff));
    toneL.coefficients = toneCoeffs; toneR.coefficients = toneCoeffs;

    reverbParams.roomSize = juce::jlimit(0.1f, 0.98f, 0.25f + macro * space * 0.72f);
    reverbParams.damping = 0.72f - macro * 0.30f;
    reverbParams.wetLevel = 0.08f + macro * space * 0.62f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = juce::jlimit(0.0f, 1.0f, width);
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    const float baseDelay = 0.012f + detune * 0.010f;
    const float depth = macro * chorus * (0.0025f + detune * 0.0045f);
    const float rate = 0.12f + wobble * (0.25f + macro * 1.8f);
    const float feedback = macro * (wobble * 0.12f + flanger * 0.22f);
    juce::AudioBuffer<float> dry(buffer.getNumChannels(), n);
    dry.makeCopyOf(buffer, true);

    for (int i = 0; i < n; ++i)
    {
        phaseL += (float)(juce::MathConstants<double>::twoPi * rate / sr);
        phaseR += (float)(juce::MathConstants<double>::twoPi * rate * 1.013 / sr);
        if (phaseL > juce::MathConstants<float>::twoPi) phaseL -= juce::MathConstants<float>::twoPi;
        if (phaseR > juce::MathConstants<float>::twoPi) phaseR -= juce::MathConstants<float>::twoPi;

        for (int ch = 0; ch < channels; ++ch)
        {
            float x = buffer.getSample(ch, i);
            const float saturated = std::tanh(x * drive) / std::tanh(drive);
            x += (saturated - x) * macro * warmth;

            const float phase = ch == 0 ? phaseL : phaseR;
            const float delaySeconds = baseDelay + depth * (0.5f + 0.5f * std::sin(phase));
            const int delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 2, (int)(delaySeconds * sr));
            const int readPos = (writePos - delaySamples + delayBuffer.getNumSamples()) % delayBuffer.getNumSamples();
            const float delayed = delayBuffer.getSample(ch, readPos);
            // Chorus + flanger share a modulated delay core. Flanger uses a shorter,
            // more resonant delay and feedback for the classic moving comb-filter tone.
            const float flangeDelay = 0.0012f + (0.0018f + 0.0025f * flanger)
                * (0.5f + 0.5f * std::sin(phase * 0.63f + (ch == 0 ? 0.0f : 1.1f)));
            const int flangeSamples = juce::jlimit(1, delayBuffer.getNumSamples() - 2,
                                                    (int)(flangeDelay * sr));
            const int flangeRead = (writePos - flangeSamples + delayBuffer.getNumSamples())
                                   % delayBuffer.getNumSamples();
            const float flangeTap = delayBuffer.getSample(ch, flangeRead);
            const float chorusMix = macro * chorus * 0.24f;
            const float flangeMix = macro * flanger * 0.42f;
            float modulated = x * (1.0f - chorusMix - flangeMix)
                            + delayed * chorusMix + flangeTap * flangeMix;
            delayBuffer.setSample(ch, writePos, x + delayed * feedback);

            // Four-stage all-pass phaser with offset L/R LFO phase for stereo motion.
            const float phaserLfo = 0.5f + 0.5f * std::sin(phase * 0.37f + (ch == 0 ? 0.0f : 1.35f));
            const float a = juce::jlimit(0.08f, 0.78f, 0.12f + phaserLfo * 0.58f);
            float ph = modulated;
            for (int stage = 0; stage < 4; ++stage)
            {
                const float out = -a * ph + phaserState[ch][stage];
                phaserState[ch][stage] = ph + a * out;
                ph = out;
            }
            modulated += (ph - modulated) * macro * phaser * 0.85f;

            float y = ch == 0 ? toneL.processSample(modulated) : toneR.processSample(modulated);
            const float original = dry.getSample(ch, i);
            buffer.setSample(ch, i, original + (y - original) * macro);
        }
        writePos = (writePos + 1) % delayBuffer.getNumSamples();
    }

    if (channels == 1)
    {
        reverb.processMono(buffer.getWritePointer(0), n);
    }
    else
    {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), n);
    }

    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < n; ++i)
        {
            const float drySample = dry.getSample(ch, i);
            const float processed = buffer.getSample(ch, i);
            buffer.setSample(ch, i, drySample + (processed - drySample) * wet);
        }
}

juce::AudioProcessorEditor* AbyssVocalAudioProcessor::createEditor()
{
    return new AbyssVocalAudioProcessorEditor(*this);
}

void AbyssVocalAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void AbyssVocalAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AbyssVocalAudioProcessor();
}
