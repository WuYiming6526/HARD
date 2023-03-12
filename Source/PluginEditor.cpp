/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HARDAudioProcessorEditor::HARDAudioProcessorEditor (HARDAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState(p.parameters)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (480 , 360);
    
    harmonySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    harmonySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    harmonySlider.setTextValueSuffix("Harmony");
    harmonySlider.setLookAndFeel(&horizontalSliderLookAndFeel);
    
    rhythmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    rhythmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    rhythmSlider.setTextValueSuffix("Rhythm");
    rhythmSlider.setLookAndFeel(&horizontalSliderLookAndFeel);
    
    sourceGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    sourceGainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sourceGainSlider.setTextValueSuffix("SourceGain");
    sourceGainSlider.setLookAndFeel(&verticalSliderLookAndFeel);
    
    sidechainGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    sidechainGainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sidechainGainSlider.setTextValueSuffix("SidechainGain");
    sidechainGainSlider.setLookAndFeel(&verticalSliderLookAndFeel);
    
    syncButton.setButtonText("sync");
    syncButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::dodgerblue);
    
    addAndMakeVisible(&harmonySlider);
    addAndMakeVisible(&rhythmSlider);
    addAndMakeVisible(&sourceGainSlider);
    addAndMakeVisible(&sidechainGainSlider);
    addAndMakeVisible(&syncButton);
    
    syncButton.setClickingTogglesState(true);
    
    harmonySliderAttachment.reset(new SliderAttachment(p.parameters, "harmony", harmonySlider));
    rhythmSliderAttachment.reset(new SliderAttachment(p.parameters, "rhythm", rhythmSlider));
    sourceGainAttachment.reset(new SliderAttachment(p.parameters, "sourceGain", sourceGainSlider));
    sidechainGainAttachment.reset(new SliderAttachment(p.parameters, "sidechainGain", sidechainGainSlider));
    syncButtonAttachment.reset(new ButtonAttachment(p.parameters, "sync", syncButton));
    
    valueTreeState.addParameterListener("harmony", this);
    valueTreeState.addParameterListener("rhythm", this);
    valueTreeState.addParameterListener("sync", this);
    
}

HARDAudioProcessorEditor::~HARDAudioProcessorEditor()
{
}

//==============================================================================
void HARDAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colour(32, 32, 32));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font("Avenir", 40.0f, juce::Font::plain));
    g.drawFittedText ("HARD Audio Remixer", 0, 20, getWidth(), 30, juce::Justification::centred, 1);
    
    g.setFont(juce::Font("Avenir", 20.0f, juce::Font::plain));
    g.drawFittedText("Source\nGain", 10, 70, 100, 50, juce::Justification::centred,1);
    g.drawFittedText("Sidechain\nGain", 370, 70, 100, 50, juce::Justification::centred,1);
    g.drawFittedText("Harmony", 200, 120, 80, 40, juce::Justification::centred,1);
    g.drawFittedText("Rhythm", 200, 230, 80, 40, juce::Justification::centred,1);
}

void HARDAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    harmonySlider.setBounds(120, 130, 240, 80);
    rhythmSlider.setBounds(120, 240, 240, 80);
    sourceGainSlider.setBounds(20, 120, 80, 200);
    sidechainGainSlider.setBounds(380, 120, 80, 200);
    syncButton.setBounds(220, 200, 40, 20);
}

void HARDAudioProcessorEditor::parameterChanged(const juce::String &parameterID, float newValue)
{
    bool isSyncMode = *valueTreeState.getRawParameterValue("sync") > 0.5f;
    if (parameterID=="sync")
    {
        if (isSyncMode)
        {
            rhythmSlider.setValue(*valueTreeState.getRawParameterValue("harmony"));
        }
    }
    else if(isSyncMode)
    {
        if (parameterID=="harmony")
        {
            rhythmSlider.setValue(newValue);
        }
        else if (parameterID=="rhythm")
        {
            harmonySlider.setValue(newValue);
        }
    }
}

