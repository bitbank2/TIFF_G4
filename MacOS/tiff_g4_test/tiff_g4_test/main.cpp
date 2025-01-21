//
//  main.cpp
//  tiff_g4_test
//
//  Created by Laurence Bank on 1/20/25.
//

#include <stdio.h>
#include <string.h>
#include "../../../src/TIFF_G4.h"
#include "../../../src/TIFF_G4.cpp"
#include "../../../test_images/weather_icons.h"
#include "../../../test_images/bart_raw.h"
TIFFG4 g4;
int iLineCount, iOldY;
int iWidth, iHeight;
int iDrawWidth;

// Draw callback
void TIFFDraw(TIFFDRAW *pDraw)
{
    iDrawWidth = pDraw->iScaledWidth;
  if (pDraw->y == iOldY+1) {
    iOldY++;
    iLineCount++; // check that line is incrementing correctly
      if (pDraw->y == 1024) {
          iOldY |= 0;
      }
  }
} /* TIFFDraw() */

int main(int argc, const char * argv[]) {
    int i, rc;
    uint8_t *pFuzzData;
    // Test that the weather_icons TIFF file decodes all lines correctly
    iOldY = -1;
    iLineCount = 0;
    printf("TIFF full image decode:");
    if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
        rc = g4.decode();
        iWidth = g4.getWidth();
        iHeight = g4.getHeight();
        g4.close();
        if (rc == TIFF_SUCCESS && iHeight == iLineCount && iDrawWidth == iWidth) {
          printf(" PASS\n");
        } else {
          printf(" FAIL\n");
          printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
        }
    } else { // open file failed
      printf(" open failed\n");
    }
    // TEST 2
    // Test that a pure G4 image (no TIFF header) decodes correctly
    iOldY = -1;
    iLineCount = 0;
    printf("RAW G4 full image decode:");
    if (g4.openRAW(250, 122, BITDIR_MSB_FIRST, (uint8_t *)bart_raw, sizeof(bart_raw), TIFFDraw)) {
        rc = g4.decode();
        g4.close();
        if (rc == TIFF_SUCCESS && iLineCount == 122 && iDrawWidth == 250) {
          printf(" PASS\n");
        } else {
          printf(" FAIL\n");
          printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
        }
    } else { // open file failed
      printf(" open failed\n");
    }
    // TEST 3
    // Test that a sub-image (icon) can be extracted from the weather_icons TIFF
    iOldY = -1;
    iLineCount = 0;
    printf("TIFF file icon (sub-image) decode:");
    if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
      // choose sun icon (first row, 4th from the left, 128x118 pixels)
        rc = g4.drawIcon(1.0f, 46+(3*128), 50, 128, 118, 0, 0, 0xffff, 0);
        g4.close();
        if (rc == TIFF_SUCCESS && 118 == iLineCount && iDrawWidth == 128) {
          printf(" PASS\n");
        } else {
          printf(" FAIL\n");
          printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
        }
    } else { // open file failed
      printf(" open failed\n");
    }
    // TEST 4
    // Test that image cropping+scaling results in the correct output size
    iOldY = -1;
    iLineCount = 0;
    printf("TIFF file image cropping+scaling:");
    if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
      // choose sun icon (first row, 4th from the left, 128x118 pixels)
        rc = g4.drawIcon(2.5f, 46+(3*128), 50, 128, 118, 0, 0, 0xffff, 0); // scale the image by 250%
        g4.close();
        if (rc == TIFF_SUCCESS && 295 == iLineCount && iDrawWidth == 320) {
          printf(" PASS\n");
        } else {
          printf(" FAIL\n");
          printf("iHeight = %d, lines = %d, width = %d\n", iHeight, iLineCount, iDrawWidth);
        }
    } else { // open file failed
      printf(" open failed\n");
    }
    // FUZZ testing
    // Randomize the input data (file header and compressed data) and confirm that the library returns an error code
    // and doesn't have an invalid pointer exception
    printf("Begin fuzz testing...\n");
    pFuzzData = (uint8_t *)malloc(sizeof(weather_icons));
    printf("Single Byte Sequential Corruption Test\n");
    for (i=0; i<sizeof(weather_icons); i++) { // corrupt each byte one at a time by inverting it
        memcpy(pFuzzData, weather_icons, sizeof(weather_icons)); // start with the valid data
        pFuzzData[i] = ~pFuzzData[i]; // invert the bits of this byte
        if (g4.openTIFF((uint8_t *)pFuzzData, (int)sizeof(weather_icons), TIFFDraw)) { // the TIFF header may be rejected
            rc = g4.decode();
            g4.close();
        }
    } // for each test
    printf("PASS (it didn't crash :D )\n");
    printf("Multi-Byte Random Corruption Test\n");
    for (i=0; i<10000; i++) { // 10000 iterations of random spots in the file to corrupt with random values
        int iOffset;
        memcpy(pFuzzData, weather_icons, sizeof(weather_icons)); // start with the valid data
        iOffset = rand() % sizeof(weather_icons);
        pFuzzData[iOffset] = (uint8_t)rand();
        iOffset = rand() % sizeof(weather_icons); // corrupt 2 spots just for good measure
        pFuzzData[iOffset] = (uint8_t)rand();
        if (g4.openTIFF((uint8_t *)pFuzzData, (int)sizeof(weather_icons), TIFFDraw)) { // the TIFF header may be rejected
            rc = g4.decode();
            g4.close();
        }
    } // for each test
    printf("PASS (it didn't crash :D )\n");

    free(pFuzzData);
    return 0;
}
