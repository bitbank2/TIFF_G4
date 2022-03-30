#include <TIFF_G4.h>
#include <bb_spi_lcd.h>
#include "weather_icons.h"
#include "arduino_2clr.h"

static uint8_t ucTXBuf[4096];
static SPILCD lcd;
static TIFFG4 tiff;

// Set up for ILI9341 240x320 LCD on Teensy 4.0 + display board
#define DC_PIN 9
#define RESET_PIN -1
#define LED_PIN -1
#define CS_PIN 10
#define TFT_MOSI_PIN -1
#define TFT_MISO_PIN -1
#define TFT_CLK_PIN 13
#define SD_CS 4

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
//
// Callback function for TIFF_G4 library which is called for each scanline emitted
//
void TIFFDraw(TIFFDRAW *pDraw)
{  
//  Serial.printf("x,y=%d,%d, cx,cy=%d,%d\n", pDraw->iDestX, pDraw->y, pDraw->iScaledWidth, pDraw->iScaledHeight);
  if (pDraw->y == 0) {
    spilcdSetPosition(&lcd, pDraw->iDestX, pDraw->iDestY, pDraw->iScaledWidth, pDraw->iScaledHeight, DRAW_TO_LCD);
  }
  spilcdWriteDataBlock(&lcd, (uint8_t *)pDraw->pPixels, pDraw->iScaledWidth*2, DRAW_TO_LCD);
} /* TIFFDraw() */

void setup() {
  int i;
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // wait up to 3 seconds for Arduino Serial Monitor
  Serial.println("Starting...");
// A TX buffer needs to be defined to use DMA for writing
//  spilcdSetTXBuffer(ucTXBuf, sizeof(ucTXBuf));
  spilcdInit(&lcd, LCD_ILI9341, FLAGS_NONE, 60000000, CS_PIN, DC_PIN, RESET_PIN, LED_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CLK_PIN);
  spilcdSetOrientation(&lcd, LCD_ORIENTATION_270);
  spilcdFill(&lcd, 0, DRAW_TO_LCD);
// A few primary colors for the demo
const uint16_t usColors[] = {0xffff, 0x7e0, 0xffe0, 0xf800, 0x7ff, 0xf81f, 0x1f, 0x00};
// Draw 2 full sized (1.0f) weather icons
   for (i=0; i<2; i++) {
     if (tiff.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw))
     {
      tiff.drawIcon(1.0f, 46+(i*128), 50, 128, 118, i*128, 0, usColors[i]/*FG_COLOR*/, 0/*BG_COLOR*/);
      tiff.close();
     }
   }
// Draw 5 icons at 50% (0.5f) scale
   for (i=0; i<5; i++) {
     if (tiff.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw))
     {
      tiff.drawIcon(0.5f, 46+(i*128), 50, 128, 118, i*64, 120, usColors[i]/*FG_COLOR*/, 0/*BG_COLOR*/);
      tiff.close();
     }
   }
// Draw 6 icons at 30% (0.3f) scale
   for (int i=0; i<6; i++) {
     if (tiff.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw))
     {
      tiff.drawIcon(0.3f, 46+(i*128), 50+118, 128, 118, 32+i*46, 190, usColors[i]/*FG_COLOR*/, 0/*BG_COLOR*/);
      tiff.close();
     }
   }
   delay(5000);
   spilcdFill(&lcd, 0, DRAW_TO_LCD);
  // Draw 2 Arduino logos at 1/8th scale (0.125f)
   for (int i=0; i<2; i++) {
     if (tiff.openTIFF((uint8_t *)arduino_2clr, (int)sizeof(arduino_2clr), TIFFDraw))
     {
      tiff.drawIcon(0.125f, 0, 0, tiff.getWidth()-80, tiff.getHeight(), i*128, 0, 0, usColors[i+3]);
      tiff.close();
     }
   }
   // Draw 1 Arduino logo at 1/5th (0.2f) scale
   if (tiff.openTIFF((uint8_t *)arduino_2clr, (int)sizeof(arduino_2clr), TIFFDraw))
     {
      tiff.drawIcon(0.2f, 0, 0, tiff.getWidth()-80, tiff.getHeight(), 0, 100, 0xffff, 0x42a);
      tiff.close();
     }

  while (1) {}; // wait forever
} /* setup() */

void loop() {
  // nothing to see here
} /* loop() */
