// JPEG perf test
// Written by Larry Bank
// 
// Will open an arbitrary JPEG file if passed on the command line
// or will use the sample image (tulips)
//
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "../src/OneBitGFX.h"
#include "../test_images/notes.h"

OBGFXIMAGE obgi;

long micros()
{
long iTime;
struct timespec res;

    clock_gettime(CLOCK_MONOTONIC, &res);
    iTime = 1000000*res.tv_sec + res.tv_nsec/1000;

    return iTime;
} /* micros() */

void OBGDraw(OBGFXDRAW *pDraw)
{
} /* OBGDraw() */

int main(int argc, char *argv[])
{
long lTime;
int rc;

    printf("TIFF decoder demo\n");
    printf("Run without parameters to test in-memory decoding\n");
    printf("Or pass a filename\n\n");

    if (argc == 2)
        rc = OBGFX_openTIFFFile(&obgi, argv[1], OBGDraw);
    else
	rc = OBGFX_openTIFFRAM(&obgi, (uint8_t *)notes, sizeof(notes), OBGDraw);
    if (rc)
    {
        printf("Image opened, size = %d x %d\n", OBGFX_getWidth(&obgi), OBGFX_getHeight(&obgi));
        lTime = micros();
	if (OBGFX_decode(&obgi)) {
	    lTime = micros() - lTime;
            printf("full sized decode in %d us\n", (int)lTime);
	}
	else
	{
            printf("Decode failed, last error = %d\n", OBGFX_getLastError(&obgi));
	    return 0;
	}
	OBGFX_close(&obgi);
    }
    else
    {
	printf("open() failed, last error = %d\n", OBGFX_getLastError(&obgi));
	return 0;
    }

    return 0;
} /* main() */
