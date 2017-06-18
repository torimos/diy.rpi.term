#include "ra8875.h"
#include "ra8875_regs.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


// Command/Data pins for SPI
#define RA8875_DATAWRITE        0x00
#define RA8875_DATAREAD         0x40
#define RA8875_CMDWRITE         0x80
#define RA8875_CMDREAD          0xC0

#define DEFAULT_HIGH_SPI_CLOCK SPIClockDividerEnum::CLOCK_DIVIDER_16
#define DEFAULT_LOW_SPI_CLOCK SPIClockDividerEnum::CLOCK_DIVIDER_1024

RA8875::RA8875(uint8_t spiChannel, uint32_t resetPin)
{
	_resetPin = resetPin;
	_spi = new SPIdev(spiChannel);
	_textScale = 0;
}

RA8875::~RA8875()
{
	delete _spi;
}

void RA8875::writeData(uint8_t data)
{
	_spi->begin();
	_spi->write8(RA8875_DATAWRITE, true);
	_spi->write8(data, false);
	_spi->end();
}

void RA8875::writeData(uint8_t* data, uint32_t dataSize)
{
	_spi->begin();
	_spi->write8(RA8875_DATAWRITE, true);
	_spi->write(data, dataSize, false);
	_spi->end();
}

void RA8875::writeCommand(uint8_t cmd)
{
	_spi->begin();
	_spi->write8(RA8875_CMDWRITE, true);
	_spi->write8(cmd, false);
	_spi->end();
}

uint8_t RA8875::readData(void)
{
	_spi->begin();
	_spi->write8(RA8875_DATAREAD, true);
	uint8_t r =  _spi->write8(0, false);
	_spi->end();
	return r;
}

uint8_t RA8875::readStatus(void)
{
	_spi->begin();
	_spi->write8(RA8875_CMDREAD, true);
	uint8_t r = _spi->write8(0, false);
	_spi->end();
	return r;
}

void RA8875::writeReg(uint8_t reg, uint8_t val)
{
	writeCommand(reg);
	writeData(val);
}

void RA8875::writeReg16(uint8_t reg, uint16_t val)
{
	writeCommand(reg);
	writeData((uint16_t)(val)& 0xFF);
	writeCommand(reg+1);
	writeData((uint16_t)(val) >> 8);
}

uint8_t RA8875::readReg(uint8_t reg)
{
	writeCommand(reg);
	return readData();
}

bool RA8875::PLLinit(void)
{
	uint8_t r = readReg(0);
	if (r != 0x75)
		return false;

	//SYS_CLK = FIN * (PLLDIVN[4:0] + 1) / ((PLLDIVM + 1) * (2 ^ PLLDIVK[2:0]))
	writeReg(RA8875_PLLC1, RA8875_PLLC1_PLLDIV1 | 12);//PLLDIVM[7] | PLLDIVN[4:0]
	delay(1);
	writeReg(RA8875_PLLC2, RA8875_PLLC2_DIV2);//PLLDIVK[2:0]
	delay(1);
	return true;
}

void RA8875::hardReset(void)
{
	pinMode(_resetPin, OUTPUT);
	digitalWrite(_resetPin, LOW);
	delay(1);
	digitalWrite(_resetPin, HIGH);
	delay(10);
}

void RA8875::softReset(void)
{
	writeCommand(RA8875_PWRR);
	writeData(RA8875_PWRR_SOFTRESET);
	writeData(RA8875_PWRR_NORMAL);
	delay(1);
}

bool RA8875::waitPoll(uint8_t regname, uint8_t waitflag)
{
	/* Wait for the command to finish */
	while (1)
	{
		uint8_t temp = readReg(regname);
		if (!(temp & waitflag))
			return 1;
	}
	return 0; // MEMEFIX: yeah i know, unreached! - add timeout?
}

