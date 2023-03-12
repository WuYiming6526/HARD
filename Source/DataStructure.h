//
//  DataStructure.h
//  Morpher
//
//  Created by 呉いみん on 2023/02/16.
//

#ifndef DataStructure_h
#define DataStructure_h

#include <JuceHeader.h>
#include <array>

typedef struct stereo_float
{
    float l;
    float r;
    stereo_float operator +(float rhs)
    {
        stereo_float x;
        x.l = this->l + rhs;
        x.r = this->r + rhs;
        return x;
    }
    stereo_float operator -(float rhs)
    {
        stereo_float x;
        x.l = this->l - rhs;
        x.r = this->r - rhs;
        return x;
    }
    stereo_float operator *(float rhs)
    {
        stereo_float x;
        x.l = this->l * rhs;
        x.r = this->r * rhs;
        return x;
    }
    stereo_float operator +(stereo_float rhs)
    {
        stereo_float x;
        x.l = this->l + rhs.l;
        x.r = this->r + rhs.r;
        return x;
    }
    stereo_float operator -(stereo_float rhs)
    {
        stereo_float x;
        x.l = this->l - rhs.l;
        x.r = this->r - rhs.r;
        return x;
    }
    stereo_float operator *(stereo_float rhs)
    {
        stereo_float x;
        x.l = this->l * rhs.l;
        x.r = this->r * rhs.r;
        return x;
    }
};


inline void stereo2array(stereo_float arr[], float out[], int numElements)
{
    for (int i = 0; i < numElements; i++)
    {
        out[i] = arr[i].l;
        out[i + numElements] = arr[i].r;
    }
}

inline void array2stereo(float arr[], stereo_float out[], int numElements)
{
    for (int i = 0; i < numElements; i++)
    {
        out[i].l = arr[i];
        out[i].r = arr[i + numElements];
    }
}




inline void copyArrayMonoToStereo(float arr_l[], float arr_r[], stereo_float arr_stereo[], int numSamples)
{
    for(int i=0;i<numSamples;i++)
    {
        arr_stereo[i].l = arr_l[i];
        arr_stereo[i].r = arr_r[i];
    }
}

inline void copyArrayStereoToMono(stereo_float arr_stereo[], float arr_l[], float arr_r[], int numSamples)
{
    for(int i=0;i<numSamples;i++)
    {
        arr_l[i] = arr_stereo[i].l;
        arr_r[i] = arr_stereo[i].r;
    }
}


struct FifoBuffer
{
    static const unsigned int BUFFER_SIZE = 65536;
    std::array<stereo_float, BUFFER_SIZE> buffer={0.0};
    juce::AbstractFifo abstractFifo{BUFFER_SIZE};
    
    void clearBuffer()
    {
        abstractFifo.reset();
    }
    
    void fillZeros(int numData)
    {
        stereo_float zeros[BUFFER_SIZE] = {0.0,0.0};
        pushData(zeros, numData);
    }
    
    void pushDataOverlap(stereo_float data[], int numData)
    {
        jassert(abstractFifo.getNumReady()>=numData);
        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite(numData, start1, size1, start2, size2);
        int i_data=0;
        for(int i=start1-numData;i<start1;i++)
        {
            float weight = (float)i_data/(float)numData;
            int i_buffer = i;
            if(i<0){i_buffer=BUFFER_SIZE+i;}
            buffer[i_buffer]=buffer[i_buffer]*(1.0f-weight) + (data[i_data]*weight);
            i_data++;
        }
    }
    
    void pushData(const stereo_float data[], int numData)
    {
        jassert(abstractFifo.getFreeSpace()>=numData);
        
        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite(numData, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            for (int i = 0; i < size1; i++)
            {
                buffer[start1 + i]=data[i];
            }
        }
        if (size2 > 0)
        {
            for (int i = 0; i < size2; i++)
            {
                buffer[start2 + i]=data[size1+i];
            }
        }
        abstractFifo.finishedWrite(numData);
    }
    
    void pushData(const float data_l[], const float data_r[], int numData)
    {
        jassert(abstractFifo.getFreeSpace()>=numData);
        
        int start1, size1, start2, size2;
        abstractFifo.prepareToWrite(numData, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            for (int i = 0; i < size1; i++)
            {
                buffer[start1 + i].l = data_l[i];
                buffer[start1 + i].r = data_r[i];
             }
        }
        if (size2 > 0)
        {
            for (int i = 0; i < size2; i++)
            {
                buffer[start2 + i].l = data_l[size1+i];
                buffer[start2 + i].r = data_r[size1+i];
            }
        }
        abstractFifo.finishedWrite(numData);
    }

    void readData(stereo_float data[], int numData, int numRead)
    {
        jassert(abstractFifo.getNumReady()>=numData);
        
        int start1, size1, start2, size2;
        abstractFifo.prepareToRead(numData, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            for (int i = 0; i < size1; i++)
            {
                data[i] = buffer[start1 + i];
            }
        }
        if (size2 > 0)
        {
            for (int i = 0; i < size2; i++)
            {
                data[size1+i] = buffer[start2 + i];
            }
        }
        abstractFifo.finishedRead(numRead);
    }
    
    void readData(float data_l[], float data_r[], int numData, int numRead)
    {
        jassert(abstractFifo.getNumReady()>=numRead);
        
        int start1, size1, start2, size2;
        abstractFifo.prepareToRead(numData, start1, size1, start2, size2);
        
        if (size1 > 0)
        {
            for (int i = 0; i < size1; i++)
            {
                data_l[i] = buffer[start1 + i].l;
                data_r[i] = buffer[start1 + i].r;
            }
        }
        if (size2 > 0)
        {
            for (int i = 0; i < size2; i++)
            {
                data_l[size1+i] = buffer[start2 + i].l;
                data_r[size1+i] = buffer[start2 + i].r;
            }
        }
        abstractFifo.finishedRead(numRead);
    }
    
    int getBufferSize()
    {
        return abstractFifo.getNumReady();
    }
};


#endif /* DataStructure_h */
