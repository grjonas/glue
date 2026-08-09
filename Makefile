
all:
	gcc \
		main.c \
		arena.c \
		dependencies.c \
		pattern.c \
		type_expr.c \
		expr.c \
		stmt.c \
		decl.c \
		type.c \
		diagnostic.c \
		print.c \
		scanner.c \
		parser.c \
		parser_definitions.c \
		parser_pattern.c \
		parser_type_expr.c \
		parser_expr.c \
		parser_stmt.c \
		resolver.c \
		inferer.c \
	-o main -g \
		-Wall \
    	-Wextra \
    	-Wno-sign-compare \
	# Hashmaps don't work with c99
	# gcc main.c dependencies.c scanner.c parser.c arena.c stmt.c type_expr.c decl.c type.c expr.c resolver.c inferer.c print.c -o main -g -std=c99 -Wall -Wextra -Wno-sign-compare
