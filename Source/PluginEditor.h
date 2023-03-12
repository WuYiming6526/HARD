/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

//==============================================================================
/**
*/

class HorizontalSliderLookAndFeel:public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override
    {
        juce::Point<float> startPoint ((float)x, (float)y + (float)height*0.5f);
        juce::Point<float> centrePoint ((float)x + (float)width*0.5f, (float)y + (float)height*0.5f);
        juce::Point<float> thumbPoint (sliderPos, startPoint.y);
        
        g.setColour(juce::Colours::silver);
        g.drawLine((float)x, startPoint.y-15.0f, (float)x, startPoint.y+15.0f);
        g.drawLine((float)x+(float)width, startPoint.y-15.0f, (float)x+(float)width, startPoint.y+15.0f);
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(juce::Rectangle<float>((float)width+5.0f, (float)height*0.05f).withCentre(centrePoint), 5.0f);
        
        g.setColour(juce::Colours::dimgrey);
        g.fillRect(juce::Rectangle<float>(10.0f, 30.0f).withCentre(thumbPoint));
        g.setColour(juce::Colours::white);
        g.drawLine(thumbPoint.x, thumbPoint.y-15.0f, thumbPoint.x, thumbPoint.y+15.0f);

    }
};

class VerticalSliderLookAndFeel:public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override
    {
        juce::Point<float> centrePoint ((float)x + (float)width*0.5f, (float)y + (float)height*0.5f);
        juce::Point<float> thumbPoint (centrePoint.x, sliderPos);
        
        g.setColour(juce::Colours::silver);
        g.drawLine(centrePoint.x-15.0f, (float)y, centrePoint.x+15.0f, (float)y);
        g.drawLine(centrePoint.x-15.0f, (float)y+(float)height, centrePoint.x+15.0f, (float)y+(float)height);
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(juce::Rectangle<float>(width*0.05f, height+7.0f).withCentre(centrePoint), 5.0f);
        g.setColour(juce::Colours::dodgerblue);
        g.fillRoundedRectangle(juce::Rectangle<float>(width*0.05f, height+7.0f).withCentre(centrePoint).withTrimmedTop(sliderPos-(float)y), 5.0f);
        
        g.setColour(juce::Colours::dimgrey);
        g.fillRect(juce::Rectangle<float>(30.0f, 10.0f).withCentre(thumbPoint));
        g.setColour(juce::Colours::white);
        g.drawLine(thumbPoint.x-15.0f, thumbPoint.y, thumbPoint.x+15.0f, thumbPoint.y);
    }
};


class HARDAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    HARDAudioProcessorEditor (HARDAudioProcessor&);
    ~HARDAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void setSliderValues(float harmony, float rhythm)
    {
        harmonySlider.setValue(harmony);
        rhythmSlider.setValue(rhythm);
    }
    
    void parameterChanged(const juce::String &parameterID, float newValue) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    HARDAudioProcessor& audioProcessor;
    
    juce::AudioProcessorValueTreeState& valueTreeState;
    
    juce::Slider harmonySlider;
    juce::Slider rhythmSlider;
    juce::Slider sourceGainSlider;
    juce::Slider sidechainGainSlider;
    juce::TextButton syncButton;
    
    std::unique_ptr<SliderAttachment> harmonySliderAttachment;
    std::unique_ptr<SliderAttachment> rhythmSliderAttachment;
    std::unique_ptr<SliderAttachment> sourceGainAttachment;
    std::unique_ptr<SliderAttachment> sidechainGainAttachment;
    std::unique_ptr<ButtonAttachment> syncButtonAttachment;
    
    HorizontalSliderLookAndFeel horizontalSliderLookAndFeel;
    VerticalSliderLookAndFeel verticalSliderLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HARDAudioProcessorEditor)
};
