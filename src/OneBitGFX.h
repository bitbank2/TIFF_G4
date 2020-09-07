#ifndef __ONEBITGFX__
#define __ONEBITGFX__
#if defined( __MACH__ ) || defined( __LINUX__ )
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#define memcpy_P memcpy
#define PROGMEM
#define pgm_read_byte(s) *s
#define pgm_read_word(s) *(int16_t *)s
#else
#include <Arduino.h>
#endif
//
// One Bit Graphics library
// Written by Larry Bank
// Copyright (c) 2020 BitBank Software, Inc.
// 
// Designed to decode and display 1-bpp CCITT G4
// encoded images at any scale
// with optional "scale-to-gray" enhancement
//

/* Defines and variables */
#define MAX_BUFFERED_PIXELS 1024
#define FILE_BUF_SIZE 2048
#define MAX_IMAGE_WIDTH 2600
#define FILE_HIGHWATER ((FILE_BUF_SIZE * 3) >> 2)
#define TIFF_TAG_SIZE 12
#define MAX_TIFF_TAGS 128
#define BITDIR_MSB_FIRST     1
#define BITDIR_LSB_FIRST     2

// Error codes returned by getLastError()
enum {
    OBGFX_SUCCESS = 0,
    OBGFX_INVALID_PARAMETER,
    OBGFX_DECODE_ERROR,
    OBGFX_UNSUPPORTED_FEATURE,
    OBGFX_INVALID_FILE
};
//
// Output pixel types
// selecting 2 or 8-bpp output automatically selects
// scale-to-gray anti-aliasing
//
enum {
    OBGFX_PIXEL_1BPP = 0,
    OBGFX_PIXEL_2BPP,
    OBGFX_PIXEL_4BPP
};

typedef struct obgfx_file_tag
{
  int32_t iPos; // current file position
  int32_t iSize; // file size
  uint8_t *pData; // memory file pointer
  void * fHandle; // class pointer to File/SdFat or whatever you want
} OBGFXFILE;

//
// Defines the output drawing window
//
typedef struct obgfx_window_tag
{
    int x, y; // upper left corner of interest (source pixels)
    float fScale;
    uint32_t iScale; // 16:16 fixed scale factor (e.g. 0.5 = 0x8000)
    int iWidth, iHeight; // destination window size (for clipping purposes)
    uint8_t *p4BPP; // user-supplied buffer for 4-bpp grayscale output
    uint8_t ucPixelType;
} OBGFXWINDOW;

typedef struct obgfx_draw_tag
{
    int y; // current ;ome
    int iWidth, iHeight; // size of entire image in pixels
    uint8_t *pPixels; // 1 or 2-bit pixels
    uint8_t ucPixelType, ucLast;
} OBGFXDRAW;

