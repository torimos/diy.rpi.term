#include "ra8875.h"
#include "ADS1x15.h"
#include "ADXL345.h"
#include "BMP085.h"
#include "ITG3200.h"
#include "HMC5883L.h"

int main(int argc, char *argv[])
{
	bcm2835_init();
	RA8875* tft = new RA8875();
	ADS1x15* ads = new ADS1x15(0x49);
	ADXL345* accel = new ADXL345(0x53);
	BMP085* bar = new BMP085(0x77);
	HMC5883L* mag = new HMC5883L(0x1E);
	ITG3200* gyro = new ITG3200(0x68);
	
	int fh = 16, j = 0, c = 0;
	int16_t x, y, z,mx,my,mz,gx,gy,gz;
	float t, p, a;
	accel->initialize();
	bar->initialize();
	mag->initialize();
	gyro->initialize();
	if (tft->initialize(RA8875_800x480))
	{
		//tft->touchEnable(true);
		tft->setMode(RA8875ModeEnum::TEXT);
		tft->textColor(RGB(0xFF, 0xFF, 0), 0);
	}
	tft->textWrite(0, fh * j++, "ACCEL ID=0x%02X", accel->getDeviceID());
	delay(1000);
	
	while (1)
	{
		tft->setMode(RA8875ModeEnum::TEXT);
		tft->textColor(RGB(0xFF, 0xFF, 0), 0);
		j = 0;
		tft->textWrite(0, fh * j++, "COUNTER %010d", c++);
		for (int ch = 0; ch < 4; ch++)
		{
			float val = ads->readADC_SingleEnded_V(ch);
			tft->textWrite(0, fh * j++, "ADC CH%d %f", ch, val);
		}
		accel->getAcceleration(&y, &x, &z);
		mag->getHeading(&mx, &my, &mz);
		gyro->getRotation(&gx, &gy, &gz);

		tft->textWrite(0, fh * j++, "ACCEL %05d:%05d:%05d", x, y, z);
		tft->textWrite(0, fh * j++, "ROT %05d:%05d:%05d", gx, gy, gz);
		float heading = atan2(my, mx);
		if (heading < 0)
			heading += 2 * M_PI;
		tft->textWrite(0, fh * j++, "MAG %03.3f  %05d:%05d:%05d", heading, mx, my, mz);
		bar->setControl(BMP085_MODE_TEMPERATURE);
		t = bar->getTemperatureC();
		bar->setControl(BMP085_MODE_PRESSURE_0);
		p = bar->getPressure();
		a = bar->getAltitude(p);
		tft->textWrite(0, fh * j++, "Temp=%03.3f Press=%03.3f Alt=%03.3f", t, p, a);
		tft->setMode(RA8875ModeEnum::GRAPHIC);
		tft->circleHelper(400 + x, 240 - y, 2, RGB(0, 0, 0xFF), true);
		//delay(125);
	}
	tft->deinitialize();
	return 0;
}