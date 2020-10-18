//
// Demo to display a 1-bpp TIFF image
// on a 240x135 color LCD display
// using the 4-bpp anti-aliasing option
//
#include "test_images/notes.h"
#include <TIFF_G4.h>
#include <bb_spi_lcd.h>

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 135

#define TFT_CS 5
#define TFT_RST 18
#define TFT_DC 23
#define TFT_CLK 13
#define TFT_MOSI 15

#define MAX_WIDTH 256

static TIFFG4 tiff;
static SPILCD lcd;
static uint8_t ucTXBuf[2048];
static uint8_t ucTempBuf[MAX_WIDTH/2]; // used for 4bpp pixel output
const uint16_t usGrayToRGB565[16] = {0x0000, 0x1082, 0x2104, 0x3186, 0x4208, 0x528a, 0x630c, 0x738e,
                                     0x8410, 0x9492, 0xa514, 0xb596, 0xc618, 0xd69a, 0xe71c, 0xffff};
const uint16_t us2BppToRGB565[4] = {0x0000, 0x528a, 0xa514, 0xffff};

//
// This callback is given a single line of pixels
// that are either 1, 2 or 4-bits each
// They're packed into bytes such that the most significant bits
// represent the left-most pixel
//
void TIFFDraw(TIFFDRAW *pDraw)
{
uint8_t c, *src;
int i, j, iWidth;
uint16_t usTemp[DISPLAY_WIDTH], *d;

// Clip to display bounds
  if (pDraw->y >= DISPLAY_HEIGHT)
     return;
  iWidth = pDraw->iScaledWidth;
  if (iWidth > DISPLAY_WIDTH)
     iWidth = DISPLAY_WIDTH;

  src = pDraw->pPixels;
  if (pDraw->ucPixelType == TIFF_PIXEL_1BPP)
  {
    for (i=0; i<iWidth/8; i++)
    {
      d = &usTemp[i*8];
      c = *src++;
      while (c)
      {
        if (c & 0x80)
          *d = 0xffff;
        c <<= 1;
        d++;
      }
    }
  }
  else if (pDraw->ucPixelType == TIFF_PIXEL_2BPP)
  {
    d = usTemp;
    for (i=0; i<iWidth/4; i++)
    {
      c = *src++;
      for (j=0; j<4; j++)
      {
        *d++ = us2BppToRGB565[c >> 6];
        c <<= 2;
      }
    }
  }
  else if (pDraw->ucPixelType == TIFF_PIXEL_4BPP)
  {
    d = usTemp;
    for (i=0; i<iWidth/2; i++)
    {
      c = *src++;
      *d++ = usGrayToRGB565[c >> 4];
      *d++ = usGrayToRGB565[c & 0xf];
    }
  }
   spilcdSetPosition(&lcd, 0, pDraw->y, iWidth, 1, DRAW_TO_LCD);
   spilcdWriteDataBlock(&lcd, (uint8_t *)usTemp, iWidth*2, DRAW_TO_LCD | DRAW_WITH_DMA);
} /* TIFFDraw() */

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  spilcdSetTXBuffer(ucTXBuf, sizeof(ucTXBuf));
  spilcdInit(&lcd, LCD_ST7789_135, FLAGS_NONE, 32000000, 5, 16, -1, 4, -1, 19, 18); // TTGO T-Display pin numbering
  spilcdSetOrientation(&lcd, LCD_ORIENTATION_270);
  spilcdFill(&lcd, 0xffff,DRAW_TO_LCD); // default to white background
} /* setup() */

void loop() {
float f;

// loop through various scales from tiny to 2x
  for (f=0.05f; f <= 2.0f; f *= 1.05f)
  {
    if (tiff.openTIFF((uint8_t *)notes, (int)sizeof(notes), TIFFDraw))
    {
      Serial.println(obg.getWidth(), DEC);
      Serial.println(obg.getHeight(), DEC);
      tiff.setDrawParameters(f, TIFF_PIXEL_4BPP, 0, 0, 240, 135, ucTempBuf);
      tiff.decode();
      Serial.println("Finished!");
      tiff.close();
    }
  } // for f
} /* loop() */
