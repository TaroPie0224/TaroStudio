/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TaroDelayAudioProcessor::TaroDelayAudioProcessor()
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

TaroDelayAudioProcessor::~TaroDelayAudioProcessor()
{
}

//==============================================================================
const juce::String TaroDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TaroDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TaroDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TaroDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TaroDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TaroDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TaroDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TaroDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TaroDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void TaroDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TaroDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auto delayBufferSize = sampleRate * 2.0;
    delayBuffer.setSize (getTotalNumOutputChannels(), (int)delayBufferSize);
}

void TaroDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TaroDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void TaroDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        fillBuffer(buffer, channel);
        readFromBuffer(buffer, delayBuffer, channel);
        fillBuffer(buffer, channel);
    }
    
    updateBufferPositions(buffer, delayBuffer);
    
}

void TaroDelayAudioProcessor::fillBuffer(juce::AudioBuffer<float>& buffer, int channel)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    
    // ????????????circular buffer?????????????????????????????????????????????buffer
    if (delayBufferSize > bufferSize + writePosition)
    {
        // ??????????????????writePositon?????????????????????buffer
        delayBuffer.copyFrom(channel, writePosition, buffer.getWritePointer (channel), bufferSize);
    }
    // ??????????????????
    else
    {
        // ???????????????????????????????????????????????????
        auto numSamplesToEnd = delayBufferSize - writePosition;
        delayBuffer.copyFrom(channel, writePosition, buffer.getWritePointer (channel), numSamplesToEnd);
        
        // ?????????bufferSize????????????????????????????????????????????????
        auto numSamplesAtStart = bufferSize - numSamplesToEnd;
        // ???circular buffer????????????????????????????????????
        delayBuffer.copyFrom(channel, 0, buffer.getWritePointer (channel, numSamplesToEnd), numSamplesAtStart);
    }
}

void TaroDelayAudioProcessor::readFromBuffer(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer, int channel)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    auto currentFeedBack = apvts.getRawParameterValue("FEEDBACK")->load();
    auto currentDelay = apvts.getRawParameterValue("DELAY")->load();
    
    // ??????1?????????audio
    auto readPosition = writePosition - currentDelay * getSampleRate();
    // ????????????????????????writePosition??????circular buffer??????????????????????????????readPositon?????????????????????????????????????????????
    // ???????????????????????????????????????????????????circular buffer???????????????
    if(readPosition < 0)
        readPosition += delayBufferSize;
    
    if (readPosition + bufferSize < delayBufferSize)
    {
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, currentFeedBack, currentFeedBack);
    }
    else
    {
        auto numSamplesToEnd = delayBufferSize - readPosition;
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, currentFeedBack, currentFeedBack);
        
        auto numSamplesAtStart = bufferSize - numSamplesToEnd;
        buffer.addFromWithRamp(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesAtStart, currentFeedBack, currentFeedBack);
    }
}

void TaroDelayAudioProcessor::updateBufferPositions(juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>& delayBuffer)
{
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();
    
    writePosition += bufferSize;
    writePosition %= delayBufferSize;
}

//==============================================================================
bool TaroDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TaroDelayAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
    // return new TaroDelayAudioProcessorEditor (*this);
}

//==============================================================================
void TaroDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);
}

void TaroDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);
            
    if (tree.isValid()) {
        apvts.state = tree;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TaroDelayAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    
    using namespace juce;
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "FEEDBACK", 1 },
                                                     "FEEDBACK",
                                                     NormalisableRange<float>(0, 1, 0.1, 1), 0.5));
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { "DELAY", 1 },
                                                     "DELAY",
                                                     NormalisableRange<float>(0, 2, 0.1, 1), 1));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TaroDelayAudioProcessor();
}
