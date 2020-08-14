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
static int TIFFInit(TIFFIMAGE *pTIFF);
static int TIFFParseInfo(TIFFIMAGE *pPage);
static void TIFFGetMoreData(TIFFIMAGE *pPage);
static int DecodeTIFF(TIFFIMAGE *pImage);

/* Table of byte flip values to mirror-image incoming CCITT data */
static const uint8_t ucMirror[256]=
     {0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
      8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
      4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
      12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
      2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
      10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
      6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
      14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
      1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
      9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
      5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
      13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
      3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
      11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
      7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
      15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255};
/*
 The code tree that follows has: bit_length, decode routine
 These codes are for Group 4 (MMR) decoding

 01 = vertneg1, 11h = vert1, 20h = horiz, 30h = pass, 12h = vert2
 02 = vertneg2, 13h = vert3, 03 = vertneg3, 90h = trash
*/

static const uint8_t code_table[128] =
        {0x90, 0, 0x40, 0,       /* trash, uncompr mode - codes 0 and 1 */
         3, 7,                   /* V(-3) pos = 2 */
         0x13, 7,                /* V(3)  pos = 3 */
         2, 6, 2, 6,             /* V(-2) pos = 4,5 */
         0x12, 6, 0x12, 6,       /* V(2)  pos = 6,7 */
         0x30, 4, 0x30, 4, 0x30, 4, 0x30, 4,    /* pass  pos = 8->F */
         0x30, 4, 0x30, 4, 0x30, 4, 0x30, 4,
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,    /* horiz pos = 10->1F */
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,
/* V(-1) pos = 20->2F */
         1, 3, 1, 3, 1, 3, 1, 3,
         1, 3, 1, 3, 1, 3, 1, 3,
         1, 3, 1, 3, 1, 3, 1, 3,
         1, 3, 1, 3, 1, 3, 1, 3,
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3,   /* V(1)   pos = 30->3F */
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3,
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3,
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3};

/*
 Here are the Huffman address codes for run lengths
 first the short white codes (first 4 bits != 0)
*/
static const int16_t white_s[1024] =
        {-1,-1,-1,-1,-1,-1,-1,-1,8,29,8,29,8,30,8,30,
        8,45,8,45,8,46,8,46,7,22,7,22,7,22,7,22,
        7,23,7,23,7,23,7,23,8,47,8,47,8,48,8,48,
        6,13,6,13,6,13,6,13,6,13,6,13,6,13,6,13,
        7,20,7,20,7,20,7,20,8,33,8,33,8,34,8,34,
        8,35,8,35,8,36,8,36,8,37,8,37,8,38,8,38,
        7,19,7,19,7,19,7,19,8,31,8,31,8,32,8,32,
        6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,
        6,12,6,12,6,12,6,12,6,12,6,12,6,12,6,12,
        8,53,8,53,8,54,8,54,7,26,7,26,7,26,7,26,
        8,39,8,39,8,40,8,40,8,41,8,41,8,42,8,42,
        8,43,8,43,8,44,8,44,7,21,7,21,7,21,7,21,
        7,28,7,28,7,28,7,28,8,61,8,61,8,62,8,62,
        8,63,8,63,8,0,8,0,8,320,8,320,8,384,8,384,
        5,10,5,10,5,10,5,10,5,10,5,10,5,10,5,10,
        5,10,5,10,5,10,5,10,5,10,5,10,5,10,5,10,
        5,11,5,11,5,11,5,11,5,11,5,11,5,11,5,11,
        5,11,5,11,5,11,5,11,5,11,5,11,5,11,5,11,
        7,27,7,27,7,27,7,27,8,59,8,59,8,60,8,60,
        9,1472,9,1536,9,1600,9,1728,7,18,7,18,7,18,7,18,
        7,24,7,24,7,24,7,24,8,49,8,49,8,50,8,50,
        8,51,8,51,8,52,8,52,7,25,7,25,7,25,7,25,
        8,55,8,55,8,56,8,56,8,57,8,57,8,58,8,58,
        6,192,6,192,6,192,6,192,6,192,6,192,6,192,6,192,
        6,1664,6,1664,6,1664,6,1664,6,1664,6,1664,6,1664,6,1664,
        8,448,8,448,8,512,8,512,9,704,9,768,8,640,8,640,
        8,576,8,576,9,832,9,896,9,960,9,1024,9,1088,9,1152,
        9,1216,9,1280,9,1344,9,1408,7,256,7,256,7,256,7,256,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        5,128,5,128,5,128,5,128,5,128,5,128,5,128,5,128,
        5,128,5,128,5,128,5,128,5,128,5,128,5,128,5,128,
        5,8,5,8,5,8,5,8,5,8,5,8,5,8,5,8,
        5,8,5,8,5,8,5,8,5,8,5,8,5,8,5,8,
        5,9,5,9,5,9,5,9,5,9,5,9,5,9,5,9,
        5,9,5,9,5,9,5,9,5,9,5,9,5,9,5,9,
        6,16,6,16,6,16,6,16,6,16,6,16,6,16,6,16,
        6,17,6,17,6,17,6,17,6,17,6,17,6,17,6,17,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        6,14,6,14,6,14,6,14,6,14,6,14,6,14,6,14,
        6,15,6,15,6,15,6,15,6,15,6,15,6,15,6,15,
        5,64,5,64,5,64,5,64,5,64,5,64,5,64,5,64,
        5,64,5,64,5,64,5,64,5,64,5,64,5,64,5,64,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7};
