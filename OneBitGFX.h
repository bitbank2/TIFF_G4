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
#define MAX_BUFFERED_PIXELS 2048
#ifdef __AVR__
#define MAX_IMAGE_WIDTH 128
#define FILE_BUF_SIZE 256
#else
#define FILE_BUF_SIZE 2048
#define MAX_IMAGE_WIDTH 2600
#endif
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

typedef struct obgfx_file_tag
{
  int32_t iPos; // current file position
  int32_t iSize; // file size
  uint8_t *pData; // memory file pointer
  void * fHandle; // class pointer to File/SdFat or whatever you want
} OBGFXFILE;

typedef struct obgfx_draw_tag
{
    int x, y; // upper left corner of current MCU
    int iWidth, iHeight; // size of this MCU
    int iBpp; // bit depth of the pixels (8 or 16)
    uint16_t *pPixels; // 16-bit pixels
} OBGFXDRAW;

// Callback function prototypes
typedef int32_t (OBGFX_READ_CALLBACK)(OBGFXFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (OBGFX_SEEK_CALLBACK)(OBGFXFILE *pFile, int32_t iPosition);
typedef void (OBGFX_DRAW_CALLBACK)(OBGFXDRAW *pDraw);
typedef void * (OBGFX_OPEN_CALLBACK)(char *szFilename, int32_t *pFileSize);
typedef void (OBGFX_CLOSE_CALLBACK)(void *pHandle);

//
// our private structure to hold a TIFF image decode state
//
typedef struct obgfx_image_tag
{
    int iWidth, iHeight; // image size
    int iXOffset, iYOffset; // placement on the display
    int iError;
    int iOptions;
    int iVLCOff, iVLCSize;
    int iStripSize, iStripOffset;
    uint8_t ucCompression, ucPhotometric, ucFillOrder;
    OBGFX_READ_CALLBACK *pfnRead;
    OBGFX_SEEK_CALLBACK *pfnSeek;
    OBGFX_DRAW_CALLBACK *pfnDraw;
    OBGFX_OPEN_CALLBACK *pfnOpen;
    OBGFX_CLOSE_CALLBACK *pfnClose;
    OBGFXFILE OBGFXFile;
    int16_t CurFlips[MAX_IMAGE_WIDTH];
    int16_t RefFlips[MAX_IMAGE_WIDTH];
    uint8_t ucFileBuf[FILE_BUF_SIZE]; // holds temp data and pixel stack
} OBGFXIMAGE;

//
// The ONEBITGFX class wraps portable C code which does the actual work
//
class ONEBITGFX
{
  public:
    int openTIFF(uint8_t *pData, int iDataSize, OBGFX_DRAW_CALLBACK *pfnDraw);
    int openTIFF(char *szFilename, OBGFX_OPEN_CALLBACK *pfnOpen, OBGFX_CLOSE_CALLBACK *pfnClose, OBGFX_READ_CALLBACK *pfnRead, OBGFX_SEEK_CALLBACK *pfnSeek, OBGFX_DRAW_CALLBACK *pfnDraw);
    int openRAW(int iWidth, int iHeight, int iFillOrder, uint8_t *pData, int iDataSize, OBGFX_DRAW_CALLBACK *pfnDraw);
    void close();
    int decode(int x, int y, int iOptions);
    int getWidth();
    int getHeight();
    int getLastError();

  private:
    OBGFXIMAGE _obgfx;
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
