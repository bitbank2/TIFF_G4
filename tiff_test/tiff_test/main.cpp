//
//  main.cpp
//  tiff_test
//
//  Created by Laurence Bank on 8/14/20.
//  Copyright © 2020 Laurence Bank. All rights reserved.
//

#include "OneBitGFX.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../../test_images/notes.h"

ONEBITGFX obg;

void OBGDraw(OBGFXDRAW *pDraw)
{
    printf("y = %d\n", pDraw->y);
} /* OBGDraw() */

uint8_t bitcount[16] = {0,1,1,2,1,2,2,2,1,2,2,2,2,2,2,3};

int main(int argc, const char * argv[]) {
//    int i;
//    for (i=0; i<256; i++)
//    {
//        uint8_t c, d;
//        d = (i & 0xc0) >> 4;
//        d |= (i & 0xc) >> 2;
//        c = bitcount[d] << 2;
//        d = (i & 0x30) >> 2;
//        d |= (i & 0x3);
//        c |= bitcount[d];
//        printf("0x%02x,", c);
//        if ((i & 15) == 15)
//            printf("\n");
//    }
//    return 0;
    
    printf("Size of ONEBITGFX struct = %d\n", (int)sizeof(obg));
    
    if(obg.openTIFF((uint8_t *)notes, sizeof(notes), OBGDraw))
    {
        obg.setDrawParameters(0.125f, OBGFX_PIXEL_2BPP, 0, 0, 400, 300);
        printf("image size = %d x %d\n", obg.getWidth(), obg.getHeight());
        if (obg.decode())
        {
            printf("Successfully decoded\n");
        }
        obg.close();
    }
    return 0;
}
