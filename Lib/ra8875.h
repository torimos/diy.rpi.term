#pragma once
#include "SPI.h"

#define RA8875_480x272			0x01
#define RA8875_800x480			0x02

#define RGB(red, green, blue) (uint16_t) (((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3))

enum RA8875ModeEnum { GRAPHIC, TEXT };
enum RA8875MemoryEnum { Layer1, Layer2, CGRAM, Cursor, Pattern };
enum RA8875FontSourceEnum { INT_CGRAM, INT_CGROM };

enum RA8875LayerModeEnum
{
	OnlyLayer1,         ///< Only layer 1 is visible
	OnlyLayer2,         ///< Only layer 2 is visible
	LightenOverlay,     ///< Lighten-overlay mode
	TransparentMode,    ///< Transparent mode
	BooleanOR,          ///< Boolean OR mode
	BooleanAND,         ///< Boolean AND mode
	FloatingWindow      ///< Floating Window mode
};

class RA8875
{
public:
	RA8875(SPI* spi, uint32_t resetPin);
	~RA8875();

	void hardReset(void);
	void softReset(void);
	bool initialize(uint8_t mode);
	void deinitialize();
	void clearMemory(bool full);
	void setMode(RA8875ModeEnum mode);
	void selectMemory(RA8875MemoryEnum memory);
	void setLayerMode(RA8875LayerModeEnum mode);
	void setLayerTransparency(uint8_t layer1, uint8_t layer2);
	void setActiveWindow(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

	void setFontSource(RA8875FontSourceEnum source);
	void uploadUserChar(const uint8_t symbol[], uint8_t address);
	void textSetCursor(uint16_t x, uint16_t y);
	void textColor(uint16_t foreColor, uint16_t bgColor);
	void textTransparent(uint16_t foreColor);
	void textEnlarge(uint8_t scale);
	void textWrite(int x, int y, const char *str, ...);
	void showCursor(bool show, bool blink);
	void setCursorBlinkRate(uint8_t rate);

	bool waitPoll(uint8_t regname, uint8_t waitflag);
	void displayOn(bool on);
	void sleep(uint8_t sleep);
	void PWM1out(uint8_t p);
	void PWM2out(uint8_t p);
	void PWM1config(uint8_t on, uint8_t clock);
	void PWM2config(uint8_t on, uint8_t clock);

	void setXY(uint16_t x, uint16_t y);
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	void drawImage(uint16_t* addr, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
	void rectHelper(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled);
	void circleHelper(int16_t x0, int16_t y0, int16_t r, uint16_t color, bool filled);
	void ellipseHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color, bool filled);
	void triangleHelper(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, bool filled);
	void curveHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color, bool filled);
	void fillScreen(uint16_t color);


	void touchEnable(bool on);
	bool touched(bool clearIntFlag);
	bool touchRead(uint16_t *x, uint16_t *y);

	uint16_t get_width() { return _width; }
	uint16_t get_height() { return _height; }
private:
	uint32_t _resetPin;
	SPI* _spi;
	uint16_t _width;
	uint16_t _height;
	uint8_t _textScale;
	char _textBuffer[256];

	void writeData(uint8_t data);
	void writeCommand(uint8_t cmd);
	uint8_t readData(void);
	uint8_t readStatus(void);
	void writeReg(uint8_t reg, uint8_t val);
	void writeReg16(uint8_t reg, uint16_t val);
	uint8_t readReg(uint8_t reg);
	bool spiInit(uint32_t speed);
	bool PLLinit(void);
};
