#include "ADS1x15.h"
#include "I2Cdev.h"
/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {
	bool r = I2Cdev::writeWord(i2cAddress, reg, value);
}

/**************************************************************************/
/*!
    @brief  Reads 16-bits to the specified destination register
*/
/**************************************************************************/
static uint16_t readRegister(uint8_t i2cAddress, uint8_t reg) {
	uint16_t value =  0;
	bool r = I2Cdev::readWord(i2cAddress, reg, &value);
	return value;
}

/**************************************************************************/
/*!
    @brief  Instantiates a new ADS1x15 class w/appropriate properties
*/
/**************************************************************************/
ADS1x15::ADS1x15(uint8_t i2cAddress, uint8_t bitShift) 
{
	m_i2cAddress = i2cAddress;
	m_bitShift = bitShift;
}

/**************************************************************************/
/*!
    @brief  Sets up the HW (reads coefficients values, etc.)
*/
/**************************************************************************/
void ADS1x15::begin() {
}

/**************************************************************************/
/*!
    @brief  Sets the gain and input voltage range
*/
/**************************************************************************/
void ADS1x15::setGain(adsGain_t gain)
{
  m_gain = gain;
}

/**************************************************************************/
/*!
    @brief  Gets a gain and input voltage range
*/
/**************************************************************************/
adsGain_t ADS1x15::getGain()
{
  return m_gain;
}

/**************************************************************************/
/*!
    @brief  Sets the Samples per Second setting
*/
/**************************************************************************/
void ADS1x15::setSPS(adsSPS_t SPS)
{
  m_SPS = SPS;
}

/**************************************************************************/
/*!
    @brief  Gets the Samples per Second setting
*/
/**************************************************************************/
adsSPS_t ADS1x15::getSPS()
{
  return m_SPS;
}

/**************************************************************************/
/*!
    @brief  Given the input pin (channel) Determines the MUX bits in the config 
	        register for single ended operations
*/
/**************************************************************************/
uint16_t getSingleEndedConfigBitsForMUX(uint8_t channel) {
  uint16_t c = 0;
  switch (channel)
  {
    case (0):
      c = ADS1X15_REG_CONFIG_MUX_SINGLE_0;
      break;
    case (1):
      c = ADS1X15_REG_CONFIG_MUX_SINGLE_1;
      break;
    case (2):
      c = ADS1X15_REG_CONFIG_MUX_SINGLE_2;
      break;
    case (3):
      c = ADS1X15_REG_CONFIG_MUX_SINGLE_3;
      break;
  }
  return c;
}

/**************************************************************************/
/*!
    @brief  Gets a single-ended ADC reading from the specified channel in Volts
*/
/**************************************************************************/
float ADS1x15::readADC_SingleEnded_V(uint8_t channel) {
	return (float)readADC_SingleEnded(channel) * voltsPerBit();
}

/**************************************************************************/
/*!
    @brief  Gets a single-ended ADC reading from the specified channel
*/
/**************************************************************************/
int16_t ADS1x15::readADC_SingleEnded(uint8_t channel) {
  if (channel > 3)
  {
    return 0;
  }
  
  // Start with default values
  uint16_t config = ADS1X15_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1X15_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1X15_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1X15_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1X15_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;
  
  // Set Samples per Second
  config |= m_SPS;

  // Set single-ended input channel
  config |= getSingleEndedConfigBitsForMUX(channel);

  // Set 'start single-conversion' bit
  config |= ADS1X15_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONFIG, config);
  
  // Wait for the conversion to complete
  waitForConversion();

  return getLastConversionResults();                                      // conversion delay is included in this method
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN0) and N (AIN1) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t ADS1x15::readADC_Differential(adsDiffMux_t regConfigDiffMUX) {
  // Start with default values
  uint16_t config = ADS1X15_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1X15_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1X15_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1X15_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1X15_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;
  
  // Set Samples per Second
  config |= m_SPS;
                    
  // Set channels
  config |= regConfigDiffMUX;          // set P and N inputs for differential

  // Set 'start single-conversion' bit
  config |= ADS1X15_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONFIG, config);
  
  // Wait for the conversion to complete
  waitForConversion();
  
  return getLastConversionResults();                                      // conversion delay is included in this method
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN0) and N (AIN1) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t ADS1x15::readADC_Differential_0_1() {
  return readADC_Differential(DIFF_MUX_0_1);                               // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN1) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t ADS1x15::readADC_Differential_0_3() {
  return readADC_Differential(DIFF_MUX_0_3);                               // AIN0 = P, AIN3 = N
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN1) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t ADS1x15::readADC_Differential_1_3() {
  return readADC_Differential(DIFF_MUX_1_3);                               // AIN1 = P, AIN3 = N
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN2) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t ADS1x15::readADC_Differential_2_3() {
	return readADC_Differential(DIFF_MUX_2_3);                               // AIN2 = P, AIN3 = N
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN0) and N (AIN1) input.  Generates
            a signed value since the difference can be either
            positive or negative.
			Applies the Volts per bit to return actual voltage
