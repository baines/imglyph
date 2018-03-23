all: imglyph mkmask

imglyph: main.c
	gcc -g $< -o $@ -lm

mkmask: mkmask.c
	gcc -g $< -o $@ -lX11
