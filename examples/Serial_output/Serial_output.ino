/*
    Strain gauge amplifier V1.0 29/12/15
    An instrumentation amplifier (AD8553) is used to amplify
    the output of a strain gauge, this is then fed into an ADC
    (ADS1118). This is a dual differential 16 bit ADC.
    The ADC will be used in continuous mode.
*/

#include <SPI.h>
#include "loadCell.h"

const float g_gain = 0.348620;    //kg per bit

//Create an instance of the loadCell class
//and set pin 7 as the chip select pin
loadCell amp(7);

void setup()
{
    Serial.begin(115200);
    Serial.println("Strain gauge amp");
    
    amp.init_ADC();
    amp.tare();
    amp.setFloatMultiplier(g_gain);
}

unsigned long time = 0;
unsigned long lastADCTime = 0;
const unsigned long ADCInterval = 4;

void loop()
{
    time = millis();
    
    //if difference between last time and now is greater
    //than the interval, proceed.
    if ((time - lastADCTime) >= ADCInterval) {
        lastADCTime = time;
        
        Serial.print(millis());
        Serial.print(",");
        Serial.print(amp.getFloatValue());
        Serial.println(",");
    }
}
