CPPFLAGS=$(shell llvm-config-6.0 --cxxflags)
LDFLAGS=$(shell llvm-config-6.0 --ldflags --libs)

haskell_to_llvm: lex.yy.o parser.o ast.o
	clang++-6.0 $(LDFLAGS) -o $@ $^
lex.yy.o: lex.yy.c parser.tab.hpp
	clang++-6.0 $(CPPFLAGS) -Wno-deprecated -c -o $@ $<
lex.yy.c: lexer.lex
	flex $<
parser.o: parser.tab.cpp parser.tab.hpp
	clang++-6.0 $(CPPFLAGS) -c -o $@ $<
parser.tab.cpp parser.tab.hpp: parser.ypp
	bison -v -d $<
ast.o: ast.cpp ast.hpp
	clang++-6.0 $(CPPFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -f *~ *tab* lex.yy.c haskell_to_llvm *.o

