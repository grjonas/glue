build:
	gcc \
		arena.c \
		dependencies.c \
		pattern.c \
		token.c \
		type_expr.c \
		expr.c \
		stmt.c \
		decl.c \
		type.c \
		diagnostic.c \
		print.c \
		scanner.c \
		parser.c \
		parser_pattern.c \
		parser_type_expr.c \
		parser_expr.c \
		parser_stmt.c \
		resolver.c \
		inferer.c \
		encoder.c \
		runtime.c \
		main.c \
	-o main -g \
		-Wall \
    	-Wextra \
    	-Wno-sign-compare
# Hashmaps don't work with c99

test:
	gcc \
		arena.c \
		dependencies.c \
		pattern.c \
		token.c \
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
		test_dependencies.c \
		test.c \
		main_test.c \
	-o main_test -g \
		-Wall \
    	-Wextra \
    	-Wno-sign-compare
	./main_test
