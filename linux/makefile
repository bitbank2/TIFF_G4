CFLAGS=-c -Wall -O2 -I../src -D__LINUX__
LIBS = -lm -lpthread

all: demo

demo: tiffg4.o main.o makefile
	$(CC) main.o tiffg4.o $(LIBS) -o demo

tiffg4.o: ../src/tiffg4.c ../src/TIFF_G4.h makefile
	$(CC) $(CFLAGS) ../src/tiffg4.c

main.o: main.c ../src/TIFF_G4.h makefile
	$(CC) $(CFLAGS) main.c

clean:
	rm *.o demo