bool RA8875::initialize(uint8_t mode)
{
	uint8_t pixclk;
	uint8_t hsync_start;
	uint8_t hsync_pw;
	uint8_t hsync_finetune;
	uint8_t hsync_nondisp;
	uint8_t vsync_pw;
	uint16_t vsync_nondisp;
	uint16_t vsync_start;

	if (mode == RA8875_480x272)
	{
		_width = 480;
		_height = 272;
		pixclk = RA8875_PCSR_PDATL | RA8875_PCSR_4CLK;
		hsync_nondisp = 10;
		hsync_start = 8;
		hsync_pw = 48;
		hsync_finetune = 0;
		vsync_nondisp = 3;
		vsync_start = 8;
		vsync_pw = 10;
	}
	else if (mode == RA8875_800x480)
	{
		_width = 800;
		_height = 480;
		pixclk = RA8875_PCSR_PDATL | RA8875_PCSR_2CLK;
		hsync_nondisp = 26;
		hsync_start = 32;
		hsync_pw = 96;
		hsync_finetune = 0;
		vsync_nondisp = 32;
		vsync_start = 23;
		vsync_pw = 2;
	}

	hardReset();
	if (!_spi->initialize(SPIDataModeEnum::MODE_0, SPIBitSizeOrderEnum::SPI_8BIT_MSB)) return false;
	if (!PLLinit())
	{
		return false;
	}

	writeReg(RA8875_SYSR, RA8875_SYSR_16BPP | RA8875_SYSR_MCU8);
	writeReg(RA8875_PCSR, pixclk);
	delay(1);
	
	/* Horizontal settings registers */
	writeReg(RA8875_HDWR, (_width / 8) - 1);                          // H width: (HDWR + 1) * 8 = 480
	writeReg(RA8875_HNDFTR, RA8875_HNDFTR_DE_HIGH + hsync_finetune);
	writeReg(RA8875_HNDR, (hsync_nondisp - hsync_finetune - 2) / 8);    // H non-display: HNDR * 8 + HNDFTR + 2 = 10
	writeReg(RA8875_HSTR, hsync_start / 8 - 1);                         // Hsync start: (HSTR + 1)*8 
	writeReg(RA8875_HPWR, RA8875_HPWR_LOW + (hsync_pw / 8 - 1));        // HSync pulse width = (HPWR+1) * 8

	/* Vertical settings registers */
	writeReg16(RA8875_VDHR0, _height - 1);
	writeReg16(RA8875_VNDR0, vsync_nondisp - 1);                          // V non-display period = VNDR + 1
	writeReg16(RA8875_VSTR0, vsync_start - 1);                            // Vsync start position = VSTR + 1
	writeReg(RA8875_VPWR, RA8875_VPWR_LOW + vsync_pw - 1);            // Vsync pulse width = VPWR + 1

	writeReg(RA8875_DPCR, RA8875_DPCR_HDIR0 | RA8875_DPCR_VDIR0 | RA8875_DPCR_L1);
	
	setActiveWindow(0, 0, _width - 1, _height - 1);

	clearMemory(true);
	setCursorBlinkRate(255);
	showCursor(false, false);
	setFontSource(RA8875FontSourceEnum::INT_CGROM);
	selectMemory(RA8875MemoryEnum::Layer1);
	
	touchEnable(true);
	
	PWM1config(true, RA8875_PWM_CLK_DIV1024);
	PWM1out(255);
	displayOn(true);
	
	_spi->setClockDiv(DEFAULT_HIGH_SPI_CLOCK);
	return true;
}

void RA8875::deinitialize()
{
	_spi->deinitialize();
}

void RA8875::setActiveWindow(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	writeReg16(RA8875_HSAW0, left);
	writeReg16(RA8875_HEAW0, right);
	writeReg16(RA8875_VSAW0, top);
	writeReg16(RA8875_VEAW0, bottom);
	writeReg16(RA8875_CURH0, left);
	writeReg16(RA8875_CURV0, top);
}

void RA8875::clearMemory(bool full)
{
	uint8_t temp = RA8875_MCLR_START;
	if (!full) temp |= RA8875_MCLR_ACTIVE;
	writeReg(RA8875_MCLR, temp);
	waitPoll(RA8875_MCLR, RA8875_MCLR_READSTATUS);
}

