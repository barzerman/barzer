CFLAGS := -I/opt/local/include -Wall -g -I. -I./ay -j4
LINKFLAGS := 
BINARY=barzer.exe
libs = ay/aylib.a /opt/local/lib/boost/libboost_system.a -lexpat
ECHO = echo
objects = \
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
lg_ru/barzer_ru_lex.o

all: ay/aylib.a $(objects) 
	c++ $(LINKFLAGS) -o  $(BINARY) $(libs) $(objects)
clean: 
	rm -f $(objects) $(BINARY)
cleanall: clean cleanaylib
	rm -f $(objects) $(BINARY)
cleanaylib: 
	cd ay; make -f aylib.mk clean; cd ..
aylib: 
	cd ay; make -f aylib.mk rebuild; cd ..
ay/aylib.a: 
	cd ay; make -f aylib.mk rebuild; cd ..
.cpp.o:
	c++  -c $(CFLAGS) $< -o $@
rebuild: clean aylib all