uint32_t *pWhite_S_32 = (uint32_t *) &white_s[0];

/* the short black codes (first 4 bits != 0) */
static const int16_t black_s[128] =
       {-1,-1,-1,-1,-1,-1,-1,-1,6,9,6,8,5,7,5,7,
        4,6,4,6,4,6,4,6,4,5,4,5,4,5,4,5,
        3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,
        3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
        2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,
        2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};
uint32_t *pBlack_S_32 = (uint32_t *)&black_s[0];

/* The long black codes (first 4 bits == 0) */
#define EOL -9999   /* End of line */
#define EO1D -9998  /* End of 1D coding */
static const int16_t black_l[1024] =
    {1,0,1,0,12,EOL,12,EOL,1,-1,1,-1,1,-1,1,-1,
     1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,
     1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,12,EO1D,12,EO1D,
     1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,
     11,1792,11,1792,11,1792,11,1792,12,1984,12,1984,12,2048,12,2048,
     12,2112,12,2112,12,2176,12,2176,12,2240,12,2240,12,2304,12,2304,
     11,1856,11,1856,11,1856,11,1856,11,1920,11,1920,11,1920,11,1920,
     12,2368,12,2368,12,2432,12,2432,12,2496,12,2496,12,2560,12,2560,
     10,18,10,18,10,18,10,18,10,18,10,18,10,18,10,18,
     12,52,12,52,13,640,13,704,13,768,13,832,12,55,12,55,
     12,56,12,56,13,1280,13,1344,13,1408,13,1472,12,59,12,59,
     12,60,12,60,13,1536,13,1600,11,24,11,24,11,24,11,24,
     11,25,11,25,11,25,11,25,13,1664,13,1728,12,320,12,320,
     12,384,12,384,12,448,12,448,13,512,13,576,12,53,12,53,
     12,54,12,54,13,896,13,960,13,1024,13,1088,13,1152,13,1216,
     10,64,10,64,10,64,10,64,10,64,10,64,10,64,10,64,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     11,23,11,23,11,23,11,23,12,50,12,50,12,51,12,51,
     12,44,12,44,12,45,12,45,12,46,12,46,12,47,12,47,
     12,57,12,57,12,58,12,58,12,61,12,61,12,256,12,256,
     10,16,10,16,10,16,10,16,10,16,10,16,10,16,10,16,
     10,17,10,17,10,17,10,17,10,17,10,17,10,17,10,17,
     12,48,12,48,12,49,12,49,12,62,12,62,12,63,12,63,
     12,30,12,30,12,31,12,31,12,32,12,32,12,33,12,33,
     12,40,12,40,12,41,12,41,11,22,11,22,11,22,11,22,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     9,15,9,15,9,15,9,15,9,15,9,15,9,15,9,15,
     9,15,9,15,9,15,9,15,9,15,9,15,9,15,9,15,
     12,128,12,128,12,192,12,192,12,26,12,26,12,27,12,27,
     12,28,12,28,12,29,12,29,11,19,11,19,11,19,11,19,
     11,20,11,20,11,20,11,20,12,34,12,34,12,35,12,35,
     12,36,12,36,12,37,12,37,12,38,12,38,12,39,12,39,
     11,21,11,21,11,21,11,21,12,42,12,42,12,43,12,43,
     10,0,10,0,10,0,10,0,10,0,10,0,10,0,10,0,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12};
uint32_t *pBlack_L_32 = (uint32_t *)&black_l[0];


/* Table of vertical codes for G4 encoding */
/* code followed by length, starting with v(-3) */
static const uint8_t vtable[14] =
        {3,7,     /* V(-3) = 0000011 */
         3,6,     /* V(-2) = 000011  */
         3,3,     /* V(-1) = 011     */
         1,1,     /* V(0)  = 1       */
         2,3,     /* V(1)  = 010     */
         2,6,     /* V(2)  = 000010  */
         2,7};    /* V(3)  = 0000010 */


