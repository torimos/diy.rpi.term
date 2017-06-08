#pragma once
#include "def.h"

#define USE_SPI_DEVICE
//#define USE_SPI_BCM2835

typedef enum
{
	MODE_0 = 0x00, //CPOL:0, CPHA:0
	MODE_1 = 0x02, //CPOL:0, CPHA:1
	MODE_2 = 0x04, //CPOL:1, CPHA:0
	MODE_3 = 0x08, //CPOL:1, CPHA:1
} SPIDataModeEnum;

typedef enum
{
	SPI_8BIT_MSB  = 0x00,
	SPI_16BIT_MSB = 0x02,
	SPI_8BIT_LSB  = 0x04,
	SPI_16BIT_LSB = 0x08
} SPIBitSizeOrderEnum;

typedef enum
{
	CLOCK_DIVIDER_65536 = 0,       ///< 65536 = 256us = 4kHz
	CLOCK_DIVIDER_32768 = 32768,   ///< 32768 = 126us = 8kHz
	CLOCK_DIVIDER_16384 = 16384,   ///< 16384 = 64us = 15.625kHz
	CLOCK_DIVIDER_8192  = 8192,    ///< 8192 = 32us = 31.25kHz
	CLOCK_DIVIDER_4096  = 4096,    ///< 4096 = 16us = 62.5kHz
	CLOCK_DIVIDER_2048  = 2048,    ///< 2048 = 8us = 125kHz
	CLOCK_DIVIDER_1024  = 1024,    ///< 1024 = 4us = 250kHz
	CLOCK_DIVIDER_512   = 512,     ///< 512 = 2us = 500kHz
	CLOCK_DIVIDER_256   = 256,     ///< 256 = 1us = 1MHz
	CLOCK_DIVIDER_128   = 128,     ///< 128 = 500ns = = 2MHz
	CLOCK_DIVIDER_64    = 64,      ///< 64 = 250ns = 4MHz
	CLOCK_DIVIDER_32    = 32,      ///< 32 = 125ns = 8MHz
	CLOCK_DIVIDER_16    = 16,      ///< 16 = 50ns = 20MHz
	CLOCK_DIVIDER_8     = 8,       ///< 8 = 25ns = 40MHz
	CLOCK_DIVIDER_4     = 4,       ///< 4 = 12.5ns 80MHz
	CLOCK_DIVIDER_2     = 2,       ///< 2 = 6.25ns = 160MHz
	CLOCK_DIVIDER_1     = 1,       ///< 0 = 256us = 4kHz
} SPIClockDividerEnum;

class SPI
{
public:
	SPI(uint8_t channel);
	~SPI();
	bool initialize(SPIDataModeEnum mode, SPIBitSizeOrderEnum bitsSizeOrder, SPIClockDividerEnum clockDivider);
	void deinitialize();
	uint16_t transfer(uint16_t data);
	int write(uint8_t* data, uint32_t dataSize);
	void begin();
	void end();
private:
	uint8_t _spiChannel;
	uint8_t _spiCSPin;
	uint8_t _spi_mode;
	uint8_t _spi_bitsOrder;
	uint8_t _spi_bitsPerWord;
	uint16_t _spi_clockDivider;
	
#if defined(USE_SPI_DEVICE)
	int _spiFileHandle;
	bool _spiTriggerCS;
#elif defined(USE_SPI_BCM2835)
#endif
};