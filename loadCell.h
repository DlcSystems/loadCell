/*
    loadCell.h - library for interfacing with 
    load cell instrumentation amplifier.
    
    An instrumentation amplifier (AD8553) is used to amplify
    the output of a strain gauge, this is then fed into an ADC
    (ADS1118). This is a dual differential 16 bit ADC.
    
    The ADC will be used in continuous mode. When a conversion is
    complete, MISO will be asserted high for 8uS. The output of
    the ADC will be positive only as the AINn input is connected
    to ground rather than a voltage divider (config).
*/

#ifndef _loadCell_H_
#define _loadCell_H_

#include "Arduino.h"

class loadCell
{
    public:
        // Public functions
        
        /******* Functions required for normal use *******/
        
        //This must be called to initiate the ADC
        loadCell(byte pin);
        
        void init_ADC(void);    //Setup config registers
        
        //Returns raw ADC output
        //if bUseOffset is true, the offset from tare() or
        //setOffset() will be applied, if false, no offset applied
        int getRawValue(boolean bUseOffset = true);
        
        //Tare now, so subsequent readings will zeroed
        void tare(void);
        
        //Set the float to multiply the raw value by
        //If 1 least significant bit from ADC equals 1g, in your code
        //call setFloatMultiplier(1.0) to read grams back, or 0.001
        //to read Kgs back when you call getFloatValue()
        void setFloatMultiplier(float multiplier);
        
        //Returns the raw value multiplied by setFloatGain(float gain)
        float getFloatValue(void);
        
        /****** End of required functions ********/
        
        /****** Extended functions for more advanced features *****/
        
        //Set gain of ADC, 1, 2, 4, 8, 16
        //Returns the gain set
        //Default is 2, ±2.048V
        byte setGain(byte gain);
        
        //Offset in raw steps - this overrides the raw value tare() sets
        void setOffset(int offset);
        void setNoOfAverages(byte averages);	//0 - 32, 0 is no averaging
        
    private:
        // Private functions
        
        int read_ADC(void);     //Get raw ADC data, int16
        int calcAverages(int newValue);	//Takes a new value and returns rolling average
        
        // Private variables 
        // m_ refers to member, i.e. member of this class
        // as apose to g_ for global
        byte m_CS_pin;
        byte m_gain;    //1, 2, 4, 8, 16
        int m_offset;   //Set with tare() or setOffset(offset)
        byte m_configHighByte;
        byte m_configLowByte;
        //set by setFloatMultiplier() and used for getFloatValue()
        float m_multiplier;
};

#endif
