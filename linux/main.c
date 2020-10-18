// TIFF G4 perf test
// Written by Larry Bank
// 
// Will open an arbitrary TIFF file if passed on the command line
// or will use the sample image (notes)
//
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "../src/TIFF_G4.h"
#include "../test_images/notes.h"

TIFFIMAGE tiff;

long micros()
{
long iTime;
struct timespec res;

    clock_gettime(CLOCK_MONOTONIC, &res);
    iTime = 1000000*res.tv_sec + res.tv_nsec/1000;

    return iTime;
} /* micros() */

void TIFFDraw(TIFFDRAW *pDraw)
{
} /* TIFFDraw() */

int main(int argc, char *argv[])
{
long lTime;
int rc;

    printf("TIFF decoder demo\n");
    printf("Run without parameters to test in-memory decoding\n");
    printf("Or pass a filename\n\n");

    if (argc == 2)
        rc = TIFF_openTIFFFile(&tiff, argv[1], TIFFDraw);
    else
	rc = TIFF_openTIFFRAM(&tiff, (uint8_t *)notes, sizeof(notes), TIFFDraw);
    if (rc)
    {
        printf("Image opened, size = %d x %d\n", TIFF_getWidth(&tiff), TIFF_getHeight(&tiff));
        lTime = micros();
	if (TIFF_decode(&tiff)) {
	    lTime = micros() - lTime;
            printf("full sized decode in %d us\n", (int)lTime);
	}
	else
	{
            printf("Decode failed, last error = %d\n", TIFF_getLastError(&tiff));
	    return 0;
	}
	TIFF_close(&tiff);
    }
    else
    {
	printf("open() failed, last error = %d\n", TIFF_getLastError(&tiff));
	return 0;
    }

    return 0;
} /* main() */
