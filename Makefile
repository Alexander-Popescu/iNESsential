all:
	gcc -I SDL2/include -I json-c/include -L SDL2/lib -L json-c/lib -o NES NES.c -lmingw32 -lSDL2main -lSDL2_ttf -lSDL2 -ljson-c