/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TaroNoiseGateAudioProcessor::TaroNoiseGateAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)
                       ),apvts(*this, nullptr, "Parameters", createParameters())

{
}

TaroNoiseGateAudioProcessor::~TaroNoiseGateAudioProcessor()
{
}

//==============================================================================
const juce::String TaroNoiseGateAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TaroNoiseGateAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TaroNoiseGateAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TaroNoiseGateAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TaroNoiseGateAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TaroNoiseGateAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TaroNoiseGateAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TaroNoiseGateAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TaroNoiseGateAudioProcessor::getProgramName (int index)
{
    return {};
}

void TaroNoiseGateAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TaroNoiseGateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    lowPassCoeff = 0.0f;
    sampleCountDown = 0;
}

void TaroNoiseGateAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TaroNoiseGateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return (layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()) && ! layouts.getMainInputChannelSet().isDisabled();
    //isDisabled: Returns true if there are no channels in the set
    //?????????input channels???output channels??????????????????
    //?????????input buses?????????enabled???
}
#endif

void TaroNoiseGateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    auto mainInputOutput = getBusBuffer (buffer, true, 0);
    auto sideChainInput  = getBusBuffer (buffer, true, 1);
    //?????????sidechain buffer???main IO buffer????????????????????????????????????????????????
    
    auto alphaCopy = apvts.getRawParameterValue("alpha")->load();
    auto thresholdCopy = apvts.getRawParameterValue("threshold")->load();
    //???valuetree???????????????parameters??????
    
    for (int j = 0; j < buffer.getNumSamples(); ++j)
    //?????????????????????????????????audio buffer block??????samples???????????????????????????????????????????????????   ???channels???????????????????????????????????????????????????sample??????????????????????????????channel??????????????????????????????????????????
    {
        auto mixedSamples = 0.0f;

        for (int i = 0; i < sideChainInput.getNumChannels(); ++i)
            mixedSamples += sideChainInput.getReadPointer (i)[j];
        //??????sidechain????????????channel??????????????????????????????????????????????????????????????????
        
        mixedSamples /= static_cast<float> (sideChainInput.getNumChannels());
        lowPassCoeff = (alphaCopy * lowPassCoeff) + ((1.0f - alphaCopy) * mixedSamples);
        //????????????y[i] = ((1 - alpha) * sidechain) + (alpha * y[i - 1])?????????lowPassCoeff
        
        if (lowPassCoeff >= thresholdCopy)
            sampleCountDown = (int) getSampleRate();
        //??????lowPassCoeff >= thresholdCopy???????????????????????????sampleCountDown???sample rate

        // very in-effective way of doing this
        for (int i = 0; i < mainInputOutput.getNumChannels(); ++i)
            *mainInputOutput.getWritePointer (i, j) = sampleCountDown > 0 ? *mainInputOutput.getReadPointer (i, j) : 0.0f;
        //???????????????input channel?????????countdown??????0?????????????????????input buffer sample?????????output buffer sample??????????????????????????????zero samples????????????output signal

        if (sampleCountDown > 0)
            --sampleCountDown;
        //?????????sample????????????????????????????????????????????????
        
    }
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        // ..do something to the data...
    }
}

//==============================================================================
bool TaroNoiseGateAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TaroNoiseGateAudioProcessor::createEditor()
{
    return new TaroNoiseGateAudioProcessorEditor (*this);
}

//==============================================================================
void TaroNoiseGateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TaroNoiseGateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TaroNoiseGateAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout TaroNoiseGateAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("threshold", "Threshold", 0.0f, 1.0f, 0.5f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("alpha", "Alpha", 0.0f, 1.0f, 0.8f));
    
    return { params.begin(), params.end() };
}
