all:
	gcc -I SDL2/include -L SDL2/lib -o NES NES.c -lmingw32 -lSDL2main -lSDL2