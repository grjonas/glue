
all:
	gcc main.c dependencies.c scanner.c parser.c arena.c stmt.c type.c expr.c resolver.c inferer.c print.c -o main -g -std=c99 -Wall -Wextra -Wno-sign-compare
