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

#include "../../CCITT.h"

ONEBITGFX obg;

void OBGDraw(OBGFXDRAW *pDraw)
{
    
} /* OBGDraw() */

int main(int argc, const char * argv[]) {
    if(obg.open((uint8_t *)CCITT, sizeof(CCITT), OBGDraw))
    {
        printf("image size = %d x %d\n", obg.getWidth(), obg.getHeight());
        if (obg.decode(0,0,0))
        {
            printf("Successfully decoded\n");
        }
        obg.close();
    }
    return 0;
}
