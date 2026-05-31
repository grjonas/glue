
all:
	gcc main.c dependencies.c scanner.c -o main -g -std=c99 -Wextra
