#include "ra8875.h"
#include "ADS1x15.h"
#include "BMP280.h"
#include <wiringPi.h>
#include <bcm2835.h>
#include <math.h>
int main(int argc, char *argv[])
{
	bcm2835_init();
	
	RA8875* tft = new RA8875();
	ADS1x15* ads = new ADS1x15(0x49);
	BMP280* bar = new BMP280();
	
	int fh = 16, j = 0, c = 0, tc = 0;
	uint16_t tx=0, ty=0;
	int x=0, y=0, z=0, ix = 0, iy = 0, iz = 0;
	float t, p, a, a0, a1, a2, a3;
	bool init = bar->initialize();
	if (tft->initialize(RA8875_800x480))
	{
		tft->setMode(RA8875ModeEnum::TEXT);
		tft->textColor(RGB(0xFF, 0xFF, 0), 0);
	}
	uint32_t last_time, time;
	last_time = time = millis();
	while (1)
	{
		if ((time - last_time) > 1000)
		{
			last_time = time;
			a0 = ads->readADC_SingleEnded_V(0);
			a1 = ads->readADC_SingleEnded_V(1);
			a2 = ads->readADC_SingleEnded_V(3);
		
			t = bar->readTemperature();
			p = bar->readPressure();
			a = bar->readAltitude();
	//		tc = tft->touchRead(&tx, &ty);
	//		if (!tc)
	//		{
	//			tx = ty = 0;
	//		}

			j = 0;
			tft->setMode(RA8875ModeEnum::TEXT);
			tft->textColor(RGB(0xFF, 0xFF, 0), 0);
			tft->textWrite(0, fh * j++, "TIME %010d", time);
			tft->textWrite(0, fh * j++, "VDD=%04.2f VBAT=%04.2f VCHRG=%04.2f BAT=%02.0f\%", a0, a1, a2, (a1 - 2.95) / ((3.99 - 2.95)/100.0));
			tft->textWrite(0, fh * j++, "Temp=%04.2f Alt=%04.2f Press=%04.2f", t, a, p);
			//tft->textWrite(0, fh * j++, "TS %05d %05d:%05d", tc, tx, ty);
			//tft->setMode(RA8875ModeEnum::GRAPHIC);
			//tft->circleHelper(400 + x, 240 - y, 2, RGB(0, 0, 0xFF), true);
			//tft->circleHelper(tx, ty, 2, RGB(0,0,0xFF), true);
		}
		time = millis();
	}
	tft->deinitialize();
	return 0;
}