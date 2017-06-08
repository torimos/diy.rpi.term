#include "SPI.h"
#include <wiringPi.h>

#ifdef USE_SPI_DEVICE
#include <fcntl.h>				//Needed for SPI port
#include <sys/ioctl.h>			//Needed for SPI port
#include <linux/spi/spidev.h>	//Needed for SPI port
#include <unistd.h>				//Needed for SPI port
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cstring>
#elif defined(USE_SPI_BCM2835)
#include <bcm2835.h>
#endif

SPI::SPI(uint8_t channel)
{
	_spiChannel = channel;
#ifdef USE_SPI_DEVICE
	_spiCSPin = _spiChannel == 0 ? 10 : 11;
#elif defined(USE_SPI_BCM2835)
	_spiCSPin = _spiChannel == 0 ? RPI_GPIO_P1_24 : RPI_GPIO_P1_26;
#endif
}
SPI::~SPI()
{
}

bool SPI::initialize(SPIDataModeEnum mode, SPIBitSizeOrderEnum bitsSizeOrder, SPIClockDividerEnum clockDivider)
{
	_spi_clockDivider = clockDivider;
#ifdef USE_SPI_DEVICE
	int status_value = -1;
	switch (mode)
	{
	case MODE_0:
		{
			_spi_mode = SPI_MODE_0;
			break;
		}
	case MODE_1:
		{
			_spi_mode = SPI_MODE_1;
			break;
		}
	case MODE_2:
		{
			_spi_mode = SPI_MODE_2;
			break;
		}
	case MODE_3:
		{
			_spi_mode = SPI_MODE_3;
			break;
		}
	}
	uint32_t spi_speed = 250000 / clockDivider;
	switch (clockDivider)
	{
	case CLOCK_DIVIDER_1:
	case CLOCK_DIVIDER_65536:
		{
			spi_speed = 4000;
			break;
		}
	}
	if (_spiChannel == 0)
	{
		_spiFileHandle = open(std::string("/dev/spidev0.0").c_str(), O_RDWR);
	}
	else 
	{
		_spiFileHandle = open(std::string("/dev/spidev0.1").c_str(), O_RDWR);
	}
	if (_spiFileHandle < 0) return false;
	
	status_value = ioctl(_spiFileHandle, SPI_IOC_WR_MODE, &_spi_mode);
	if (status_value < 0) return false;
	
	status_value = ioctl(_spiFileHandle, SPI_IOC_RD_MODE, &_spi_mode);
	if (status_value < 0) return false;
	
	status_value = ioctl(_spiFileHandle, SPI_IOC_WR_BITS_PER_WORD, &_spi_bitsPerWord);
	if (status_value < 0) return false;
	
	status_value = ioctl(_spiFileHandle, SPI_IOC_RD_BITS_PER_WORD, &_spi_bitsPerWord);
	if (status_value < 0) return false;
	
	status_value = ioctl(_spiFileHandle, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
	if (status_value < 0) return false;
	
	status_value = ioctl(_spiFileHandle, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
	if (status_value < 0) return false;
	
#elif defined(USE_SPI_BCM2835)
	if (!bcm2835_init()) return false;
	bcm2835_spi_begin();
	switch (mode)
	{
	case MODE_0:
		{
			_spi_mode = BCM2835_SPI_MODE0;
			break;
		}
	case MODE_1:
		{
			_spi_mode = BCM2835_SPI_MODE1;
			break;
		}
	case MODE_2:
		{
			_spi_mode = BCM2835_SPI_MODE2;
			break;
		}
	case MODE_3:
		{
			_spi_mode = BCM2835_SPI_MODE3;
			break;
		}
	}
	switch (bitsSizeOrder)
	{
	case SPI_8BIT_MSB:
		{
			_spi_bitsOrder = BCM2835_SPI_BIT_ORDER_MSBFIRST;
			_spi_bitsPerWord = 8;
			break;
		}
	case SPI_8BIT_LSB:
		{
			_spi_bitsOrder = BCM2835_SPI_BIT_ORDER_LSBFIRST;
			_spi_bitsPerWord = 8;
			break;
		}
	case SPI_16BIT_MSB:
		{
			_spi_bitsOrder = BCM2835_SPI_BIT_ORDER_MSBFIRST;
			_spi_bitsPerWord = 16;
			break;
		}
	case SPI_16BIT_LSB:
		{
			_spi_bitsOrder = BCM2835_SPI_BIT_ORDER_LSBFIRST;
			_spi_bitsPerWord = 16;
			break;
		}
	}
	bcm2835_spi_setBitOrder(_spi_bitsOrder);      // The default
	bcm2835_spi_setDataMode(_spi_mode);                   // The default
	bcm2835_spi_setClockDivider(_spi_clockDivider); // The default
	bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);                  // The default
	bcm2835_gpio_fsel(_spiCSPin, BCM2835_GPIO_FSEL_OUTP);
#endif
	return true;
}

void SPI::deinitialize()
{
#ifdef USE_SPI_DEVICE
	close(_spiFileHandle);
#elif defined(USE_SPI_BCM2835)
	bcm2835_spi_end();
	bcm2835_close();
#endif
}

void SPI::begin()
{
#ifdef USE_SPI_DEVICE
	_spiTriggerCS = true;
	uint8_t buffer = 0;
	write((uint8_t*)&buffer, 0);
#elif defined(USE_SPI_BCM2835)
	bcm2835_gpio_write(_spiCSPin, LOW);
#endif
}

void SPI::end()
{
#ifdef USE_SPI_DEVICE
	_spiTriggerCS = false;
	uint8_t buffer = 0;
	write((uint8_t*)&buffer, 0);
#elif defined(USE_SPI_BCM2835)
	bcm2835_gpio_write(_spiCSPin, HIGH);
#endif
}

int SPI::write(uint8_t* data, uint32_t dataSize)
{
	int retVal = -1;
#ifdef USE_SPI_DEVICE
	struct spi_ioc_transfer spi;
	memset(&spi, 0, sizeof(spi));
	spi.tx_buf        = (unsigned long)(data); // transmit from "data"
	spi.rx_buf        = (unsigned long)(data); // receive into "data"
	spi.len           = dataSize;
	spi.cs_change = _spiTriggerCS;
	retVal = ioctl(_spiFileHandle, SPI_IOC_MESSAGE(dataSize), &spi);

	if (retVal < 0)
	{
		perror("Error - Problem transmitting spi data..ioctl");
		exit(1);
	}

#elif defined(USE_SPI_BCM2835)
	bcm2835_spi_writenb((char*)data, dataSize);
	retVal = dataSize;
#endif
	return retVal;
}

uint16_t SPI::transfer(uint16_t data)
{
#ifdef USE_SPI_DEVICE
	uint8_t buffer = data;
	write((uint8_t*)&buffer, _spi_bitsPerWord >> 3);
	return buffer;
#elif defined(USE_SPI_BCM2835)
	data = bcm2835_spi_transfer(data);
#endif
}