void RA8875::setMode(RA8875ModeEnum mode)
{
	if (mode == _mode) return;
	_mode = mode;
	/* Set text mode */
	writeCommand(RA8875_MWCR0);
	uint8_t temp = readData();
	if (mode == RA8875ModeEnum::GRAPHIC)
		temp &= ~RA8875_MWCR0_TXTMODE; // bit #7
	else
		temp |= RA8875_MWCR0_TXTMODE; // Set bit 7
	writeData(temp);
}

void RA8875::selectMemory(RA8875MemoryEnum memory)
{
	uint8_t regFNCR0;
	uint8_t regMWCR1 = readReg(RA8875_MWCR1);
	switch (memory){
	case RA8875MemoryEnum::Layer1:
		regMWCR1 &= ~((1 << 3) | (1 << 2));// Clear bits 3 and 2
		regMWCR1 &= ~(1 << 0); //clear bit 0
		break;
	case RA8875MemoryEnum::Layer2:
		regMWCR1 &= ~((1 << 3) | (1 << 2));// Clear bits 3 and 2
		regMWCR1 |= (1 << 0); //set bit 0
		break;
	case RA8875MemoryEnum::CGRAM:
		regMWCR1 &= ~(1 << 3); //clear bit 3
		regMWCR1 |= (1 << 2); //set bit 2
		regFNCR0 = readReg(RA8875_FNCR0);
		if (regFNCR0 & (1 << 7))
		{
			regFNCR0 &= ~(1 << 7); //clear bit 7
			writeReg(RA8875_FNCR0, regFNCR0);
		}
		break;
	case RA8875MemoryEnum::Pattern:
		regMWCR1 |= (1 << 3); //set bit 3
		regMWCR1 |= (1 << 2); //set bit 2
		break;
	case RA8875MemoryEnum::Cursor:
		regMWCR1 |= (1 << 3); //set bit 3
		regMWCR1 &= ~(1 << 2); //clear bit 2
		break;
	}
	writeReg(RA8875_MWCR1, regMWCR1);
}

void RA8875::setLayerMode(RA8875LayerModeEnum mode)
{
	writeCommand(RA8875_LTPR0);
	uint8_t ltpr0 = readData() & ~0x7; // retain all but the display layer mode
	writeData(ltpr0 | (mode & 0x7));
}

void RA8875::setLayerTransparency(uint8_t layer1, uint8_t layer2)
{
	if (layer1 > 8) layer1 = 8;
	if (layer2 > 8) layer2 = 8;
	writeReg(RA8875_LTPR1, ((layer2 & 0xF) << 4) | (layer1 & 0xF));
}

void RA8875::setFontSource(RA8875FontSourceEnum source)
{
	/* Select the internal (ROM) font */
	writeCommand(RA8875_FNCR0);
	uint8_t temp = readData();
	if (source == RA8875FontSourceEnum::INT_CGROM)
		temp &= ~((1 << 7) | (1 << 5)); // Clear bits 7 and 5
	else if (source == RA8875FontSourceEnum::INT_CGRAM)
		temp |= (1 << 7);
	writeData(temp);
}

void RA8875::textSetCursor(uint16_t x, uint16_t y)
{
	writeReg16(RA8875_F_CURXL, x);
	writeReg16(RA8875_F_CURYL, y);
}

void RA8875::textColor(uint16_t foreColor, uint16_t bgColor)
{
	/* Set Fore Color */
	writeReg(RA8875_FGCR0, (foreColor & 0xf800) >> 11);
	writeReg(RA8875_FGCR1, (foreColor & 0x07e0) >> 5);
	writeReg(RA8875_FGCR2, (foreColor & 0x001f));

	/* Set Background Color */
	writeReg(RA8875_BGCR0, (bgColor & 0xf800) >> 11);
	writeReg(RA8875_BGCR1, (bgColor & 0x07e0) >> 5);
	writeReg(RA8875_BGCR2, (bgColor & 0x001f));

	/* Clear transparency flag */
	writeCommand(RA8875_FNCR1);
	uint8_t temp = readData();
	temp &= ~(1 << 6); // Clear bit 6
	writeData(temp);
}

