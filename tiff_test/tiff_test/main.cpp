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
OBGFXWINDOW obgw;

void OBGDraw(OBGFXDRAW *pDraw)
{
    printf("y = %d\n", pDraw->y);
} /* OBGDraw() */

uint8_t bitcount[16] = {0,1,1,2,1,2,2,2,1,2,2,2,2,2,2,3};

int main(int argc, const char * argv[]) {
//    int i;
    
    
//    for (i=0; i<256; i++)
//    {
//        uint8_t c = bitcount[i & 0xf] | ((bitcount[i>>4]) << 2);
//        printf("0x%02x,", c);
//        if ((i & 15) == 15)
//            printf("\n");
//    }
//    return 0;
    
    printf("Size of ONEBITGFX struct = %d\n", (int)sizeof(obg));
    
    if(obg.openTIFF((uint8_t *)notes, sizeof(notes), OBGDraw))
    {
        memset(&obgw, 0, sizeof(obgw));
        obgw.iScale = 0x2000;
        obgw.iWidth = 400;
        obgw.ucPixelType = OBGFX_PIXEL_1BPP;
        printf("image size = %d x %d\n", obg.getWidth(), obg.getHeight());
        if (obg.decode(&obgw))
        {
            printf("Successfully decoded\n");
        }
        obg.close();
    }
    return 0;
}