*/
/**************************************************************************/
float ADS1x15::readADC_Differential_0_1_V() {
  return (float) readADC_Differential(DIFF_MUX_0_1) * voltsPerBit();                               // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN0) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
			Applies the Volts per bit to return actual voltage
*/
/**************************************************************************/
float ADS1x15::readADC_Differential_0_3_V() {
  return (float) readADC_Differential(DIFF_MUX_0_3) * voltsPerBit();                               // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN1) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
			Applies the Volts per bit to return actual voltage
*/
/**************************************************************************/
float ADS1x15::readADC_Differential_1_3_V() {
  return (float) readADC_Differential(DIFF_MUX_1_3) * voltsPerBit();                               // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN2) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
			Applies the Volts per bit to return actual voltage
*/
/**************************************************************************/
float ADS1x15::readADC_Differential_2_3_V() {
  return (float) readADC_Differential(DIFF_MUX_2_3) * voltsPerBit();                               // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*!
    @brief  Sets up the comparator to operate in basic mode, causing the
            ALERT/RDY pin to assert (go from high to low) when the ADC
            value exceeds the specified threshold.
			The pin latches, call getLastConversionResults() to clear the latch.

            This will also set the ADC in continuous conversion mode.
*/
/**************************************************************************/
void ADS1x15::startComparator_SingleEnded(uint8_t channel, int16_t highThreshold)
{
  // Start with default values
  uint16_t config = ADS1X15_REG_CONFIG_CQUE_1CONV   | // Comparator enabled and asserts on 1 match
                    ADS1X15_REG_CONFIG_CLAT_LATCH   | // Latching mode
                    ADS1X15_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1X15_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1X15_REG_CONFIG_MODE_CONTIN;   // Continuous conversion mode

  // Set PGA/voltage range
  config |= m_gain;
  
  // Set Samples per Second
  config |= m_SPS;
                    
  // Set single-ended input channel
  config |= getSingleEndedConfigBitsForMUX(channel);

  // Set the high threshold register
  // Shift 12-bit results left 4 bits for the ADS1x15
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_HITHRESH, highThreshold << m_bitShift);
  
  // Set the high threshold register to the default
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_LOWTHRESH, ADS1X15_LOW_THRESHOLD_DEFAULT);

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONFIG, config);
  
}


/**************************************************************************/
/*!
    @brief  Sets up the comparator to operate in window mode, causing the
            ALERT/RDY pin to assert (go from high to low) when the ADC
            value exceeds the high threshold or drops below the low threshold.
			The pin latches, call getLastConversionResults() to clear the latch.

            This will also set the ADC in continuous conversion mode.
*/
/**************************************************************************/
void ADS1x15::startWindowComparator_SingleEnded(uint8_t channel, int16_t lowThreshold, int16_t highThreshold)
{
  // Start with default values
  uint16_t config = ADS1X15_REG_CONFIG_CQUE_1CONV   | // Comparator enabled and asserts on 1 match
                    ADS1X15_REG_CONFIG_CLAT_LATCH   | // Latching mode
                    ADS1X15_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1X15_REG_CONFIG_CMODE_WINDOW | // Window comparator 
                    ADS1X15_REG_CONFIG_MODE_CONTIN;   // Continuous conversion mode

  // Set PGA/voltage range
  config |= m_gain;
  
  // Set Samples per Second
  config |= m_SPS;
                    
  // Set single-ended input channel
  config |= getSingleEndedConfigBitsForMUX(channel);

  // Set the high threshold register
  // Shift 12-bit results left 4 bits for the ADS1x15
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_HITHRESH, highThreshold << m_bitShift);
  
  // Set the high threshold register to the default
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_LOWTHRESH, lowThreshold << m_bitShift);

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONFIG, config);
  
}


