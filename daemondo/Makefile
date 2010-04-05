all: daemondo

daemondo: main.c
	gcc -framework SystemConfiguration -framework IOKit -framework CoreFoundation -o daemondo main.c

clean:
	rm -rf main.o daemondo
