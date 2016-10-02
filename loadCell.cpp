/*
    loadCell.cpp
*/

#include "Arduino.h"
#include "loadCell.h"
#include <SPI.h>

//  Initialise the config bytes for the ADC
//  High byte
//  Bit:    Function:
//  7       SS - single shot
//  6       MUX2 - multiplexer
//  5       MUX1 - we want these set to 011
//  4       MUX0   AInP is AIn2 and AInN is AIn3
//  3       PGA2 - see setGain
//  2       PGA1
//  1       PGA0
//  0       Mode - 0 for continous conversions
//
//  Low byte
//  Bit:    Function:
//  7       DR2 - Data Rate
//  6       DR1   Set to 111 for 860 SPS
//  5       DR0
//  4       Temp - Set to read temperature
//  3       Pull - Enabled a pullup on MISO
//  2       NOP1 - No operation. Must be set to 01 to
//  1       NOP0   update the config bytes
//  0       Not used
//  

// Private variables
byte m_CS_pin;
byte m_gain;    //1, 2, 4, 8, 16
int m_offset;   //Set with tare() or setOffset(offset)
byte m_configHighByte;
byte m_configLowByte;
//set by setFloatMultiplier() and used for getFloatValue()
float m_multiplier;
byte m_noOfAverages = 0;	//How many averages to take
byte m_averIndex;		//Index for where to store next reading
const byte _averageLength_ = 32;
int m_averageStore[_averageLength_];		//Readings are stored here
boolean m_bAverage = false;

//Forward declares
int getRawValue(boolean bUseOffset = true);
void tare(void);
void setFloatMultiplier(float multiplier);
float getFloatValue(void);
byte setGain(byte gain);
void setOffset(int offset);
float getTemperature(void);

// Private functions

void init_ADC(void);    //Setup config registers
int read_ADC(void);

loadCell::loadCell(byte pin)
{
    pinMode(pin, OUTPUT);    //Setup as output
    digitalWrite(pin, HIGH); //Set high to disable ADC
    m_CS_pin = pin;           //Store pin number in private variable
    m_noOfAverages = 0;
    m_averIndex = 0;
    for (byte ix = 0; ix < _averageLength_; ix++)	m_averageStore[ix] = 0;
    m_bAverage = false;
}

void loadCell::init_ADC(void)
{
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV64);
    SPI.setDataMode(SPI_MODE1);
    
    //Ain2 is AinP, Ain3 is AinN
    //±2.048V input range, continuous conversion
    m_configHighByte = 0x34;
    //860SPS
    m_configLowByte = 0xE2;
    m_offset = 0;
    
    digitalWrite(m_CS_pin, LOW);
    SPI.transfer(m_configHighByte);
    SPI.transfer(m_configLowByte);
    digitalWrite(m_CS_pin, HIGH);
}

//Returns the voltage (VinP - VinN)
//Range of -32,768 to 32,767 for the voltage
//range set by setGain() default ±2.048V
//This gives 62.5µV (micro Volts) per bit
int loadCell::read_ADC(void)
{
    byte tempHigh = 0;
    byte tempLow = 0;
    int result = 0;
    digitalWrite(m_CS_pin, LOW);
    tempHigh = SPI.transfer(m_configHighByte);
    tempLow = SPI.transfer(m_configLowByte);
    digitalWrite(m_CS_pin, HIGH);
    result = (int(tempHigh) << 8) | tempLow;
    return result;
}

byte loadCell::setGain(byte gain)
{
    byte PgaSetting = 0x01;    //Set to 1 for shifting later
    m_configHighByte &= 0xF1;    //Clear bits 1 - 3
    /*
    Of the config high byte, bits 3 - 1 control
    the PGA, Programmable Gain Amplifier
    Bit 3: PGA2
    Bit 2: PGA1
    Bit 1: PGA0
    Note on gain:
    The instrumentation amplifier that feeds the ADC has an
    output voltage between 0 and 4.096V. The offset voltage,
    which is the output voltage of the amplifer when VinP and
    VinN are equal, should be set so that the output is at 2.048V
    when the load cell is tared when no load on the load cell.
    If the load cell will only be used with force in one direction,
    Then the reference voltage should be set at 0V. If the output 
    decreases with an increasing load, then swap VinP and VinN so
    that the output of the amplifier, and ADC, increases with an
    increasing load. Look at the output of getRawValue(). 
    The Gain should then by set to 2 for a full scale range of ±2.048 V.
    
    PGA  Full Scale range        Gain
    1    001 = FS is ±4.096 V    1
    2    010 = FS is ±2.048 V    2    Default
    3    011 = FS is ±1.024 V    4
    4    100 = FS is ±0.512 V    8
    5    101 = FS is ±0.256 V    16
    */
    
    if ((gain > 0) && (gain <= 0x05))    {
        m_configHighByte |= (gain << 1);    //Set bits 1 - 3
        return (0x01 << (gain - 1));      //Return the set gain
    } else {
        m_configHighByte |= (0x02 << 1);    //Else set gain of 2
        return 2;                         //Return the set gain
    }
    //Returns the gain set, 1, 2, 4, 8, or 16
}

int loadCell::getRawValue(boolean bUseOffset)
{
    //2 byte
    int result = 0;
    //Use a long, 4 bytes, to stop wrapping
    long int tempLong = read_ADC();
    
    if (bUseOffset)    {
        tempLong = tempLong - m_offset;    //Apply offset
        //If the long has gone past the limits of an int, clip
        if (tempLong > 32767) tempLong = 32767;
        if (tempLong < -32768) tempLong = -32768;
    }
    
    result = int(tempLong);
    
    return result;
}
        
void loadCell::tare(void)
{
    long int tempOffset = 0;
    for (byte ix = 0; ix < 64; ix++)    {
        tempOffset += read_ADC();    //Average the ADC value 16 times
        delay(1);
    }
    
    m_offset = int(tempOffset / 64);
}

void loadCell::setFloatMultiplier(float multiplier)
{
    m_multiplier = multiplier;
}
        
float loadCell::getFloatValue(void)
{
    float fResult;
    if (m_bAverage)	{
     fResult = float(calcAverages(getRawValue(true)));
     }
    else fResult = float(getRawValue(true));
    return fResult * m_multiplier;
}

void loadCell::setOffset(int offset)
{
    m_offset = offset;
}

int loadCell::calcAverages(int newValue)
{
	long int tempLong = 0;
    m_averIndex++;
    if (m_averIndex >= m_noOfAverages)    m_averIndex = 0;
    m_averageStore[m_averIndex] = newValue;
    for (byte ix = 0; ix < m_noOfAverages; ix++)    {
        tempLong += m_averageStore[ix];
    }
    return int(tempLong / m_noOfAverages);
}

//0 - 32, 0 is no averaging
void loadCell::setNoOfAverages(byte averages)
{
	if (averages <= 32)	{
		m_noOfAverages = averages;
		m_bAverage = true;
	}
	if (averages == 0)	{
		m_bAverage = false;
	}
}

