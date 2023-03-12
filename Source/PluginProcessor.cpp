/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HARDAudioProcessor::HARDAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
,parameters(*this, nullptr, juce::Identifier("HARDPlugin"),
            {
    std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("harmony", 1),
                                                "Harmony",
                                                0.0f, 1.0f, 0.0f),
    std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("rhythm", 1),
                                                "Rhythm",
                                                0.0f, 1.0f, 0.0f),
    std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sourceGain", 1),
                                                "Source Gain",
                                                0.0f, 1.0f, 1.0f),
    std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sidechainGain", 1),
                                                "Sidechain Gain",
                                                0.0f, 1.0f, 1.0f),
    std::make_unique<juce::AudioParameterBool>(juce::ParameterID("sync",1),
                                               "Link Sliders",
                                               false)
})
{
    fifoBufferIn1.clearBuffer();
    fifoBufferIn2.clearBuffer();
    fifoBufferOutDNN.clearBuffer();
    setLatencySamples(OUTPUT_DELAY_SAMPLES-OUTPUT_DELAY_BIAS_SAMPLES);
    pInferenceThread = new ONNXMorpherInferenceThread();
    
    harmonyParameter = parameters.getRawParameterValue("harmony");
    rhythmParameter = parameters.getRawParameterValue("rhythm");
    sourceGainParameter = parameters.getRawParameterValue("sourceGain");
    sidechainGainParameter = parameters.getRawParameterValue("sidechainGain");
    syncParameter = parameters.getRawParameterValue("sync");
}

HARDAudioProcessor::~HARDAudioProcessor()
{
}

//==============================================================================
const juce::String HARDAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HARDAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HARDAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HARDAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HARDAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HARDAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int HARDAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HARDAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String HARDAudioProcessor::getProgramName (int index)
{
    return {};
}

void HARDAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void HARDAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    fifoBufferIn1.clearBuffer();
    fifoBufferIn2.clearBuffer();
    fifoBufferOutDNN.clearBuffer();
    //fifoBufferIn1.fillZeros(DNN_INPUT_CACHE_SAMPLES);
    //fifoBufferIn2.fillZeros(DNN_INPUT_CACHE_SAMPLES);
    fifoBufferOutDNN.fillZeros(OUTPUT_DELAY_SAMPLES);
}

void HARDAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HARDAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void HARDAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto numSamples = buffer.getNumSamples();
    
    auto mainInputOutput = getBusBuffer(buffer, true, 0);
    auto sideChainInput = getBusBuffer(buffer, true, 1);
    
    bool isSyncMode = *syncParameter > 0.5f;

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    //for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    //    buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    {
        const juce::ScopedLock lock (critical);
        fifoBufferIn1.pushData(mainInputOutput.getWritePointer(0), mainInputOutput.getWritePointer(1),  numSamples);
        fifoBufferIn2.pushData(sideChainInput.getWritePointer(0), sideChainInput.getWritePointer(1),  numSamples);
        numNewInputSamples += numSamples;
    }
    
    if (isSyncMode)
    {
        if(*harmonyParameter != preHarmonyParam)
        {
            *rhythmParameter = (float)*harmonyParameter;
        }
        else if(*rhythmParameter != preRhythmParam)
        {
            *harmonyParameter = (float)*rhythmParameter;
        }
    }
    
    
    if ((numNewInputSamples >= DNN_INPUT_SAMPLES) and (fifoBufferIn1.getBufferSize() >= (DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES)) and (!pInferenceThread->threadIsInferring()))
    {
        const juce::ScopedLock lock (critical);
        jassert(fifoBufferIn1.getBufferSize() >= (DNN_INPUT_SAMPLES + DNN_INPUT_CACHE_SAMPLES));
        fifoBufferIn1.readData(dnnInputData1.data(), DNN_INPUT_SAMPLES + DNN_INPUT_CACHE_SAMPLES, DNN_INPUT_SAMPLES);
        fifoBufferIn2.readData(dnnInputData2.data(), DNN_INPUT_SAMPLES + DNN_INPUT_CACHE_SAMPLES, DNN_INPUT_SAMPLES);
        
        // Trigger a DNN inference
        pInferenceThread->requestInference(dnnInputData1.data(), dnnInputData2.data(), *rhythmParameter, *harmonyParameter, *sourceGainParameter, *sidechainGainParameter, &fifoBufferOutDNN);
        
        numNewInputSamples -= DNN_INPUT_SAMPLES;
        printf("Inference requested. \n");
    }
    
    
    
    auto dnnOutBufferSize = fifoBufferOutDNN.getBufferSize();
    while((pInferenceThread->threadIsInferring()) and (dnnOutBufferSize<numSamples))
    {
        // If output buffer is empty but DNN inference is not done,
        // pause the process.
        // This loop should not be triggered.
    }
    
    {
        const juce::ScopedLock lock (critical);
        fifoBufferOutDNN.readData(outBufferL.data(), outBufferR.data(), numSamples, numSamples);
        
        mainInputOutput.copyFrom(0, 0, outBufferL.data(), numSamples);
        mainInputOutput.copyFrom(1, 0, outBufferR.data(), numSamples);
        
    }
    //printf("Buffer size out: %d in: %d New: %d\n", fifoBufferOutDNN.getBufferSize(), fifoBufferIn1.getBufferSize(), numNewInputSamples);
    
    preHarmonyParam = *harmonyParameter;
    preRhythmParam = *rhythmParameter;
}

//==============================================================================
bool HARDAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* HARDAudioProcessor::createEditor()
{
    return new HARDAudioProcessorEditor (*this);
}

//==============================================================================
void HARDAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HARDAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if(xmlState.get()!=nullptr)
    {
        if(xmlState->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HARDAudioProcessor();
}
