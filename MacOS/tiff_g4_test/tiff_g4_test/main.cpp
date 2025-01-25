//
//  main.cpp
//  tiff_g4_test
//
//  Created by Laurence Bank on 1/20/25.
//

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../../../src/TIFF_G4.h"
#include "../../../src/TIFF_G4.cpp"
#include "../../../test_images/weather_icons.h"
#include "../../../test_images/bart_raw.h"
TIFFG4 g4;
int iLineCount, iOldY;
int iWidth, iHeight;
int iDrawWidth;

//
// Return the current time in milliseconds
//
int MilliTime(void)
{
int iTime;
struct timespec res;
                        
    clock_gettime(CLOCK_MONOTONIC, &res);
    iTime = (int)(1000*res.tv_sec + res.tv_nsec/1000000);
                
    return iTime;
} /* MilliTime() */

//
// Simple logging print
//
void TIFFLOG(int line, char *string, const char *result)
{
    printf("Line: %d: msg: %s%s\n", line, string, result);
} /* TIFFLOG() */

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
    int i, rc, iTime1, iTime2;
    uint8_t *pFuzzData;
    char *szTestName;
    const char *szStart = " - START";
    // Test that the weather_icons TIFF file decodes all lines correctly
    iOldY = -1;
    iLineCount = 0;
    szTestName = (char *)"TIFF full image decode";
    TIFFLOG(__LINE__, szTestName, szStart);
    if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
        rc = g4.decode();
        iWidth = g4.getWidth();
        iHeight = g4.getHeight();
        g4.close();
        if (rc == TIFF_SUCCESS && iHeight == iLineCount && iDrawWidth == iWidth) {
          TIFFLOG(__LINE__, szTestName, " - PASSED");
        } else {
          TIFFLOG(__LINE__, szTestName, " - FAILED");
          printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
        }
    } else { // open file failed
      TIFFLOG(__LINE__, szTestName, " - open failed");
    }
    // TEST 2
    // Test that a pure G4 image (no TIFF header) decodes correctly
    iOldY = -1;
    iLineCount = 0;
    szTestName = (char *)"RAW G4 full image decode";
    TIFFLOG(__LINE__, szTestName, szStart);
    if (g4.openRAW(250, 122, BITDIR_MSB_FIRST, (uint8_t *)bart_raw, sizeof(bart_raw), TIFFDraw)) {
        rc = g4.decode();
        g4.close();
        if (rc == TIFF_SUCCESS && iLineCount == 122 && iDrawWidth == 250) {
          TIFFLOG(__LINE__, szTestName, " - PASSED");
        } else {
          TIFFLOG(__LINE__, szTestName, " - FAILED");
          printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
        }
    } else { // open file failed
      TIFFLOG(__LINE__, szTestName, " - open failed\n");
    }
    // TEST 3
    // Test that a sub-image (icon) can be extracted from the weather_icons TIFF
    iOldY = -1;
    iLineCount = 0;
    szTestName = (char *)"TIFF file icon (sub-image) decode";
    TIFFLOG(__LINE__, szTestName, szStart);
    if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
      // choose sun icon (first row, 4th from the left, 128x118 pixels)
        rc = g4.drawIcon(1.0f, 46+(3*128), 50, 128, 118, 0, 0, 0xffff, 0);
        g4.close();
        if (rc == TIFF_SUCCESS && 118 == iLineCount && iDrawWidth == 128) {
          TIFFLOG(__LINE__, szTestName, " - PASSED\n");
        } else {
          TIFFLOG(__LINE__, szTestName, " - FAILED");
          printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
        }
    } else { // open file failed
      TIFFLOG(__LINE__, szTestName, " - open failed\n");
    }
    // TEST 4
    // Test that image cropping+scaling results in the correct output size
    iOldY = -1;
    iLineCount = 0;
    szTestName = (char *)"TIFF file image cropping+scaling";
    TIFFLOG(__LINE__, szTestName, szStart);
    if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
      // choose sun icon (first row, 4th from the left, 128x118 pixels)
        rc = g4.drawIcon(2.5f, 46+(3*128), 50, 128, 118, 0, 0, 0xffff, 0); // scale the image by 250%
        g4.close();
        if (rc == TIFF_SUCCESS && 295 == iLineCount && iDrawWidth == 320) {
          TIFFLOG(__LINE__, szTestName, " - PASSED\n");
        } else {
          TIFFLOG(__LINE__, szTestName, " - FAILED");
          printf("iHeight = %d, lines = %d, width = %d\n", iHeight, iLineCount, iDrawWidth);
        }
    } else { // open file failed
      TIFFLOG(__LINE__, szTestName, " - open failed");
    }
    // Test 5
    // Test that the performance of requesting a partial decode is not the same as a full decode
    // In other words, see if asking for 1/2 of the image to be decoded takes about 1/2 the time
    // of asking for the full image decode
    //
