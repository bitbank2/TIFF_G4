#ifndef __ONEBITGFX__
#define __ONEBITGFX__
#if defined( __MACH__ ) || defined( __LINUX__ )
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#define memcpy_P memcpy
#define PROGMEM
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
#define FILE_HIGHWATER 1536
#define FILE_BUF_SIZE 2048
#define MAX_BUFFERED_PIXELS 2048
#define MAX_IMAGE_WIDTH 2550
#define TIFF_TAG_SIZE 12
#define MAX_TIFF_TAGS 128

// Error codes returned by getLastError()
enum {
    OBGFX_SUCCESS = 0,
    OBGFX_INVALID_PARAMETER,
    OBGFX_DECODE_ERROR,
    OBGFX_UNSUPPORTED_FEATURE,
    OBGFX_INVALID_FILE
};

typedef struct buffered_bits
{
unsigned char *pBuf; // buffer pointer
uint32_t ulBits; // buffered bits
uint32_t ulBitOff; // current bit offset
} BUFFERED_BITS;

typedef struct tiff_file_tag
{
  int32_t iPos; // current file position
  int32_t iSize; // file size
  uint8_t *pData; // memory file pointer
  void * fHandle; // class pointer to File/SdFat or whatever you want
} TIFFFILE;

typedef struct tiff_draw_tag
{
    int x, y; // upper left corner of current MCU
    int iWidth, iHeight; // size of this MCU
    int iBpp; // bit depth of the pixels (8 or 16)
    uint16_t *pPixels; // 16-bit pixels
} TIFFDRAW;

// Callback function prototypes
typedef int32_t (TIFF_READ_CALLBACK)(TIFFFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (TIFF_SEEK_CALLBACK)(TIFFFILE *pFile, int32_t iPosition);
typedef void (TIFF_DRAW_CALLBACK)(TIFFDRAW *pDraw);
typedef void * (TIFF_OPEN_CALLBACK)(char *szFilename, int32_t *pFileSize);
typedef void (TIFF_CLOSE_CALLBACK)(void *pHandle);

//
// our private structure to hold a TIFF image decode state
//
typedef struct tiff_image_tag
{
    int iWidth, iHeight; // image size
    int iXOffset, iYOffset; // placement on the display
    int iError;
    int iOptions;
    TIFF_READ_CALLBACK *pfnRead;
    TIFF_SEEK_CALLBACK *pfnSeek;
    TIFF_DRAW_CALLBACK *pfnDraw;
    TIFF_OPEN_CALLBACK *pfnOpen;
    TIFF_CLOSE_CALLBACK *pfnClose;
    TIFFFILE TIFFFile;
    BUFFERED_BITS bb;
    int16_t CurFlips[MAX_IMAGE_WIDTH];
    int16_t RefFlips[MAX_IMAGE_WIDTH];
    uint8_t ucFileBuf[FILE_BUF_SIZE]; // holds temp data and pixel stack
} TIFFIMAGE;

//
// The ONEBITGFX class wraps portable C code which does the actual work
//
class ONEBITGFX
{
  public:
    int open(uint8_t *pData, int iDataSize, TIFF_DRAW_CALLBACK *pfnDraw);
    int open(char *szFilename, TIFF_OPEN_CALLBACK *pfnOpen, TIFF_CLOSE_CALLBACK *pfnClose, TIFF_READ_CALLBACK *pfnRead, TIFF_SEEK_CALLBACK *pfnSeek, TIFF_DRAW_CALLBACK *pfnDraw);
    void close();
    int decode(int x, int y, int iOptions);
    int getWidth();
    int getHeight();
    int getLastError();

  private:
    TIFFIMAGE _tiff;
};

// Due to unaligned memory causing an exception, we have to do these macros the slow way
#define INTELSHORT(p) ((*p) + (*(p+1)<<8))
#define INTELLONG(p) ((*p) + (*(p+1)<<8) + (*(p+2)<<16) + (*(p+3)<<24))
#define MOTOSHORT(p) (((*(p))<<8) + (*(p+1)))
#define MOTOLONG(p) (((*p)<<24) + ((*(p+1))<<16) + ((*(p+2))<<8) + (*(p+3)))
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
                  { ul = (ulBits >> ((REGISTER_WIDTH-14) - ulBitOff)) & 0x3fe; ulBitOff += black_l[ul]; iLen = black_l[ul+1];} \
                else {ul = (ulBits >> ((REGISTER_WIDTH - 10) - ulBitOff)) & 0x3fe; ulBitOff += white_s[ul]; iLen = white_s[ul+1];} \
     sCode += iLen; }}

#define CLIMBBLACK_NEW(pBuf, ulBitOff, ulBits, sCode) \
    { uint32_t ul; int iLen = 64; sCode = 0; while (iLen > 63) \
    { if (ulBitOff > (REGISTER_WIDTH-15)) \
      { pBuf += (ulBitOff>>3); ulBitOff &= 7; ulBits = MOTOLONG(pBuf); } \
      if ((ulBits << ulBitOff) < LONGBLACKCODEMASK) \
         { ul = (ulBits >> ((REGISTER_WIDTH-14) - ulBitOff)) & 0x3fe; ulBitOff += black_l[ul]; iLen = black_l[ul+1];} \
        else {ul = (ulBits >> ((REGISTER_WIDTH - 7) - ulBitOff)) & 0x7e; ulBitOff += black_s[ul]; iLen = black_s[ul+1];} \
     sCode += iLen; }}

#endif // __ONEBITGFX__