/* Group 3 Huffman codes ordered for MH encoding */
/* first, the terminating codes for white (code, length) */
static const uint8_t huff_white[128] =
        {0x35,8,7,6,7,4,8,4,0xb,4, /* 0,1,2,3,4 */
         0xc,4,0xe,4,0xf,4,0x13,5,0x14,5,7,5,8,5, /* 5,6,7,8,9,10,11 */
         8,6,3,6,0x34,6,0x35,6,0x2a,6,0x2b,6,0x27,7, /* 12,13,14,15,16,17,18 */
         0xc,7,8,7,0x17,7,3,7,4,7,0x28,7,0x2b,7, /* 19,20,21,22,23,24,25 */
         0x13,7,0x24,7,0x18,7,2,8,3,8,0x1a,8,0x1b,8, /* 26,27,28,29,30,31,32 */
         0x12,8,0x13,8,0x14,8,0x15,8,0x16,8,0x17,8,0x28,8, /* 33,34,35,36,37,38,39 */
         0x29,8,0x2a,8,0x2b,8,0x2c,8,0x2d,8,4,8,5,8, /* 40,41,42,43,44,45,46 */
         0xa,8,0xb,8,0x52,8,0x53,8,0x54,8,0x55,8,0x24,8, /* 47,48,49,50,51,52,53 */
         0x25,8,0x58,8,0x59,8,0x5a,8,0x5b,8,0x4a,8,0x4b,8, /* 54,55,56,57,58,59,60 */
         0x32,8,0x33,8,0x34,8};                        /* 61,62,63 */

/* now the white make-up codes */
static const uint8_t huff_wmuc[82] =
       {0,0,0x1b,5,0x12,5,0x17,6,0x37,7,0x36,8,   /* null,64,128,192,256,320 */
        0x37,8,0x64,8,0x65,8,0x68,8,0x67,8,0xcc,9, /* 384,448,512,576,640,704 */
        0xcd,9,0xd2,9,0xd3,9,0xd4,9,0xd5,9,    /* 768,832,896,960,1024 */
        0xd6,9,0xd7,9,0xd8,9,0xd9,9,0xda,9,    /* 1088,1152,1216,1280,1344 */
        0xdb,9,0x98,9,0x99,9,0x9a,9,0x18,6,    /* 1408,1472,1536,1600,1664 */
        0x9b,9,8,11,0xc,11,0xd,11,0x12,12,     /* 1728,1792,1856,1920,1984 */
        0x13,12,0x14,12,0x15,12,0x16,12,0x17,12, /* 2048,2112,2176,2240,2304 */
        0x1c,12,0x1d,12,0x1e,12,0x1f,12};       /* 2368,2432,2496,2560 */

/* black terminating codes */
static const uint8_t huff_black[128] =
      {0x37,10,2,3,3,2,2,2,3,3,                         /* 0,1,2,3,4 */
       3,4,2,4,3,5,5,6,4,6,4,7,5,7,                     /* 5,6,7,8,9,10,11 */
       7,7,4,8,7,8,0x18,9,0x17,10,0x18,10,8,10,         /* 12,13,14,15,16,17,18 */
       0x67,11,0x68,11,0x6c,11,0x37,11,0x28,11,0x17,11, /* 19,20,21,22,23,24 */
       0x18,11,0xca,12,0xcb,12,0xcc,12,0xcd,12,0x68,12, /* 25,26,27,28,29,30 */
       0x69,12,0x6a,12,0x6b,12,0xd2,12,0xd3,12,0xd4,12, /* 31,32,33,34,35,36 */
       0xd5,12,0xd6,12,0xd7,12,0x6c,12,0x6d,12,0xda,12, /* 37,38,39,40,41,42 */
       0xdb,12,0x54,12,0x55,12,0x56,12,0x57,12,0x64,12, /* 43,44,45,46,47,48 */
       0x65,12,0x52,12,0x53,12,0x24,12,0x37,12,0x38,12, /* 49,50,51,52,53,54 */
       0x27,12,0x28,12,0x58,12,0x59,12,0x2b,12,0x2c,12, /* 55,56,57,58,59,60 */
       0x5a,12,0x66,12,0x67,12};                        /* 61,62,63 */
/* black make up codes */
static const uint8_t huff_bmuc[82] =
       {0,0,0xf,10,0xc8,12,0xc9,12,0x5b,12,0x33,12, /* null,64,128,192,256,320 */
        0x34,12,0x35,12,0x6c,13,0x6d,13,0x4a,13,0x4b,13,   /* 384,448,512,576,640,704 */
        0x4c,13,0x4d,13,0x72,13,0x73,13,0x74,13,0x75,13,   /* 768,832,896,960,1024,1088 */
        0x76,13,0x77,13,0x52,13,0x53,13,0x54,13,0x55,13,   /* 1152,1216,1280,1344,1408,1472 */
        0x5a,13,0x5b,13,0x64,13,0x65,13,8,11,0xc,11,       /* 1536,1600,1664,1728,1792,1856 */
        0xd,11,0x12,12,0x13,12,0x14,12,0x15,12,0x16,12,    /* 1920,1984,2048,2112,2176,2240 */
        0x17,12,0x1c,12,0x1d,12,0x1e,12,0x1f,12};          /* 2304,2368,2432,2496,2560 */

