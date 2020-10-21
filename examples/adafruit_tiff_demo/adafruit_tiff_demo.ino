// TIFF_G4 example for Adafruit GFX displays
// Scales a TIFF image (smaller or large) to fill the display

#include <TIFF_G4.h>
// Sample image (truncated) containing a 320x240 Exif thumbnail
//#include "thumb_test.h"
#include "arduino_logo.h"

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

// PyPortal-specific pins
#define SD_CS         32 // SD card select
#define TFT_D0        34 // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR        26 // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC        10 // Data/command pin
#define TFT_CS        11 // Chip-select pin
#define TFT_RST       24 // Reset pin
#define TFT_RD         9 // Read-strobe pin
#define TFT_BACKLIGHT 25 // Backlight enable (active high)

Adafruit_ILI9341 tft(tft8bitbus, TFT_D0, TFT_WR, TFT_DC, TFT_CS, TFT_RST, TFT_RD);
TIFFG4 tiff;
const uint16_t us2BppToRGB565[4] = {0x0000, 0x528a, 0xa514, 0xffff};

void TIFFDraw(TIFFDRAW *pDraw)
{
uint8_t c, *src;
int i, j;
uint16_t usTemp[DISPLAY_WIDTH], *d;

  if (pDraw->y >= DISPLAY_HEIGHT)
     return; // beyond bottom of the display
     
  src = pDraw->pPixels;
  if (pDraw->ucPixelType == TIFF_PIXEL_1BPP)
  {
    memset(usTemp, 0, sizeof(usTemp)); // start with black
    for (i=0; i<(pDraw->iScaledWidth+7)/8; i++)
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
  else if (pDraw->ucPixelType == TIFF_PIXEL_2BPP) // 2-bit gray anti-aliased pixels
  {
    d = usTemp;
    for (i=0; i<(pDraw->iScaledWidth+3)/4; i++)
    {
      c = *src++;
      for (j=0; j<4; j++)
      {
        *d++ = us2BppToRGB565[c >> 6];
        c <<= 2;
      }
    }
  }
  tft.dmaWait(); // Wait for prior writePixels() to finish
  tft.setAddrWindow(0, pDraw->y, pDraw->iScaledWidth, 1);
  tft.writePixels(usTemp, pDraw->iScaledWidth, true, false); // Use DMA, big-endian
} /* TIFFDraw() */

void setup() {
  Serial.begin(115200);
//  while (!Serial);
  Serial.println("Starting...");
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH); // Backlight on

  // put your setup code here, to run once:
  tft.begin();
  tft.setRotation(3); // PyPortal native orientation
} /* setup() */

void loop() {
int x, cx;
float fScale;

    tft.fillScreen(ILI9341_WHITE);
    tft.startWrite(); // Not sharing TFT bus on PyPortal, just set CS once and leave it
    if (tiff.openTIFF((uint8_t *)arduino_logo, (int)sizeof(arduino_logo), TIFFDraw))
    {
      cx = tiff.getWidth(); // scale the image to fit the display      
      fScale = (float)DISPLAY_WIDTH / (float)cx;
      // Use TIFF_PIXEL_1BPP for bitonal only, 2BPP for anti-aliased grayscale
      tiff.setDrawParameters(fScale, TIFF_PIXEL_2BPP, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, NULL);
      tiff.decode();
      tiff.close();
    }
    while (1)
    {};
} /* loop() */