void RA8875::textTransparent(uint16_t foreColor)
{
	/* Set Fore Color */
	writeReg(RA8875_FGCR0, (foreColor & 0xf800) >> 11);
	writeReg(RA8875_FGCR1, (foreColor & 0x07e0) >> 5);
	writeReg(RA8875_FGCR2, (foreColor & 0x001f));

	/* Set transparency flag */
	writeCommand(RA8875_FNCR1);
	uint8_t temp = readData();
	temp |= (1 << 6); // Set bit 6
	writeData(temp);
}

void RA8875::textEnlarge(uint8_t scale)
{
	if (scale > 3) scale = 3;

	/* Set font size flags */
	writeCommand(RA8875_FNCR1);
	uint8_t temp = readData();
	temp &= ~(0xF); // Clears bits 0..3
	temp |= scale << 2;
	temp |= scale;
	writeData(temp);

	_textScale = scale;
}

void RA8875::textWrite(int x, int y, const char *str, ...)
{
	va_list ap;
	va_start(ap, str);
	vsprintf(_textBuffer, str, ap);
	va_end(ap);
	textSetCursor(x, y);
	char *t = _textBuffer;
	writeCommand(RA8875_MRWC);
	while (*t != 0)
	{
		writeData((uint8_t)(*t));
		if (_textScale > 0) delay(1); // This delay is needed when CPU clock is high
		t++;
	}
}

void RA8875::uploadUserChar(const uint8_t symbol[], uint8_t address) {
	setMode(RA8875ModeEnum::GRAPHIC);
	writeReg(RA8875_CGSR, address); //CGRAM Space Select
	selectMemory(RA8875MemoryEnum::CGRAM);
	writeCommand(RA8875_MRWC);
	for (uint8_t i = 0; i<16; i++){
		writeData(symbol[i]);
	}
	setMode(RA8875ModeEnum::TEXT);
	selectMemory(RA8875MemoryEnum::Layer1);
}

void RA8875::showCursor(bool show, bool blink)
{
	uint8_t curh = 0, curv = 0;
	writeCommand(RA8875_MWCR0);
	uint8_t temp = readData();
	if (show) temp |= (1 << 6);
	else temp &= ~(1 << 6);
	if (blink) temp |= (1 << 5);
	else temp &= ~(1 << 5);
	writeReg(RA8875_MWCR0, temp);
	writeReg(RA8875_MWCR1, 0);
	if (show)
	{
		curh = 0x07;
		curv = 0x01;
	}
	writeReg(RA8875_CURHS, curh);
	writeReg(RA8875_CURVS, curv);
}

void RA8875::setCursorBlinkRate(uint8_t rate)
{
	writeReg(RA8875_BTCR, rate);
}

///////////////// GFX

void RA8875::setXY(uint16_t x, uint16_t y)
{
	writeReg16(RA8875_CURH0, x);
	writeReg16(RA8875_CURV0, y);
}

void RA8875::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	writeReg16(RA8875_CURH0, x);
	writeReg16(RA8875_CURV0, y);

	writeCommand(RA8875_MRWC);
	writeData(color);
}

void RA8875::drawImage(uint16_t *addr, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	setActiveWindow(x, y, x + w - 1, y + h-1);
	writeCommand(RA8875_MRWC);
	writeData((uint8_t*)addr, w*h << 1);
}

void RA8875::rectHelper(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled)
{
	/* Set X */
	writeReg16(RA8875_DLHSR0, x);
	/* Set Y */
	writeReg16(RA8875_DLVSR0, y);
	
	/* Set X1 */
	writeReg16(RA8875_DLHER0, w);
	/* Set Y1 */
	writeReg16(RA8875_DLVER0, h);
	
	/* Set Color */
	writeReg(RA8875_FGCR0, (color & 0xf800) >> 11);
	writeReg(RA8875_FGCR1, (color & 0x07e0) >> 5);
	writeReg(RA8875_FGCR2, (color & 0x001f));

	/* Draw! */
	writeCommand(RA8875_DCR);
	if (filled)
	{
		writeData(RA8875_DCR_LINESQUTRI_START | RA8875_DCR_DRAWSQUARE | RA8875_DCR_FILL);
	}
	else
	{
		writeData(RA8875_DCR_LINESQUTRI_START | RA8875_DCR_DRAWSQUARE);
	}

	/* Wait for the command to finish */
	waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

void RA8875::circleHelper(int16_t x0, int16_t y0, int16_t r, uint16_t color, bool filled)
{
	/* Set X */
	writeReg16(RA8875_DCHR0, x0);
	/* Set Y */
	writeReg16(RA8875_DCVR0, y0);

	/* Set Radius */
	writeReg(RA8875_DCRR, r);

	/* Set Color */
	writeReg(RA8875_FGCR0, (color & 0xf800) >> 11);
	writeReg(RA8875_FGCR1, (color & 0x07e0) >> 5);
	writeReg(RA8875_FGCR2, (color & 0x001f));

	/* Draw! */
	writeCommand(RA8875_DCR);
	if (filled)
	{
		writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_FILL);
	}
	else
	{
		writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_NOFILL);
	}

	/* Wait for the command to finish */
	waitPoll(RA8875_DCR, RA8875_DCR_CIRCLE_STATUS);
}