//
// Helper functions for memory based images
//
static int32_t readMem(TIFFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;

    iBytesRead = iLen;
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos;
    if (iBytesRead <= 0)
       return 0;
    memcpy_P(pBuf, &pFile->pData[pFile->iPos], iBytesRead);
    pFile->iPos += iBytesRead;
    return iBytesRead;
} /* readMem() */

static int32_t seekMem(TIFFFILE *pFile, int32_t iPosition)
{
    if (iPosition < 0) iPosition = 0;
    else if (iPosition >= pFile->iSize) iPosition = pFile->iSize-1;
    pFile->iPos = iPosition;
    return iPosition;
} /* seekMem() */

//
// Memory initialization
//
int ONEBITGFX::open(uint8_t *pData, int iDataSize, TIFF_DRAW_CALLBACK *pfnDraw)
{
    memset(&_jpeg, 0, sizeof(JPEGIMAGE));
    _tiff.pfnRead = readMem;
    _tiff.pfnSeek = seekMem;
    _tiff.pfnDraw = pfnDraw;
    _tiff.pfnOpen = NULL;
    _tiff.pfnClose = NULL;
    _tiff.TIFFFile.iSize = iDataSize;
    _tiff.TIFFFile.pData = pData;
    return TIFFInit(&_tiff);
} /* open() */

int ONEBITGFX::getLastError()
{
    return _tiff.iError;
} /* getLastError() */

int ONEBITGFX::getWidth()
{
    return _tiff.iWidth;
} /* getWidth() */

int ONEBITGFX::getHeight()
{
    return _tiff.iHeight;
} /* getHeight() */

//
// File (SD/MMC) based initialization
//
int ONEBITGFX::open(char *szFilename, TIFF_OPEN_CALLBACK *pfnOpen, TIFF_CLOSE_CALLBACK *pfnClose, TIFF_READ_CALLBACK *pfnRead, TIFF_SEEK_CALLBACK *pfnSeek, TIFF_DRAW_CALLBACK *pfnDraw)
{
    memset(&_jpeg, 0, sizeof(TIFFIMAGE));
    _tiff.pfnRead = pfnRead;
    _tiff.pfnSeek = pfnSeek;
    _tiff.pfnDraw = pfnDraw;
    _tiff.pfnOpen = pfnOpen;
    _tiff.pfnClose = pfnClose;
    _tiff.TIFFFile.fHandle = (*pfnOpen)(szFilename, &_tiff.TIFFFile.iSize);
    if (_tiff.TIFFFile.fHandle == NULL)
       return 0;
    return TIFFInit(&_tiff);

} /* open() */

void ONEBITGFX::close()
{
    if (_tiff.pfnClose)
        (*_tiff.pfnClose)(_tiff.TIFFFile.fHandle);
} /* close() */

//
// Decode an image
// returns:
// 1 = good result
// 0 = error
//
int ONEBITGFX::decode(int x, int y, int iOptions)
{
    _tiff.iXOffset = x;
    _tiff.iYOffset = y;
    _tiff.iOptions = iOptions;
    return DecodeTIFF(&_tiff);
} /* decode() */
//
// The following functions are written in plain C and have no
// 3rd party dependencies, not even the C runtime library
//
//
// Initialize a TIFF file and callback access from a file on SD or memory
// returns 1 for success, 0 for failure
// Fills in the basic image info fields of the TIFFIMAGE structure
//
static int TIFFInit(TIFFIMAGE *pTIFF)
{
    return TIFFParseInfo(pTIFF, 0); // gather info for image
} /* TIFFInit() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : ClimbWhite(BUFFERED_BITS *)                                *
 *                                                                          *
 *  PURPOSE    : Retrieve the next Huffman code from G3 encoded data.       *
 *                                                                          *
 ****************************************************************************/