/**************************************************************************/
/*!
    @brief  Sets up continous coversion operatoin, causing the
            ALERT/RDY pin to assert (go from high to low) each time a conversion
			completes. Pin stays low for 8 micro seconds (per the datasheet)
*/
/**************************************************************************/
void ADS1x15::startContinuous_SingleEnded(uint8_t channel)
{
  // Initial single ended non-contunuous read primes the conversion buffer with a valid reading
  // so that the initial interrupts produced a correct result instead of a left over 
  // conversion result from previous operations.
  int16_t primingRead = readADC_SingleEnded(channel); 
  
  // Start with default values
  uint16_t config = ADS1X15_REG_CONFIG_CQUE_1CONV   | // Comparator enabled and asserts on 1 match
                    ADS1X15_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1X15_REG_CONFIG_MODE_CONTIN;   // Continuous conversion mode

  // Set PGA/voltage range
  config |= m_gain;
  
  // Set Samples per Second
  config |= m_SPS;
                    
  // Set single-ended input channel
  config |= getSingleEndedConfigBitsForMUX(channel);

  // Continuous mode is set by setting the most signigicant bit for the HIGH threshold to 1
  // and for the LOW threshold to 0.  This is accomlished by setting the HIGH threshold to the 
  // low default (a negative number) and the LOW threshold to the HIGH default (a positive number)
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_HITHRESH, ADS1X15_LOW_THRESHOLD_DEFAULT);
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_LOWTHRESH, ADS1X15_HIGH_THRESHOLD_DEFAULT);

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONFIG, config);
  
}

/**************************************************************************/
/*!
    @brief  Sets up Differential continous coversion operatoin, causing the
            ALERT/RDY pin to assert (go from high to low) each time a conversion
			completes. Pin stays low for 8 micro seconds (per the datasheet)
*/
/**************************************************************************/
void ADS1x15::startContinuous_Differential(adsDiffMux_t regConfigDiffMUX)
{
  // Initial Differential non-contunuous read primes the conversion buffer with a valid reading
  // so that the initial interrupts produced a correct result instead of a left over 
  // conversion result from previous operations.
  int16_t primingRead = readADC_Differential(regConfigDiffMUX); 
  
  // Start with default values
  uint16_t config = ADS1X15_REG_CONFIG_CQUE_1CONV   | // Comparator enabled and asserts on 1 match
                    ADS1X15_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1X15_REG_CONFIG_MODE_CONTIN;   // Continuous conversion mode

  // Set PGA/voltage range
  config |= m_gain;
  
  // Set Samples per Second
  config |= m_SPS;
                    
  // Set channels
  config |= regConfigDiffMUX;          // set P and N inputs for differential

  // Continuous mode is set by setting the most signigicant bit for the HIGH threshold to 1
  // and for the LOW threshold to 0.  This is accomlished by setting the HIGH threshold to the 
  // low default (a negative number) and the LOW threshold to the HIGH default (a positive number)
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_HITHRESH, ADS1X15_LOW_THRESHOLD_DEFAULT);
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_LOWTHRESH, ADS1X15_HIGH_THRESHOLD_DEFAULT);

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONFIG, config);
  
}

