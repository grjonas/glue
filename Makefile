
all:
	gcc main.c dependencies.c scanner.c parser.c -o main -g -std=c99 -Wall -Wextra -Werror
