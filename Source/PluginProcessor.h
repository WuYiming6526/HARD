/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ONNXInferenceThread.hpp"

//==============================================================================
/**
*/
class HARDAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    HARDAudioProcessor();
    ~HARDAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
private:
    juce::CriticalSection critical;
    
    std::atomic<float>* harmonyParameter = nullptr;
    std::atomic<float>* rhythmParameter = nullptr;
    std::atomic<float>* sourceGainParameter = nullptr;
    std::atomic<float>* sidechainGainParameter = nullptr;
    std::atomic<float>* syncParameter = nullptr;
    
    float preHarmonyParam;
    float preRhythmParam;
    
    FifoBuffer fifoBufferIn1;
    FifoBuffer fifoBufferIn2;
    FifoBuffer fifoBufferOutDNN;
    
    int numNewInputSamples=0;
    static const unsigned int DNN_INPUT_SAMPLES = 8192;
    static const unsigned int DNN_INPUT_CACHE_SAMPLES = 8192;
    static const unsigned int OUTPUT_DELAY_SAMPLES = 16384+8192;
    static const unsigned int OUTPUT_DELAY_BIAS_SAMPLES = 4096;
    
    std::array<stereo_float, DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES> dnnInputData1;
    std::array<stereo_float, DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES> dnnInputData2;
    std::array<float, OUTPUT_DELAY_SAMPLES> outBufferL;
    std::array<float, OUTPUT_DELAY_SAMPLES> outBufferR;    // 出力書き込み用配列
    
    ONNXMorpherInferenceThread* pInferenceThread;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HARDAudioProcessor)
};
