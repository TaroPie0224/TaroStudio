/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TaroTremeloAudioProcessor::TaroTremeloAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

TaroTremeloAudioProcessor::~TaroTremeloAudioProcessor()
{
}

//==============================================================================
const juce::String TaroTremeloAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TaroTremeloAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TaroTremeloAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TaroTremeloAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TaroTremeloAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TaroTremeloAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TaroTremeloAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TaroTremeloAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TaroTremeloAudioProcessor::getProgramName (int index)
{
    return {};
}

void TaroTremeloAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TaroTremeloAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void TaroTremeloAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TaroTremeloAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void TaroTremeloAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    auto currentRate = apvts.getRawParameterValue("Rate")->load();
    auto currentDepth = apvts.getRawParameterValue("Depth")->load();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block {buffer};

    // ????????????
    float periodicity = 1 / currentRate;
    // ???????????????
    int sampleRate = getSampleRate();
    // ??????????????????block????????????sample
    for (int sample = 0; sample < block.getNumSamples(); ++sample)
    {
        // ??????????????????
        auto* channelDataLeft = buffer.getWritePointer(0);
        auto* channelDataRight = buffer.getWritePointer(1);
            
        // ???depth?????????LFO???amplitude
        float a = currentDepth / 200;
        // offset???a??????
        float offset = 1 - a;
        // ???????????????????????????????????????????????????????????????
        float currentTime = static_cast<float>(sampleCount) / static_cast<float>(sampleRate);
        
        // ???????????????LFO
        auto lfo = a * std::sin(juce::MathConstants<float>::twoPi * currentRate * currentTime) + offset;
        
        // ?????????sample???
        channelDataLeft[sample] *= lfo;
        channelDataRight[sample] *= lfo;
        
        // ???????????????sampleCount???????????????????????????sampleCount??????
        if (currentTime < periodicity)
        {
            sampleCount++;
        }
        // ?????????????????????????????????sampleCount??????
        // ?????????????????????sampleCount?????????????????????????????????????????????????????????????????????
        else
        {
            sampleCount = 0;
        }
    }
}

//==============================================================================
bool TaroTremeloAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TaroTremeloAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
//    return new TaroTremeloAudioProcessorEditor (*this);
}

//==============================================================================
void TaroTremeloAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);
}

void TaroTremeloAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
        
    if (tree.isValid()) {
        apvts.state = tree;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TaroTremeloAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    
    using namespace juce;
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "Rate", 1 },
                                                     "Rate",
                                                     NormalisableRange<float>(0, 20, 1, 1), 5));
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "Depth", 1 },
                                                     "Depth",
                                                     NormalisableRange<float>(0, 100, 1, 1), 50));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TaroTremeloAudioProcessor();
}
