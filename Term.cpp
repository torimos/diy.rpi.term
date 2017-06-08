#include <wiringPi.h>
#include "SPI.h"
#include "ra8875.h"
#include <stdlib.h>
#include <stdio.h>

SPI spi0(0);
RA8875 tft(&spi0, 6);
uint16_t buffer1[800 * 480];
uint16_t buffer2[800 * 480];

bool spiTest()
{
	uint16_t r;
	if (!spi0.initialize(SPIDataModeEnum::MODE_0, SPIBitSizeOrderEnum::SPI_8BIT_MSB, SPIClockDividerEnum::CLOCK_DIVIDER_512)) return false;
	spi0.begin();
	r = spi0.transfer(0x80);
	r = spi0.transfer(0);
	spi0.end();
	spi0.begin();
	r = spi0.transfer(0x40);
	r = spi0.transfer(0);
	spi0.end();
	spi0.deinitialize();
	return r == 0x75;
}

int main(int argc, char *argv[])
{
	
	
//	pinMode(5, OUTPUT); //BL
//	pinMode(6, OUTPUT); //RESET
//	//pinMode(10, OUTPUT); //CS
//	digitalWrite(5, HIGH);
//	digitalWrite(6, HIGH);
//	//digitalWrite(10, HIGH);
//	
	if (tft.initialize(RA8875_800x480))
	{
		tft.touchEnable(true);
		tft.setMode(RA8875ModeEnum::TEXT);
		tft.textColor(RGB(0, 0xFF, 0), 0);
		tft.textWrite(0, 0, "Home Terminal v2.1");
	}
	
	int i = 0;
	while (i < 800 * 480)
	{
		buffer1[i] = rand() % 0xFFFF;
		buffer2[i] = rand() % 0xFFFF;
		i++;
	}
	
//	FILE *f;
//	unsigned char buffer[800*480*2];
//	int n;
//
//	f = fopen("filename.bin", "rb");
//	if (f)
//	{
//		n = fread(buffer1, 800 * 480 * 2, 1, f);
//	}
//	else
//	{
//	    // error opening file
//	}
	
	tft.setMode(RA8875ModeEnum::GRAPHIC);
	tft.drawImage((uint16_t*)(&buffer1), 0, 0, 800, 480);
	uint16_t x, y, c;
	i = 0;
	while (1)
	{
		tft.setMode(RA8875ModeEnum::TEXT);
		tft.touchRead(&x, &y);
		tft.textWrite(x, y, "%04d:%04d", x, y);
	}	
	tft.deinitialize();
	return 0;
}