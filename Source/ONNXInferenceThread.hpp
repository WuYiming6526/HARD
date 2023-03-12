//
//  ProcessThread.hpp
//  Morpher - Shared Code
//
//  Created by 呉いみん on 2023/02/16.
//

#ifndef ONNXInferenceThread_hpp
#define ONNXInferenceThread_hpp

#include <JuceHeader.h>
#include "DataStructure.h"
#include <onnxruntime_cxx_api.h>
#include <array>

class ONNXMorpherInferenceThread: public juce::Thread
{
public:
    ONNXMorpherInferenceThread();
    ~ONNXMorpherInferenceThread() override;
    void run() override;
    void run_warmup(int n_iter);
    bool threadIsInferring(){return isInferring;}
    void requestInference(stereo_float input1[], stereo_float input2[], float rhythmFader, float harmonyFader, float sourceGainFader, float sidechainGainFader, FifoBuffer* outputBuffer);
private:
    bool isInferring;
    juce::CriticalSection critical;
    
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session_{nullptr};
    
    bool inputIsEmpty();
    
    static const unsigned int DNN_INPUT_SAMPLES = 8192;
    static const unsigned int DNN_INPUT_CACHE_SAMPLES = 8192;
    static const unsigned int OVERLAP_SAMPLES = 1024;
    static const unsigned int DNN_OUTPUT_DROP_HEAD_SAMPLES = 3072;
    
    std::array<stereo_float, DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES> inputWav1 = {0.0};
    std::array<stereo_float, DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES> inputWav2 = {0.0};
    std::array<stereo_float, DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES> outputWav = {0.0};
    
    std::array<float,(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES)*6> inputWavArray={0.0};
    std::array<float,(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES)*2> outputWavArray={0.0};
    
    const std::array<int64_t, 3> inputShape = {1, 6, DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES};
    const std::array<int64_t, 3> outputShape = {1, 2, DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES};
    const std::array<const char*, 1> dnnInputNames = {"input"};
    const std::array<const char*, 1> dnnOutputNames = {"output"};
    
    float faderValue;
    float rhythmFaderValue;
    float harmonyFaderValue;
    float sourceGain;
    float sidechainGain;
    FifoBuffer* pOutputBuffer;
    
};

#endif /* ProcessThread_hpp */
