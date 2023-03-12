//
//  ProcessThread.cpp
//  Morpher - Shared Code
//
//  Created by 呉いみん on 2023/02/16.
//

#include "ONNXInferenceThread.hpp"
#define ONNX_FILENAME "morpher.onnx"

#define PERFORMANCE_COUNT_START()    static juce::PerformanceCounter    performanceCounter ("MORPHING", 1); \
performanceCounter.start();
#define PERFORMANCE_COUNT_END()    performanceCounter.stop();

ONNXMorpherInferenceThread::ONNXMorpherInferenceThread()
:juce::Thread("InferenceThread")
{
    const juce::File dir = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getChildFile("Contents/Resources");
    juce::String model_path = dir.getChildFile(ONNX_FILENAME).getFullPathName();
    
    session_ = Ort::Session(env, model_path.getCharPointer(), session_options);
    run_warmup(3);
    isInferring = false;
    startThread();
}

ONNXMorpherInferenceThread::~ONNXMorpherInferenceThread()
{
    stopThread(2000);
}

void ONNXMorpherInferenceThread::requestInference(stereo_float input1[], stereo_float input2[], float rhythmFader,float harmonyFader,float sourceGainFader, float sidechainGainFader, FifoBuffer *outputBuffer)
{
    memcpy(inputWav1.data(), input1, sizeof(stereo_float)*(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES));
    memcpy(inputWav2.data(), input2, sizeof(stereo_float)*(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES));
    rhythmFaderValue = rhythmFader;
    harmonyFaderValue = harmonyFader;
    sourceGain = sourceGainFader;
    sidechainGain = sidechainGainFader;
    pOutputBuffer = outputBuffer;
    isInferring = true;
    notify();
}

bool ONNXMorpherInferenceThread::inputIsEmpty()
{
    for(int i=0; i < DNN_INPUT_SAMPLES; i++)
    {
        if ((inputWav1[i].l != 0)and(inputWav2[i].l != 0)) {return false;}
    }
    return true;
}

void ONNXMorpherInferenceThread::run_warmup(int n_iter)
{
    for(int i=0;i<n_iter;i++)
    {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        std::vector<Ort::Value> ort_inputs;
        ort_inputs.emplace_back(Ort::Value::CreateTensor<float>(memory_info, inputWavArray.data(), 6*(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES), inputShape.data(), inputShape.size()));
        
        std::vector<Ort::Value> ort_outputs;
        ort_outputs.emplace_back(Ort::Value::CreateTensor<float>(memory_info, outputWavArray.data(), 2*(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES), outputShape.data(), outputShape.size()));
        
        session_.Run(Ort::RunOptions{nullptr}, dnnInputNames.data(), ort_inputs.data(), 1, dnnOutputNames.data(), ort_outputs.data(), 1);
    }
    
}

void ONNXMorpherInferenceThread::run()
{
    while (!threadShouldExit())
    {
        while(!isInferring)
        {
            // Wait for inference request
            wait(-1);
        }
        printf("Inference start. \n");
        float faderSum = rhythmFaderValue + harmonyFaderValue;
        
        if ((faderSum==0.0) or (faderSum==2.0) or (inputIsEmpty()))
        {
            for(int i=0;i<DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES;i++)
            {
                float weight = (faderSum)/2.0f;
                outputWav[i] = (inputWav1[i]*(1.0f-weight)*sourceGain) + (inputWav2[i]*weight*sidechainGain);
            }
        }
        else
        {
            PERFORMANCE_COUNT_START()
            int ch = DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES;
            for(int i=0; i<(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES); i++)
            {
                inputWavArray[0*ch+i] = inputWav1[i].l * sourceGain;
                inputWavArray[1*ch+i] = inputWav1[i].r * sourceGain;
                inputWavArray[2*ch+i] = inputWav2[i].l * sidechainGain;
                inputWavArray[3*ch+i] = inputWav2[i].r * sidechainGain;
                inputWavArray[4*ch+i] = harmonyFaderValue;
                inputWavArray[5*ch+i] = rhythmFaderValue;
            }
            auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
            std::vector<Ort::Value> ort_inputs;
            ort_inputs.emplace_back(Ort::Value::CreateTensor<float>(memory_info, inputWavArray.data(), 6*(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES), inputShape.data(), inputShape.size()));
            
            std::vector<Ort::Value> ort_outputs;
            ort_outputs.emplace_back(Ort::Value::CreateTensor<float>(memory_info, outputWavArray.data(), 2*(DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES), outputShape.data(), outputShape.size()));
            
            session_.Run(Ort::RunOptions{nullptr}, dnnInputNames.data(), ort_inputs.data(), 1, dnnOutputNames.data(), ort_outputs.data(), 1);
            
            for(int i=0;i<DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES;i++)
            {
                outputWav[i].l = outputWavArray[i];
                outputWav[i].r = outputWavArray[DNN_INPUT_SAMPLES+DNN_INPUT_CACHE_SAMPLES+i];
            }
            PERFORMANCE_COUNT_END()
        }
        
        {
            // oush outputWav into outout buffer
            const juce::ScopedLock lock(critical);
            pOutputBuffer->pushDataOverlap(&outputWav[DNN_OUTPUT_DROP_HEAD_SAMPLES], OVERLAP_SAMPLES);
            pOutputBuffer->pushData(&outputWav[DNN_OUTPUT_DROP_HEAD_SAMPLES+OVERLAP_SAMPLES], DNN_INPUT_SAMPLES);
            printf("Inference complete. \n");
            // Wait for next inference request
            isInferring = false;
        }
    }
}
