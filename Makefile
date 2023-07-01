all:
	gcc -I SDL2/include -L SDL2/lib -o NES NES.c -lmingw32 -lSDL2main -lSDL2 -I SDL2_ttf/include -L SDL2_ttf/lib -lSDL2_ttf 