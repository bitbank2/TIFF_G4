/**
 *  @filename   :   epd4in2-demo.ino
 *  @brief      :   4.2inch e-paper display demo
 *  @author     :   Yehui from Waveshare
 *
 *  Copyright (C) Waveshare     August 4 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <SPI.h>
#include "epd4in2.h"
#include "imagedata.h"
#include "epdpaint.h"
#include <OneBitGFX.h>
#include "notes.h" // TIFF file

#define COLORED     0
#define UNCOLORED   1
ONEBITGFX obg;
static uint8_t image[1500];
Epd epd;

void TIFFDraw(OBGFXDRAW *pDraw)
{
uint8_t *d;
   d = &image[(pDraw->y & 7) * (400/8)]; // current line offset
   memcpy(d, pDraw->pPixels, 400/8); // get current line
   if ((pDraw->y & 7) == 7 || pDraw->y == pDraw->iHeight-1) // send to display
   {
      epd.SetPartialWindow(image, 0, pDraw->y, 400, 8);
      if (pDraw->y == pDraw->iHeight-1) // display the whole thing
         epd.DisplayFrame();
   }
} /* TIFFDraw() */

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
	//Serial.print(UNCOLORED);
  /* This clears the SRAM of the e-paper display */
  epd.ClearFrame();

  /**
    * Due to RAM not enough in Arduino UNO, a frame buffer is not allowed.
    * In this case, a smaller image buffer is allocated and you have to 
    * update a partial display several times.
    * 1 byte = 8 pixels, therefore you have to set 8*N pixels at a time.
    */
  Paint paint(image, 400, 8);    //width should be the multiple of 8 
  if (obg.openTIFF((uint8_t *)notes, sizeof(notes), TIFFDraw))
  {
    obg.decode(0,0,0);
    obg.close();
  }
if (0)
{
  paint.Clear(UNCOLORED);
  paint.DrawStringAt(0, 0, "e-Paper Demo", &Font24, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 100, 40, paint.GetWidth(), paint.GetHeight());

  paint.Clear(COLORED);
  paint.DrawStringAt(100, 2, "Hello world", &Font24, UNCOLORED);
  epd.SetPartialWindow(paint.GetImage(), 0, 64, paint.GetWidth(), paint.GetHeight());
  
  paint.SetWidth(64);
  paint.SetHeight(64);

  paint.Clear(UNCOLORED);
  paint.DrawRectangle(0, 0, 40, 50, COLORED);
  paint.DrawLine(0, 0, 40, 50, COLORED);
  paint.DrawLine(40, 0, 0, 50, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 72, 120, paint.GetWidth(), paint.GetHeight());
  
  paint.Clear(UNCOLORED);
  paint.DrawCircle(32, 32, 30, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 200, 120, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawFilledRectangle(0, 0, 40, 50, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 72, 200, paint.GetWidth(), paint.GetHeight());

  paint.Clear(UNCOLORED);
  paint.DrawFilledCircle(32, 32, 30, COLORED);
  epd.SetPartialWindow(paint.GetImage(), 200, 200, paint.GetWidth(), paint.GetHeight());

  /* This displays the data from the SRAM in e-Paper module */
  epd.DisplayFrame();

  /* This displays an image */
  //epd.DisplayFrame(IMAGE_BUTTERFLY);

  
  
  epd.Init_4Gray();
  epd.ClearFrame();
//  epd.Set_4GrayDisplay((const char *)gImage_4in2_4Gray1, 100, 100,  200,150);
  epd.Set_4GrayDisplay((const char *)IMAGE_BUTTERFLY, 100, 100,  200,150);
}
  /* Deep sleep */
  epd.Sleep();
}

void loop() {
  // put your main code here, to run repeatedly:

}
