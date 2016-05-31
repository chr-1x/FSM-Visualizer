CPPFLAGS := -std=c++0x -g -Wno-write-strings

code_all := code/graphgen.cpp code/render.cpp code/nfa_parse.cpp code/graphgen_static_posix.cpp

all: 
	@mkdir -p build/
	@$(CC) $(CPPFLAGS) $(code_all) -o build/graphgen -lrt -lm
	@mkdir -p build/data
	@cp data/* build/data/