// Callback function prototypes
typedef int32_t (OBGFX_READ_CALLBACK)(OBGFXFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (OBGFX_SEEK_CALLBACK)(OBGFXFILE *pFile, int32_t iPosition);
typedef void (OBGFX_DRAW_CALLBACK)(OBGFXDRAW *pDraw);
typedef void * (OBGFX_OPEN_CALLBACK)(const char *szFilename, int32_t *pFileSize);
typedef void (OBGFX_CLOSE_CALLBACK)(void *pHandle);

//
// our private structure to hold a TIFF image decode state
//
typedef struct obgfx_image_tag
{
    int iWidth, iHeight; // image size
    int iError;
    int y; // last y value drawn
    int iVLCOff, iVLCSize;
    int iStripSize, iStripOffset;
    int iPitch; // width in bytes of output buffer
    uint32_t u32Accum; // fractional scaling accumulator
    uint8_t ucCompression, ucPhotometric, ucFillOrder;
    OBGFX_READ_CALLBACK *pfnRead;
    OBGFX_SEEK_CALLBACK *pfnSeek;
    OBGFX_DRAW_CALLBACK *pfnDraw;
    OBGFX_OPEN_CALLBACK *pfnOpen;
    OBGFX_CLOSE_CALLBACK *pfnClose;
    OBGFXFILE OBGFXFile;
    OBGFXWINDOW window;
    int16_t CurFlips[MAX_IMAGE_WIDTH];
    int16_t RefFlips[MAX_IMAGE_WIDTH];
    uint8_t ucPixels[MAX_BUFFERED_PIXELS];
    uint8_t ucFileBuf[FILE_BUF_SIZE]; // holds temp data and pixel stack
} OBGFXIMAGE;

#ifdef __cplusplus
//
// The ONEBITGFX class wraps portable C code which does the actual work
//
class ONEBITGFX
{
  public:
    int openTIFF(uint8_t *pData, int iDataSize, OBGFX_DRAW_CALLBACK *pfnDraw);
    int openTIFF(const char *szFilename, OBGFX_OPEN_CALLBACK *pfnOpen, OBGFX_CLOSE_CALLBACK *pfnClose, OBGFX_READ_CALLBACK *pfnRead, OBGFX_SEEK_CALLBACK *pfnSeek, OBGFX_DRAW_CALLBACK *pfnDraw);
    int openRAW(int iWidth, int iHeight, int iFillOrder, uint8_t *pData, int iDataSize, OBGFX_DRAW_CALLBACK *pfnDraw);
    void close();
    void setDrawParameters(float scale, int iPixelType, int iStartX, int iStartY, int iWidth, int iHeight);
    int decode();
    int getWidth();
    int getHeight();
    int getLastError();

  private:
    OBGFXIMAGE _obgfx;
};
#else
    int OBGFX_openTIFFRAM(OBGFXIMAGE *pImage, uint8_t *pData, int iDatasize, OBGFX_DRAW_CALLBACK *pfnDraw);
    int OBGFX_openTIFFFile(OBGFXIMAGE *pImage, const char *szFilename, OBGFX_OPEN_CALLBACK *pfnOpen, OBGFX_CLOSE_CALLBACK *pfnClose, OBGFX_READ_CALLBACK *pfnRead, OBGFX_SEEK_CALLBACK *pfnSeek, OBGFX_DRAW_CALLBACK *pfnDraw);
    int OBGFX_openRAW(OBGFXIMAGE *pImage, int iWidth, int iHeight, int iFillOrder, uint8_t *pData, int iDataSize, OBGFX_DRAW_CALLBACK *pfnDraw);
    void OBGFX_close(OBGFXIMAGE *pImage);
    void OBGFX_setDrawParameters(OBGFXIMAGE *pImage, float scale, int iPixelType, int iStartX, int iStartY, int iWidth, int iHeight);
    int OBGFX_decode(OBGFXIMAGE *pImage);
    int OBGFX_getWidth(OBGFXIMAGE *pImage);
    int OBGFX_getHeight(OBGFXIMAGE *pImage);
    int OBGFX_getLastError(OBGFXIMAGE *pImage);
#endif

// Due to unaligned memory causing an exception, we have to do these macros the slow way
#define MOTOLONG(p) (((*p)<<24UL) + ((*(p+1))<<16UL) + ((*(p+2))<<8UL) + (*(p+3)))
#define TOP_BIT 0x80000000
#define MAX_VALUE 0xffffffff
#define LONGWHITECODEMASK 0x2000000
#define LONGBLACKCODEMASK 0x10000000
// Must be a 32-bit target processor
#define REGISTER_WIDTH 32

#define CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode) \
    { uint32_t ul; int iLen = 64; sCode = 0; while (iLen > 63) \
    { if (ulBitOff > (REGISTER_WIDTH-17)) \
      { pBuf += (ulBitOff>>3); ulBitOff &= 7; ulBits = MOTOLONG(pBuf); } \
      if ((ulBits << ulBitOff) < LONGWHITECODEMASK) \
                  { ul = (ulBits >> ((REGISTER_WIDTH-14) - ulBitOff)) & 0x3fe; ulBitOff += pgm_read_word(&black_l[ul]); iLen = pgm_read_word(&black_l[ul+1]);} \
                else {ul = (ulBits >> ((REGISTER_WIDTH - 10) - ulBitOff)) & 0x3fe; ulBitOff += pgm_read_word(&white_s[ul]); iLen = pgm_read_word(&white_s[ul+1]);} \
     sCode += iLen; }}

#define CLIMBBLACK_NEW(pBuf, ulBitOff, ulBits, sCode) \
    { uint32_t ul; int iLen = 64; sCode = 0; while (iLen > 63) \
    { if (ulBitOff > (REGISTER_WIDTH-15)) \
      { pBuf += (ulBitOff>>3); ulBitOff &= 7; ulBits = MOTOLONG(pBuf); } \
      if ((ulBits << ulBitOff) < LONGBLACKCODEMASK) \
         { ul = (ulBits >> ((REGISTER_WIDTH-14) - ulBitOff)) & 0x3fe; ulBitOff += pgm_read_word(&black_l[ul]); iLen = pgm_read_word(&black_l[ul+1]);} \
        else {ul = (ulBits >> ((REGISTER_WIDTH - 7) - ulBitOff)) & 0x7e; ulBitOff += pgm_read_word(&black_s[ul]); iLen = pgm_read_word(&black_s[ul+1]);} \
     sCode += iLen; }}

#endif // __ONEBITGFX__
