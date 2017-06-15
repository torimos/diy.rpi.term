/*#include <stdio.h>
#include "SDL/SDL.h"
#include <SDL/SDL_ttf.h>


TTF_Font *font;
SDL_Surface* scrMain = NULL;
char _textBuffer[1024];

void printf2d(SDL_Color color, int x, int y, const char *str)
{
	SDL_Surface *text = TTF_RenderText_Solid(font, str, color);
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	dst.w = text->w;
	dst.h = text->h;
	SDL_BlitSurface(text, NULL, scrMain, &dst);
	SDL_FreeSurface(text);
}

void printf2d(int x, int y, const char *str, ...)
{
	va_list ap;
	va_start(ap, str);
	vsprintf(_textBuffer, str, ap);
	va_end(ap);
	char *t = _textBuffer;
	SDL_Color white = { 255, 255, 255 };
	printf2d(white, x, y, _textBuffer);
}

int Init()
{
	putenv((char*)"SDL_FBDEV=/dev/fb1");
	putenv((char*)"SDL_VIDEODRIVER=fbcon");
    //Start SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) { SDL_Quit(); return -1; }
	if (TTF_Init() < 0) { SDL_Quit(); return -1; }
	SDL_ShowCursor(SDL_DISABLE);
	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();
	int nResX = videoInfo->current_w;
	int nResY = videoInfo->current_h;
	Uint8 bpp = videoInfo->vfmt->BitsPerPixel;
   
	scrMain = SDL_SetVideoMode(nResX, nResY, bpp, SDL_SWSURFACE | SDL_FULLSCREEN);
	if (!scrMain)
	{
		SDL_Quit();
		return -1;
	}
	font = TTF_OpenFont("/usr/share/fonts/truetype/droid/DroidSans.ttf", 12);
	if (font == NULL)
	{
		char *err = TTF_GetError();
		SDL_Quit();
		return -1;
	}
	
	return 0;
}

void Deinit()
{
	TTF_CloseFont(font);
	SDL_FreeSurface(scrMain);
	SDL_Quit();
}

int main_sdl(int argc, char *argv[])
{
	if (Init() < 0) return -1;
	SDL_Rect rect = { 0, 0, 800, 480 };
	int i = 0;
	while (i<10000)
	{
		SDL_FillRect(scrMain, &rect, 0);
		
		for (int y = 0; y < 300; y++)
			printf2d((y / 40) * 80, (y % 40) * 12, "Row #%d", i++);
	
		//Update Screen
		SDL_Flip(scrMain);
		//Pause
	}
	
	Deinit();
	return 0;
}	*/