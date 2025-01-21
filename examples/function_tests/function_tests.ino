//
// Functional test for the TIFF_G4 library
// written by Larry Bank
//
// This sketch will exercise the TIFF_G4 API with known data and
// test for expected outputs. It should run on any MCU
//
#include <TIFF_G4.h>
#include "../test_images/weather_icons.h"
#include "../test_images/bart_raw.h"
TIFFG4 g4;
int iLineCount, iOldY;
int iDrawWidth;

// Draw callback
void TIFFDraw(TIFFDRAW *pDraw)
{
  iDrawWidth = pDraw->iScaledWidth;
  if (pDraw->y == iOldY+1) {
    iOldY++;
    iLineCount++; // check that line is incrementing correctly
  }
} /* TIFFDraw() */

void setup()
{
  Serial.begin(115200);
  delay(3000); // allow time for USB-CDC serial to start
  Serial.println("TIFF_G4 function tests");
} /* setup() */

void loop()
{
  int rc, iWidth, iHeight;
  // TEST 1
  // Test that the weather_icons TIFF file decodes all lines correctly
  iOldY = -1;
  iLineCount = 0;
  Serial.print("TIFF file full image decode:");
  if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
      rc = g4.decode();
      iWidth = g4.getWidth();
      iHeight = g4.getHeight();
//      tiff.drawIcon(1.0f, 46+(i*128), 50, 128, 118, i*128, 0, usColors[i]/*FG_COLOR*/, 0/*BG_COLOR*/);
      g4.close();
      if (rc == TIFF_SUCCESS && iHeight == iLineCount) {
        Serial.println(" PASS");
      } else {
        Serial.println(" FAIL");
        Serial.printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
      }
  } else { // open file failed
    Serial.println(" open failed");
  }
  // TEST 2
  // Test that a pure G4 image (no TIFF header) decodes correctly
  iOldY = -1;
  iLineCount = 0;
  Serial.print("RAW G4 full image decode:");
  if (g4.openRAW(250, 122, BITDIR_MSB_FIRST, (uint8_t *)bart_raw, sizeof(bart_raw), TIFFDraw)) {
      rc = g4.decode();
      g4.close();
      if (rc == TIFF_SUCCESS && iLineCount == 122 && iDrawWidth == 250) {
        Serial.println(" PASS");
      } else {
        Serial.println(" FAIL");
        Serial.printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
      }
  } else { // open file failed
    Serial.println(" open failed");
  }

    // TEST 3
    // Test that a sub-image (icon) can be extracted from the weather_icons TIFF
    iOldY = -1;
    iLineCount = 0;
    Serial.print("TIFF file icon (sub-image) decode:");
    if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
      // choose sun icon (first row, 4th from the left, 128x118 pixels)
        rc = g4.drawIcon(1.0f, 46+(3*128), 50, 128, 118, 0, 0, 0xffff, 0);
        g4.close();
        if (rc == TIFF_SUCCESS && 118 == iLineCount && iDrawWidth == 128) {
          Serial.println(" PASS");
        } else {
          Serial.println(" FAIL");
          Serial.printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
        }
    } else { // open file failed
      Serial.println(" open failed");
    }
  // TEST 4
  // Test that image cropping+scaling results in the correct output size
  iOldY = -1;
  iLineCount = 0;
  Serial.print("TIFF file image cropping+scaling:");
  if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
    // choose sun icon (first row, 4th from the left, 128x118 pixels)
      rc = g4.drawIcon(2.5f, 46+(3*128), 50, 128, 118, 0, 0, 0xffff, 0); // scale the image by 250%
      g4.close();
      if (rc == TIFF_SUCCESS && 295 == iLineCount && iDrawWidth == 320) {
        Serial.println(" PASS");
      } else {
        Serial.println(" FAIL");
        Serial.printf("iHeight = %d, lines = %d, width = %d\n", iHeight, iLineCount, iDrawWidth);
      }
  } else { // open file failed
    Serial.println(" open failed");
  }
  // END
  while (1) {
    delay(1000);
  } // stop here
} /* loop() */