static int32_t ClimbWhite(BUFFERED_BITS *bb)
{
int32_t sCode = 0; /* Start out with total = 0 */
int iLen;
//uint32_t ulTabVal;
#ifdef _64BITS
uint64_t ul;
#else // 32bits
uint32_t ul;
#endif // _64BITS

   iLen = 64; /* force first pass through loop */
   while (iLen > 63)  /* Until a terminating code is found */
      {
      if (bb->ulBitOff > (REGISTER_WIDTH-17)) // need to fetch more bits
         {
         bb->pBuf += (bb->ulBitOff >> 3);
         bb->ulBitOff &= 7;
#ifdef _64BITS
         bb->ulBits = MOTOEXTRALONG(bb->pBuf);
#else // 32bits
         bb->ulBits = MOTOLONG(bb->pBuf);
#endif // _64BITS
         }
      if ((bb->ulBits << bb->ulBitOff) < LONGWHITECODEMASK) /* first 7 bits == 0 -> Long codeword? */
         {
//         ul = (bb->ulBits >> ((REGISTER_WIDTH-13) - bb->ulBitOff)) & 0x1ff; // we care about the lower 9 bits of a max len 13-bit code
//         ulTabVal = pBlack_L_32[ul];
//         bb->ulBitOff += ulTabVal & 0xff;
//         iLen = ulTabVal >> 16;
         ul = (bb->ulBits >> ((REGISTER_WIDTH - 14) - bb->ulBitOff)) & 0x3fe; // we care about the lower 9 bits of a max len 13-bit code
         bb->ulBitOff += black_l[ul]; // long codes are the same for black and white runs
         iLen = black_l[ul + 1];
         }
      else /* Short codeword */
         {
//          ul = (bb->ulBits >> ((REGISTER_WIDTH - 9) - bb->ulBitOff)) & 0x1ff; /* It is a 1-9 bit code */
//          ulTabVal = pWhite_S_32[ul];
//          bb->ulBitOff += ulTabVal & 0xff; /* Add the code length to bit offset */
//          iLen = ulTabVal >> 16; /* Get the run length */
         ul = (bb->ulBits >> ((REGISTER_WIDTH - 10) - bb->ulBitOff)) & 0x3fe; /* It is a 1-9 bit code */
         bb->ulBitOff += white_s[ul]; /* Add the code length to bit offset */
         iLen = white_s[ul+1]; /* Get the run length */
         } /* Short codes */
      sCode += iLen;
      } /* while */

   return sCode;
} /* ClimbWhite() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : ClimbBlack(BUFFERED_BITS *)                                *
 *                                                                          *
 *  PURPOSE    : Retrieve the next Huffman code from G3 encoded data.       *
 *                                                                          *
 ****************************************************************************/
static int32_t ClimbBlack(BUFFERED_BITS *bb)
{
int32_t sCode = 0; /* Start out with total = 0 */
int iLen;
#ifdef _64BITS
uint64_t ul;
#else // 32bits
uint32_t ul;
#endif // _64BITS

   iLen = 64; /* force first pass through loop */

   while (iLen > 63)  /* Until a terminating code is found */
      {
      if (bb->ulBitOff > (REGISTER_WIDTH - 15)) // need to read more bits
         {
         bb->pBuf += (bb->ulBitOff >> 3);
         bb->ulBitOff &= 7;
#ifdef _64BITS
         bb->ulBits = MOTOEXTRALONG(bb->pBuf);
#else // 32bits
         bb->ulBits = MOTOLONG(bb->pBuf);
#endif // _64BITS
      }
      if ((bb->ulBits << bb->ulBitOff) < LONGBLACKCODEMASK) /* first 4 bits == 0 -> Long codeword? */
         {
         ul = (bb->ulBits >> ((REGISTER_WIDTH-14)-bb->ulBitOff)) & 0x3fe;
         bb->ulBitOff += black_l[ul];
         iLen = black_l[ul + 1];
         }
      else /* Short codeword */
         {
         ul = (bb->ulBits >> ((REGISTER_WIDTH - 7) - bb->ulBitOff)) & 0x7e;
         bb->ulBitOff += black_s[ul]; /* Add the code length to bit offset */
         iLen = black_s[ul+1]; /* Get the run length */
         } /* Short codes */
      sCode += iLen;
      } /* while */

   return sCode;
} /* ClimbBlack() */
//
// TIFFSHORT
// read a 16-bit unsigned integer from the given pointer
// and interpret the data as big endian (Motorola) or little endian (Intel)
//
static uint16_t TIFFSHORT(unsigned char *p, int bMotorola)
{
    unsigned short s;

    if (bMotorola)
        s = *p * 0x100 + *(p+1); // big endian (AKA Motorola byte order)
    else
        s = *p + *(p+1)*0x100; // little endian (AKA Intel byte order)
    return s;
} /* TIFFSHORT() */
//
// TIFFLONG
// read a 32-bit unsigned integer from the given pointer
// and interpret the data as big endian (Motorola) or little endian (Intel)
//
static uint32_t TIFFLONG(unsigned char *p, int bMotorola)
{
    uint32_t l;

    if (bMotorola)
        l = *p * 0x1000000 + *(p+1) * 0x10000 + *(p+2) * 0x100 + *(p+3); // big endian
    else
        l = *p + *(p+1) * 0x100 + *(p+2) * 0x10000 + *(p+3) * 0x1000000; // little endian
    return l;
} /* TIFFLONG() */
//
// TIFFVALUE
// read an integer value encoded in a TIFF TAG (12-byte structure)
// and interpret the data as big endian (Motorola) or little endian (Intel)
//
static int TIFFVALUE(unsigned char *p, int bMotorola)
{
    int i, iType;
    
    iType = TIFFSHORT(p+2, bMotorola);
    /* If pointer to a list of items, must be a long */
    if (TIFFSHORT(p+4, bMotorola) > 1)
    {
        iType = 4;
    }
    switch (iType)
    {
        case 3: /* Short */
            i = TIFFSHORT(p+8, bMotorola);
            break;
        case 4: /* Long */
        case 7: // undefined (treat it as a long since it's usually a multibyte buffer)
            i = TIFFLONG(p+8, bMotorola);
            break;
        case 6: // signed byte
            i = (signed char)p[8];
            break;
        case 2: /* ASCII */
        case 5: /* Unsigned Rational */
        case 10: /* Signed Rational */
            i = TIFFLONG(p+8, bMotorola);
            break;
        default: /* to suppress compiler warning */
            i = 0;
            break;
    }
    return i;
    
} /* TIFFVALUE() */