void RA8875::ellipseHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color, bool filled)
{
	/* Set Center Point */
	writeReg16(RA8875_DEHR0, xCenter);
	writeReg16(RA8875_DEVR0, yCenter);

	/* Set Long and Short Axis */
	writeReg16(RA8875_ELL_A0, longAxis);
	writeReg16(RA8875_ELL_B0, shortAxis);

	/* Set Color */
	writeReg(RA8875_FGCR0, (color & 0xf800) >> 11);
	writeReg(RA8875_FGCR1, (color & 0x07e0) >> 5);
	writeReg(RA8875_FGCR2, (color & 0x001f));

	/* Draw! */
	writeCommand(RA8875_ELLIPSE);
	if (filled)
	{
		writeData(RA8875_DCR_LINESQUTRI_START | 0x40);
	}
	else
	{
		writeData(RA8875_DCR_LINESQUTRI_START);
	}

	/* Wait for the command to finish */
	waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

void RA8875::triangleHelper(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, bool filled)
{
	/* Set Point 0 */
	writeReg16(RA8875_DLHSR0, x0);
	writeReg16(RA8875_DLVSR0, y0);

	/* Set Point 1 */
	writeReg16(RA8875_DLHER0, x1);
	writeReg16(RA8875_DLVER0, y1);

	/* Set Point 2 */
	writeReg16(RA8875_DTPH0, x2);
	writeReg16(RA8875_DTPV0, y2);

	/* Set Color */
	writeReg(RA8875_FGCR0, (color & 0xf800) >> 11);
	writeReg(RA8875_FGCR1, (color & 0x07e0) >> 5);
	writeReg(RA8875_FGCR2, (color & 0x001f));

	/* Draw! */
	writeCommand(RA8875_DCR);
	if (filled)
	{
		writeData(RA8875_DCR_LINESQUTRI_START | RA8875_DCR_DRAWTRIANGLE | RA8875_DCR_FILL);
	}
	else
	{
		writeData(RA8875_DCR_LINESQUTRI_START | RA8875_DCR_DRAWTRIANGLE | RA8875_DCR_NOFILL);
	}

	/* Wait for the command to finish */
	waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

void RA8875::curveHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color, bool filled)
{
	/* Set Center Point */
	writeReg16(RA8875_DEHR0, xCenter);
	writeReg16(RA8875_DEVR0, yCenter);

	/* Set Long and Short Axis */
	writeReg16(RA8875_ELL_A0, longAxis);
	writeReg16(RA8875_ELL_B0, shortAxis);

	/* Set Color */
	writeReg(RA8875_FGCR0, (color & 0xf800) >> 11);
	writeReg(RA8875_FGCR1, (color & 0x07e0) >> 5);
	writeReg(RA8875_FGCR2, (color & 0x001f));

	/* Draw! */
	writeCommand(RA8875_ELLIPSE);
	if (filled)
	{
		writeData(0xD0 | (curvePart & 0x03));
	}
	else
	{
		writeData(0x90 | (curvePart & 0x03));
	}

	/* Wait for the command to finish */
	waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

void RA8875::fillScreen(uint16_t color)
{
	rectHelper(0, 0, _width - 1, _height - 1, color, 1);
}

void RA8875::displayOn(bool on)
{
	writeReg(RA8875_PWRR, RA8875_PWRR_NORMAL | (on ? RA8875_PWRR_DISPON : RA8875_PWRR_DISPOFF));
	writeReg(RA8875_GPIOX, on ? true : false);
}

void RA8875::sleep(uint8_t sleep)
{
	writeReg(RA8875_PWRR, RA8875_PWRR_DISPOFF | (sleep ? RA8875_PWRR_SLEEP : 0));
}

void RA8875::PWM1out(uint8_t p)
{
	writeReg(RA8875_P1DCR, p);
}

void RA8875::PWM2out(uint8_t p)
{
	writeReg(RA8875_P2DCR, p);
}

void RA8875::PWM1config(uint8_t on, uint8_t clock)
{
	if (on) {
		writeReg(RA8875_P1CR, RA8875_P1CR_ENABLE | (clock & 0xF));
	}
	else {
		writeReg(RA8875_P1CR, RA8875_P1CR_DISABLE | (clock & 0xF));
	}
}

void RA8875::PWM2config(uint8_t on, uint8_t clock)
{
	if (on) {
		writeReg(RA8875_P2CR, RA8875_P2CR_ENABLE | (clock & 0xF));
	}
	else {
		writeReg(RA8875_P2CR, RA8875_P2CR_DISABLE | (clock & 0xF));
	}
}

void RA8875::touchEnable(bool on)
{
	if (on)
	{
		/* Enable Touch Panel (Reg 0x70) */
		writeReg(RA8875_TPCR0, RA8875_TPCR0_ENABLE |
			RA8875_TPCR0_WAIT_4096CLK |
			RA8875_TPCR0_WAKEDISABLE |
			RA8875_TPCR0_ADCCLK_DIV4); // 10mhz max!
		/* Set Auto Mode      (Reg 0x71) */
		writeReg(RA8875_TPCR1, RA8875_TPCR1_AUTO |
			// RA8875_TPCR1_VREFEXT | 
			RA8875_TPCR1_DEBOUNCE
			);
		/* Enable TP INT */
		writeReg(RA8875_INTC1, readReg(RA8875_INTC1) | RA8875_INTC1_TP);
	}
	else
	{
		/* Disable TP INT */
		writeReg(RA8875_INTC1, readReg(RA8875_INTC1) & ~RA8875_INTC1_TP);
		/* Disable Touch Panel (Reg 0x70) */
		writeReg(RA8875_TPCR0, RA8875_TPCR0_DISABLE);
	}
}

bool RA8875::touched(bool clearIntFlag)
{
	bool r = readReg(RA8875_INTC2) & RA8875_INTC2_TP;
	if (clearIntFlag)	{
		/* Clear TP INT Status */
		writeReg(RA8875_INTC2, RA8875_INTC2_TP);
	}
	return r;
}

#include <math.h>
bool RA8875::touchRead(uint16_t *x, uint16_t *y)
{
	uint16_t tx, ty;
	uint8_t temp;
	_spi->setClockDiv(DEFAULT_LOW_SPI_CLOCK);
	bool touched = readReg(RA8875_INTC2) & RA8875_INTC2_TP;
	if (touched)
	{
		tx = readReg(RA8875_TPXH);
		ty = readReg(RA8875_TPYH);
		temp = readReg(RA8875_TPXYL);
		tx <<= 2;
		ty <<= 2;
		tx |= temp & 0x03;        // get the bottom x bits
		ty |= (temp >> 2) & 0x03; // get the bottom y bits

		int cty = (ty-120) * 480 / 830.0;
		if (cty < 0) cty = 0;
		
		
		double xx = sin(cty / (480 / 180.0)*M_PI / 180.0) * 60.0;
		
		int ctx = (tx);// * 800 / 1400.0;
		if (ctx < 0) ctx = 0;
		
		*x = ctx;
		*y = cty;
	}
	writeReg(RA8875_INTC2, RA8875_INTC2_TP);
	_spi->setClockDiv(DEFAULT_HIGH_SPI_CLOCK);
	return touched;
}