#define LOOP_COUNT 1000
    iOldY = -1;
    iLineCount = 0;
    szTestName = (char *)"TIFF crop window perf test";
    TIFFLOG(__LINE__, szTestName, szStart);
    iTime1 = MilliTime(); // start time
    for (int i=0; i<LOOP_COUNT; i++) { // needs to run many times on the Mac to measure a few milliseconds :)
        if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
            iHeight = g4.getHeight();
            rc = g4.decode(); // decode the whole image
            g4.close();
            if (!(rc == TIFF_SUCCESS && iHeight == iLineCount)) {
                TIFFLOG(__LINE__, szTestName, " - FAILED");
                printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
                i = LOOP_COUNT; // stop running
            }
        } else {
            TIFFLOG(__LINE__, szTestName, " - open failed");
            i = LOOP_COUNT; // stop running immediately
        }
    } // for i
    iTime1 = MilliTime() - iTime1; // get the total time in milliseconds
    // Now ask the library to decode only the top half of the image
    iTime2 = MilliTime(); // start time 2
    iOldY = -1;
    iLineCount = 0;
    for (i=0; i<LOOP_COUNT; i++) { // needs to run many times on the Mac to measure a few milliseconds :)
        if (g4.openTIFF((uint8_t *)weather_icons, (int)sizeof(weather_icons), TIFFDraw)) {
            iHeight = g4.getHeight();
            g4.setDrawParameters(1.0f, TIFF_PIXEL_1BPP, 0, 0, g4.getWidth(), iHeight/2, NULL);
            rc = g4.decode(); // decode the top half of the image
            g4.close();
            if (!(rc == TIFF_SUCCESS && iHeight/2 == iLineCount)) {
                TIFFLOG(__LINE__, szTestName, " - FAILED");
                printf("iHeight = %d, lines = %d\n", iHeight, iLineCount);
                i = LOOP_COUNT; // stop running
            }
        } else {
            TIFFLOG(__LINE__, szTestName, " - open failed");
            i = LOOP_COUNT; // stop running immediately
        }
    } // for i
    iTime2 = MilliTime() - iTime2; // get the total time in milliseconds for decoding 1/2 of the image
    // The time may not be exactly half because of the time to parse the file header, but it should certainly be at least 25% less
    i = (iTime1 * 3)/4;
    if (iTime2 < i) {
        TIFFLOG(__LINE__, szTestName, " - PASSED\n");
        printf("Full decode time (%d iterations) = %d ms\n", LOOP_COUNT, iTime1);
        printf("Top half decode time (%d iterations) = %d ms\n", LOOP_COUNT, iTime2);
    } else {
        TIFFLOG(__LINE__, szTestName, " - FAILED");
    }
    // FUZZ testing
    // Randomize the input data (file header and compressed data) and confirm that the library returns an error code
    // and doesn't have an invalid pointer exception
    printf("Begin fuzz testing...\n");
    szTestName = (char *)"Single Byte Sequential Corruption Test";
    pFuzzData = (uint8_t *)malloc(sizeof(weather_icons));
    TIFFLOG(__LINE__, szTestName, szStart);
    for (i=0; i<sizeof(weather_icons); i++) { // corrupt each byte one at a time by inverting it
        memcpy(pFuzzData, weather_icons, sizeof(weather_icons)); // start with the valid data
        pFuzzData[i] = ~pFuzzData[i]; // invert the bits of this byte
        if (g4.openTIFF((uint8_t *)pFuzzData, (int)sizeof(weather_icons), TIFFDraw)) { // the TIFF header may be rejected
            rc = g4.decode();
            g4.close();
        }
    } // for each test
    TIFFLOG(__LINE__, szTestName, " - PASSED");
    szTestName = (char *)"Multi-Byte Random Corruption Test";
    TIFFLOG(__LINE__, szTestName, szStart);
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
    TIFFLOG(__LINE__, szTestName, " - PASSED");

    free(pFuzzData);
    return 0;
}