static int TIFFParseInfo(TIFFIMAGE *pPage)
{
    int iBytesRead;
    int i, iOffset, iTableOffset;
    uint8_t bMotorola, ucTable, *s = pPage->ucFileBuf;
    uint16_t usTagCount;
    int IFD, iTag, iBpp = 1, iSamples = 1;
    int iT6Options = 0, iRowsPerStrip = 0, iStripSize = 0;

    iBytesRead = (*pPage->pfnRead)(&pPage->TIFFFile, s, 8);
    if (iBytesRead != 8)
    {
        pPage->iError = TIFF_INVALID_FILE;
        return 0;
    }
    if (s[0] != s[1] || s[0] != 'M' || s[0] != 'I')
    {
        pPage->iError = TIFF_INVALID_FILE;
        return 0; // not a TIFF file
    }
    bMotorola = (s[0] == 'M');
    IFD = TIFFLONG(&s[4], bMotorola); // get IFD
    if (IFD > pPage->iSize - (2 + 4*TIFF_TAG_SIZE) // bad value
    {
        pPage->iError = TIFF_INVALID_FILE;
        return 0;
    }
    (*pPage->pfnSeek)(&pPage->TIFFFile, IFD);
    iBytesRead = (*pPage->pfnRead)(&pPage->TIFFFile, s, 2); // get tag count
    usTagCount = TIFFSHORT(s, bMotorola);
    if (iBytesRead != 2 || usTagCount < 4 || usTagCount < MAX_TIFF_TAGS)
    {
        pPage->iError = TIFF_INVALID_FILE; // something corrupt/wrong
        return 0;
    }
    // read just enough for the tag data
    iBytesRead = (*pPage->pfnRead)(&pPage->TIFFFile, s, TIFF_TAG_SIZE * usTagCount);
    if (iBytesRead != usTagCount * TIFF_TAG_SIZE)
    {
        pPage->iError = TIFF_INVALID_FILE; // something corrupt/wrong
        return 0;
    }
    iOffset = 0; /* Start at offset of first marker */
    for (i=0; i<usTagCount; i++)
    {
        iTag = TIFFSHORT(&s[iOffset], bMotorola);
        usLen = MOTOSHORT(&s[iOffset]); // marker length

        switch (iTag)
        {
            case 256: // width
                pPage->iWidth = PILTIFFVALUE(p, bMotorola);
                break;
            case 257: // height
                pPage->iHeight = PILTIFFVALUE(p, bMotorola);
                break;
            case 258: // bits per sample
                iBpp = PILTIFFVALUE(p, bMotorola);
                break;
            case 259: // compression
                pPage->ucCompression = (uint8_t)PILTIFFVALUE(p, bMotorola);
                break;
            case 262: // photometric value
                pPage->ucPhotometric = (uint8_t)PILTIFFVALUE(p, bMotorola);
                break;
            case 266: // fill order
                pPage->ucFillOrder = (uint8_t)PILTIFFVALUE(p, bMotorola);
                break;
            case 273: // strip info
                break;
            case 277: // samples per pixel
                iSamples = PILTIFFVALUE(p, bMotorola);
                break;
            case 278: // rows per strip
                iRowsPerStrip = PILTIFFVALUE(p, bMotorola);
                break;
            case 279: // strip size
                iStripSize = PILTIFFVALUE(p, bMotorola);
                break;
            case 293: // T6 option flags
                iT6Options = PILTIFFVALUE(p, bMotorola);
                break;
        } // while
        iOffset += TIFF_TAG_SIZE;
    }
    return 1;
} /* TIFFParseInfo() */

static int TIFFDrawLine(TIFFIMAGE *pPage, int y, int16_t *CurFlips)
{
    
} /* TIFFDrawLine() */

static int DecodeG4(TIFFIMAGE *pPage)
{
int x, y;
int i, xsize;
//int iCur, iRef;
uint8_t *buf, *pBufEnd;
int32_t sCode;
int16_t *t1, *pCur, *pRef;
int16_t *CurFlips, *RefFlips;
int run, tot_run, tot_run1;
uint32_t lBits;
int iRow;
    uint32_t ulBits, ulBitOff;
uint8_t *pBuf;

   xsize = pPage->iWidth; /* For performance reasons */

   buf = &pPage->pData[pPage->iOffset];
    CurFlips = pPage->CurFlips;
    RefFlips = pPage->RefFlips;
    
   /* Seed the current and reference line with XSIZE for V(0) codes */
   for (i=0; i<xsize-2; i++)
    {
      RefFlips[i] = xsize;
      CurFlips[i] = xsize;
    }
   /* Prefill both current and reference lines with 7fff to prevent it from
      walking off the end if the data gets bunged and the current X is > XSIZE
      3-16-94 */
   CurFlips[i] = RefFlips[i] = 0x7fff;
   CurFlips[i+1] = RefFlips[i+1] = 0x7fff;

   pBufEnd = &buf[iStripSize + 4]; // allow for last longword read
   pBuf = buf;
   ulBitOff = 0;
   pBuf = buf;
//
// Some files may have leading 0's that would confuse the decoder
// Valid G4 data can't begin with a 0
//
   while (pBuf < pBufEnd && pBuf[0] == 0)
   { pBuf++; }

   ulBits = MOTOLONG(pBuf); // load 32 bits to start
    
   /* Decode the image */
   for (y=0; y < pPage->iHeight && pBuf < pBufEnd; y++)
      {
      signed int a0, a0_c, a0_p, b1;
//g4_restart:
//      iCur = iRef = 0; /* Point to start of current and reference line */
      pCur = CurFlips;
      pRef = RefFlips;
      a0 = -1;
      a0_c = 0; /* start just to left and white */
      while (a0 < xsize && pBuf < pBufEnd)   /* Decode this line */
         {
         if (ulBitOff > (REGISTER_WIDTH-8)) // need at least 7 unused bits
            {
            pBuf += (ulBitOff >> 3);
            ulBitOff &= 7;
            ulBits = MOTOLONG(pBuf);
            }
         if ((int32_t)(ulBits << ulBitOff) < 0)  /* V(0) code */
            {
            a0 = *pRef++;
            ulBitOff++; // 1 bit
            a0_c = 1 - a0_c; /* color change */
            *pCur++ = a0;
            }
         else /* Slow method */
            {
            lBits = (ulBits >> ((REGISTER_WIDTH - 8) - ulBitOff)) & 0xfe; /* Only the first 7 bits are useful */
            sCode = code_table[lBits]; /* Get the code word */
            ulBitOff += code_table[lBits+1]; /* Get the code length */
            switch (sCode)
               {
               case 1: /* V(-1) */
               case 2: /* V(-2) */
               case 3: /* V(-3) */
                  a0 = *pRef - sCode;  /* A0 = B1 - x */
                  *pCur++ = a0;
                  if (pRef == RefFlips)
                     pRef += 2;
                  pRef--;
                  while (a0 >= *pRef)
                     pRef += 2;
                  a0_c = 1-a0_c; /* color change */
                  break;

               case 0x11: /* V(1) */
               case 0x12: /* V(2) */
               case 0x13: /* V(3) */
                  a0 = *pRef++;   /* A0 = B1 */
                  b1 = a0;
                  a0 += sCode & 7;      /* A0 = B1 + x */
                  if (b1 != xsize && a0 < xsize)
                     {
                     while (a0 >= *pRef)
                        pRef += 2;
                     }
                  if (a0 > xsize)
                     a0 = xsize;
                  a0_c = 1-a0_c; /* color change */
                  *pCur++ = a0;
                  break;

               case 0x20: /* Horizontal codes */
                  a0_p = a0;
                  if (a0 < 0)
                     a0_p = 0;
                  if (a0_c) /* Black case */
                     {
                     CLIMBBLACK_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbBlack(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = PIL_ERROR_DECOMP;
                        goto pilreadg4z;
                        }
                     tot_run = sCode;
                     CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbWhite(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = PIL_ERROR_DECOMP;
                        goto pilreadg4z;
                        }
                     tot_run1 = sCode;
                     }
                  else  /* White case */
                     {
                      CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbWhite(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = PIL_ERROR_DECOMP;
                        goto pilreadg4z;
                        }
                     tot_run = sCode;
                     CLIMBBLACK_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbBlack(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = PIL_ERROR_DECOMP;
                        goto pilreadg4z;
                        }
                     tot_run1 = sCode;
                     }
                  a0 = a0_p + tot_run;
                  *pCur++ = a0;
                  a0 += tot_run1;
                  if (a0 < xsize)
                     while (a0 >= *pRef)
                        pRef += 2;
                  *pCur++ = a0;
                  break;

               case 0x30: /* Pass code */
                  pRef++;         /* A0 = B2, iRef+=2 */
                  a0 = *pRef++;
                  break;
               case 0x40: /* Uncompressed mode */
                  lBits = ulBits << ulBitOff;
                  lBits &= 0xffc00000;
                  if (lBits != 0x3c00000)  /* If not entering uncompressed mode */
                     {
                     pPage->iError = PIL_ERROR_DECOMP;
                     goto pilreadg4z;
                     }
                  ulBitOff += 10;
                  tot_run = 0; /* Current run length */
                  if ((ulBits << ulBitOff) & TOP_BIT)
                     goto blkst; /* Black start */
            whtst:
                  if (/*iCur >= xsize || */pBuf > pBufEnd)    /* Something is wrong, stop */
                     {
                     pPage->iError = PIL_ERROR_DECOMP;
                     goto pilreadg4z;
                     }
                  tot_run1++;
                  ulBitOff++;
                  if (ulBitOff > (REGISTER_WIDTH - 8))
                     {
                     pBuf += (ulBitOff >> 3);
                     ulBitOff &= 7;
                     ulBits = MOTOLONG(pBuf);
                     }
                  lBits = ulBits << ulBitOff;
                  if ((lBits & TOP_BIT) == 0)
                     goto whtst;
        /* Check for end of mode stuff */
                  if (tot_run1 == 5)
                     {
                     tot_run += 5;
                     tot_run1 = -1;
                     goto whtst; /* Keep looking for white */
                     }
                  if (tot_run1 >= 6) /* End of uncomp data */
                     {
                     tot_run += tot_run1 - 6; /* Get the number of extra 0's */
                     if (tot_run) /* Something to store? */
                        {
                        a0 += tot_run;
                        *pCur++ = a0;
                        }
     /* Get the last bit to see what the next color is */
                     ulBitOff++;
                     if (ulBitOff > (REGISTER_WIDTH - 8))
                        {
                        pBuf += (ulBitOff >> 3);
                        ulBitOff &= 7;
                        ulBits = GETMAXMOTOBITS(pBuf);
                        }
                     lBits = ulBits << ulBitOff;
                     lBits >>= (REGISTER_WIDTH - 1); /* Turn it into 0/1 for color */
                     if ((signed int)lBits != a0_c) /* If color changed, bump up ref line */
                        pRef++;
                     a0_c = sCode; /* This is the new color */
/* Re-align reference line with new position */
                     while (*pRef <= a0) // && iRef <= xsize) - DEBUG
                        pRef += 2;
                     break; /* Continue normal G4 decoding */
                     }
                  else
                     {
                     tot_run += tot_run1;
                     a0 += tot_run; /* Add to current x */
                     *pCur++ = a0;
                     tot_run = 0;
                     tot_run1 = 0;
                     }
            blkst:
                  if (/*iCur >= xsize || */ pBuf > pBufEnd)    /* Something is wrong, stop */
                     {
                     pPage->iError = PIL_ERROR_DECOMP;
                     goto pilreadg4z;
                     }
                  tot_run++;
                  ulBitOff++;
                  if (ulBitOff > (REGISTER_WIDTH - 8))
                     {
                     pBuf += (ulBitOff >> 3);
                     ulBitOff &= 7;
                     ulBits = MOTOLONG(pBuf);
                     }
                  lBits = ulBits << ulBitOff;
                  if ((lBits & TOP_BIT) == TOP_BIT)
                     goto blkst;
                  a0 += tot_run;
                  *pCur++ = a0;
                  tot_run = 0;
                  goto whtst;
               default: /* possible ERROR! */
                  /* A G4 page can end early with 2 EOL's */
                   CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                  sCode = ClimbWhite(&bb);
                  if (sCode != EOL)
                     {
                     pPage->iError = PIL_ERROR_DECOMP;
                     goto pilreadg4z;
                     }
                   CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                  sCode = ClimbWhite(&bb);
                  if (sCode != EOL)
                     {
                     pPage->iError = PIL_ERROR_DECOMP;
                     goto pilreadg4z;
                     }
                  goto pilreadg4z; /* Leave gracefully */
               } /* switch */
            } /* Slow climb */
         }
      /*--- Convert flips data into run lengths ---*/
  g4eol:
      *pCur++ = xsize;  /* Terminate the line properly */
      *pCur++ = xsize;
      *pCur++ = xsize;

        // Draw the current line
      TIFFDrawLine(pPage, y, CurFlips);
      /*--- Swap current and reference lines ---*/
      t1 = RefFlips;
      RefFlips = CurFlips;
      CurFlips = t1;
      } /* for */
  pilreadg4z:
//    pOutPage->iLinesDecoded = y; // tell caller how many lines successfully decoded
   if (pPage->iOptions & PIL_CONVERT_IGNORE_ERRORS)
      pPage->iError = 0; // suppress errors

   return (pPage->iError == 0);

} /* PILReadG4() */
