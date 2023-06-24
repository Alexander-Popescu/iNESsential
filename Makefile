all:
	gcc -I src/include -L src/lib -o NES NES.c -lmingw32 -lSDL2main -lSDL2