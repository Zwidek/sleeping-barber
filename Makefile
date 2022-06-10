main: main.c
	gcc main.c -o a -pthread -std=c99 -lm -O3

clean:
	rm main
