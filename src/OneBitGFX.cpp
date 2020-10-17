//
// One Bit GFX
// A TIFF / 1-bpp image library
// written by Larry Bank
// bitbank@pobox.com
// Arduino port started 8/10/2020
// Original code written 20 years ago :)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "OneBitGFX.h"

// forward references
static int OBGFXInit(OBGFXIMAGE *pTIFF);
static int OBGFXParseInfo(OBGFXIMAGE *pPage);
static void OBGFXGetMoreData(OBGFXIMAGE *pPage);
static int Decode(OBGFXIMAGE *pImage);

#include "tiffg4.c"

//
// Memory initialization
// Open a TIFF file and parse the header
//
int ONEBITGFX::openTIFF(uint8_t *pData, int iDataSize, OBGFX_DRAW_CALLBACK *pfnDraw)
{
    memset(&_obgfx, 0, sizeof(OBGFXIMAGE));
    _obgfx.pfnRead = readMem;
    _obgfx.pfnSeek = seekMem;
    _obgfx.pfnDraw = pfnDraw;
    _obgfx.pfnOpen = NULL;
    _obgfx.pfnClose = NULL;
    _obgfx.OBGFXFile.iSize = iDataSize;
    _obgfx.OBGFXFile.pData = pData;
    return OBGFXInit(&_obgfx);
} /* openTIFF() */
//
// Work with 'headerless' compressed G4 data
// Pass the width, height and fill order
//
int ONEBITGFX::openRAW(int iWidth, int iHeight, int iFillOrder, uint8_t *pData, int iDataSize, OBGFX_DRAW_CALLBACK *pfnDraw)
{
    memset(&_obgfx, 0, sizeof(OBGFXIMAGE));
    _obgfx.pfnRead = readMem;
    _obgfx.pfnSeek = seekMem;
    _obgfx.pfnDraw = pfnDraw;
    _obgfx.pfnOpen = NULL;
    _obgfx.pfnClose = NULL;
    _obgfx.OBGFXFile.iSize = iDataSize;
    _obgfx.OBGFXFile.pData = pData;
    _obgfx.iWidth = iWidth;
    _obgfx.iHeight = iHeight;
    _obgfx.ucFillOrder = (uint8_t)iFillOrder;
    return 1;
} /* openRAW() */

int ONEBITGFX::getLastError()
{
    return _obgfx.iError;
} /* getLastError() */

int ONEBITGFX::getWidth()
{
    return _obgfx.iWidth;
} /* getWidth() */

int ONEBITGFX::getHeight()
{
    return _obgfx.iHeight;
} /* getHeight() */

//
// File (SD/MMC) based initialization
//
int ONEBITGFX::openTIFF(const char *szFilename, OBGFX_OPEN_CALLBACK *pfnOpen, OBGFX_CLOSE_CALLBACK *pfnClose, OBGFX_READ_CALLBACK *pfnRead, OBGFX_SEEK_CALLBACK *pfnSeek, OBGFX_DRAW_CALLBACK *pfnDraw)
{
    memset(&_obgfx, 0, sizeof(OBGFXIMAGE));
    _obgfx.pfnRead = pfnRead;
    _obgfx.pfnSeek = pfnSeek;
    _obgfx.pfnDraw = pfnDraw;
    _obgfx.pfnOpen = pfnOpen;
    _obgfx.pfnClose = pfnClose;
    _obgfx.OBGFXFile.fHandle = (*pfnOpen)(szFilename, &_obgfx.OBGFXFile.iSize);
    if (_obgfx.OBGFXFile.fHandle == NULL)
       return 0;
    return OBGFXInit(&_obgfx);

} /* openTIFF() */

void ONEBITGFX::close()
{
    if (_obgfx.pfnClose)
        (*_obgfx.pfnClose)(_obgfx.OBGFXFile.fHandle);
} /* close() */

void ONEBITGFX::setDrawParameters(float scale, int iPixelType, int iStartX, int iStartY, int iWidth, int iHeight)
{
    _obgfx.window.iScale = (uint32_t)(scale * 65536.0f); // convert to uint32
    _obgfx.window.x = iStartX;
    _obgfx.window.y = iStartY; // upper left corner of interest (source pixels)
    _obgfx.window.iWidth = iWidth; // width of destination window (for clipping purposes)
    _obgfx.window.iHeight = iHeight;
//    uint8_t *p4BPP; // user-supplied buffer for 4-bpp grayscale output
    _obgfx.window.ucPixelType = (uint8_t)iPixelType;
} /* setDrawParameters() */
//
// Decode an image
// returns:
// 1 = good result
// 0 = error
//
int ONEBITGFX::decode()
{
    return Decode(&_obgfx);
} /* decode() */