/**************************************************************************/
/*! 
    @brief  Sets up Differential continous coversion operation between the
            P (AIN1) and N (AIN3) input, causing the
            ALERT/RDY pin to assert (go from high to low) each time a conversion
			completes. Pin stays low for 8 micro seconds (per the datasheet)
*/
/**************************************************************************/
void ADS1x15::startContinuous_Differential_0_1() {
  startContinuous_Differential(DIFF_MUX_0_1);                             // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*! 
    @brief  Sets up Differential continous coversion operation between the
            P (AIN1) and N (AIN3) input, causing the
            ALERT/RDY pin to assert (go from high to low) each time a conversion
			completes. Pin stays low for 8 micro seconds (per the datasheet)
*/
/**************************************************************************/
void ADS1x15::startContinuous_Differential_0_3() {
  startContinuous_Differential(DIFF_MUX_0_3);                             // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*! 
    @brief  Sets up Differential continous coversion operation between the
            P (AIN1) and N (AIN3) input, causing the
            ALERT/RDY pin to assert (go from high to low) each time a conversion
			completes. Pin stays low for 8 micro seconds (per the datasheet)
*/
/**************************************************************************/
void ADS1x15::startContinuous_Differential_1_3() {
  startContinuous_Differential(DIFF_MUX_1_3);                             // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*! 
    @brief  Sets up Differential continous coversion operation between the
            P (AIN1) and N (AIN3) input, causing the
            ALERT/RDY pin to assert (go from high to low) each time a conversion
			completes. Pin stays low for 8 micro seconds (per the datasheet)
*/
/**************************************************************************/
void ADS1x15::startContinuous_Differential_2_3() {
  startContinuous_Differential(DIFF_MUX_2_3);                             // AIN0 = P, AIN1 = N
}

/**************************************************************************/
/*!
    @brief  Poll the device each millisecond until the conversion is done.  
	        Using delay is important for an ESP8266 becasue it yeilds to the
			allow network operations to run.
*/
/**************************************************************************/
void ADS1x15::waitForConversion()
{
  delay(0);                // delay(0) causes a yeild for ESP8266
  delayMicroseconds(10);   // Slight delay to ensure converstion started.  Probably not needed, but for safety
  do {
	  delay(0);            // delay(0) causes a yeild for ESP8266
	 } 
	 while (ADS1X15_REG_CONFIG_OS_BUSY == (readRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONFIG) & ADS1X15_REG_CONFIG_OS_MASK));
            // Stop when the config register OS bit changes to 1
}

/**************************************************************************/
/*!
    @brief  This function reads the last conversion
            results without changing the config value.
			
			After the comparator triggers, in order to clear the comparator, 
			we need to read the conversion results.  
*/
/**************************************************************************/
int16_t ADS1x15::getLastConversionResults()
{
  // Read the conversion results
  uint16_t res = readRegister(m_i2cAddress, ADS1X15_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == ADS1115_CONV_REG_BIT_SHIFT_0)            // for ADS1115
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1x15,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

/**************************************************************************/
/*!
    @brief  Return the volts per bit for based on gain.  Multiply the adc
            reading by the value returned here to get actual volts. 
*/
/**************************************************************************/
float ADS1x15::voltsPerBit()
{
	float v = 0;
	if (m_bitShift == ADS1015_CONV_REG_BIT_SHIFT_4) {            // for ADS1015
	  switch (m_gain)
	  {
		case (GAIN_TWOTHIRDS):
		  v = ADS1x15_VOLTS_PER_BIT_GAIN_TWOTHIRDS;
		  break;
		case (GAIN_ONE):
		  v = ADS1x15_VOLTS_PER_BIT_GAIN_ONE;
		  break;
		 case (GAIN_TWO):
		  v = ADS1x15_VOLTS_PER_BIT_GAIN_TWO;
		  break;
		case (GAIN_FOUR):
		  v = ADS1x15_VOLTS_PER_BIT_GAIN_FOUR;
		  break;
		case (GAIN_EIGHT):
		  v = ADS1x15_VOLTS_PER_BIT_GAIN_EIGHT;
		  break;
		case (GAIN_SIXTEEN):
		  v = ADS1x15_VOLTS_PER_BIT_GAIN_SIXTEEN;
		  break;
	  }
	} else                  // for ADS1115
	{  
	  switch (m_gain)
	  {
		case (GAIN_TWOTHIRDS):
		  v = ADS1115_VOLTS_PER_BIT_GAIN_TWOTHIRDS;
		  break;
		case (GAIN_ONE):
		  v = ADS1115_VOLTS_PER_BIT_GAIN_ONE;
		  break;
		 case (GAIN_TWO):
		  v = ADS1115_VOLTS_PER_BIT_GAIN_TWO;
		  break;
		case (GAIN_FOUR):
		  v = ADS1115_VOLTS_PER_BIT_GAIN_FOUR;
		  break;
		case (GAIN_EIGHT):
		  v = ADS1115_VOLTS_PER_BIT_GAIN_EIGHT;
		  break;
		case (GAIN_SIXTEEN):
		  v = ADS1115_VOLTS_PER_BIT_GAIN_SIXTEEN;
		  break;
	  }
	}
	return v;
}

