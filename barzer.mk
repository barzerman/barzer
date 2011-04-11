FLAGS := $(FLAGS) 
CFLAGS := $(FLAGS) -I/opt/local/include -I/usr/include -Wall -g -I. -I./ay
LINKFLAGS := $(FLAGS)
BINARY=barzer.exe
libs = -Lay -lay -L/opt/local/lib/boost -L/usr/lib -lboost_system -lexpat -lstdc++
ECHO = echo
objects = \
barzer_el_wildcard.o \
barzer_el_rewriter.o \
barzer_el_btnd.o \
barzer_el_parser.o \
barzer_el_trie.o \
barzer_el_trie_processor.o \
barzer_el_xml.o \
barzer_basic_types.o \
barzer_storage_types.o \
barzer_loader_xml.o \
barzer.o \
barzer_shell.o\
barzer_parse_types.o \
barzer_lexer.o \
barzer_parse.o \
barzer_language.o \
barzer_dtaindex.o \
barzer_server.o \
barzer_token.o \
lg_en/barzer_en_lex.o \
lg_ru/barzer_ru_lex.o \

all: ay/libay.a $(objects) 
	c++ $(LINKFLAGS) -o  $(BINARY) $(objects) $(libs) 
clean: 
	rm -f $(objects) $(BINARY)
cleanall: clean cleanaylib
	rm -f $(objects) $(BINARY)
cleanaylib: 
	cd ay; make -f aylib.mk clean; cd ..
aylib_rebuild: 
	cd ay; make -f aylib.mk rebuild FLAGS=$(FLAGS); cd ..
aylib: 
	cd ay; make -f aylib.mk FLAGS=$(FLAGS); cd ..
ay/libay.a: 
	cd ay; make -f aylib.mk rebuild $(FLAGS); cd ..
.cpp.o:
	c++  -c $(CFLAGS) $< -o $@
rebuild: clean aylib all
