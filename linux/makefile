CFLAGS=-c -Wall -O2 -I../src -D__LINUX__
LIBS = -lm -lpthread

all: demo

demo: onebitgfx.o main.o makefile
	$(CC) main.o onebitgfx.o $(LIBS) -o demo

onebitgfx.o: ../src/onebitgfx.c ../src/OneBitGFX.h makefile
	$(CC) $(CFLAGS) ../src/onebitgfx.c

main.o: main.c ../src/OneBitGFX.h makefile
	$(CC) $(CFLAGS) main.c

clean:
	rm *.o demo
