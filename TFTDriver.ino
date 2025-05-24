//
//
//#include "WROVER_KIT_LCD.h"
#define LGFX_ESP_WROVER_KIT
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
//#include "WROVER_KIT_LCD.h"
#include <esp_task_wdt.h>
#include <byteswap.h>
#define LCD_MAX_HEIGHT   320L
#define LCD_MAX_WIDTH   240L

/* create a hardware timer */
hw_timer_t* timer = NULL;
uint16_t colr = 0, * bmp, decay[100], st = 63, sl = 31, vl, * px;
int ix, iy, tflag = 1;
uint8_t* bitmap, *bm;

void IRAM_ATTR TFTEvent() {
	tflag = 0;
}

void fade()
{
	int fflag = 0;

	for (iy = 0, px = bmp, bm=bitmap; iy < 320; iy++)
		for (ix = 0; ix < 240; ix++, px++, bm++)
			if (*bm) {
				vl = decay[*bm];
				//tft.drawPixel(40+iy, 240-ix, vl);
				*bm = vl ? *bm + 1 : 0;
				*px = vl;
				fflag++;
			}
	if (!fflag)						// Screen is blank
		return;
	tft.startWrite();
	tft.setAddrWindow(0, 0, LCD_MAX_HEIGHT, LCD_MAX_WIDTH);
	tft.writePixels(bmp, 320 * 240, false);
	//tft.pushColors(bmp, 240 * 320, false);
	tft.endWrite();
}

void TFTsetup(void* pvParameters) {
	uint16_t cmap;

	//esp_task_wdt_init(30, false);
	psramInit();
	bmp = (uint16_t*)ps_malloc(320 * 240 * 2);
	bitmap = (uint8_t*)ps_malloc(320 * 240);
	memset(bmp, 0, 320 * 240 * 2);
	memset(bitmap, 0, 320 * 240);
	//	for (i = 0; i < 300; i++)
//		bmp[i*240 + 120] = 1;
	for (i = 1; i < 100; i++) {
		cmap = (uint16_t)(st << 5);
		cmap |= (st / 2 << 11);
		cmap |= sl;
		decay[i] = __bswap_16(cmap);
		st *= 0.9;
		sl *= 0.6;
	}
	/* Use 1st timer of 4 */
	/* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &TFTEvent);
  timerAlarm(timer, 100000, true, 0);

	while (1) {
		while (tflag)
			yield();
		tflag = 1;
		fade();
		if (!tflag)
			Serial.printf(".");
	}
}


