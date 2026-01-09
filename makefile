all: libWad.a

libWad.a: Wad.o
	ar rcs libWad.a Wad.o

Wad.o: Wad.cpp Wad.h
	g++ -std=c++11 -Wall -c Wad.cpp -o Wad.o

clean:
	rm -f Wad.o libWad.a