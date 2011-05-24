FLAGS := $(FLAGS) 
ifeq ($(IS64),yes)
	BITMODE=-m64
	AYBIT="IS64=yes"
endif 
ifeq ($(IS32),yes)
	BITMODE=-m32
	AYBIT="IS32=yes"
endif 
CFLAGS :=$(CFLAGS) $(BITMODE) $(OPT) -Wno-parentheses -Wnon-virtual-dtor -I/opt/local/include -I/usr/include -Wall -g -I. -I./ay
LINKFLAGS := $(FLAGS)
BINARY=barzer.exe
libs = -Lay -lay -L/opt/local/lib -L/opt/local/lib/boost -L/usr/lib -lboost_system -lexpat -lstdc++
ECHO = echo
objects = \
barzer_server_response.o \
barzer_server_request.o \
barzer_barz.o \
barzer_el_matcher.o \
barzer_el_chain.o \
barzer_universe.o \
barzer_el_wildcard.o \
barzer_el_rewriter.o \
barzer_el_btnd.o \
barzer_el_parser.o \
barzer_el_trie.o \
barzer_el_trie_processor.o \
barzer_el_trie_shell.o \
barzer_el_trie_walker.o \
barzer_el_xml.o \
barzer_el_function.o \
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
barzer_date_util.o \
barzer_datelib.o \
barzer_settings.o \
lg_en/barzer_en_lex.o \
lg_ru/barzer_ru_lex.o \
lg_en/barzer_en_date_util.o \
lg_ru/barzer_ru_date_util.o \

DIST_FILES = \
	$(BINARY) \
	config.xml \
	barzel_rules.xml
DIST_DATA_FILES = \
	data/barzel_prices.xml \
	data/barzel_dates.xml \
	data/barzel_fluff.xml 

INSTALL_DIR = /usr/share/barzer
INSTALL_DATA_DIR = $(INSTALL_DIR)/data

all: ay/libay.a $(objects) 
	$(CC) $(BITMODE) $(LINKFLAGS) -o  $(BINARY) $(objects) $(libs)
clean: 
	rm -f $(objects) $(BINARY)
cleanall: clean cleanaylib
	rm -f $(objects) $(BINARY)
cleanaylib: 
	cd ay; make -f aylib.mk clean; cd ..
aylib_rebuild: 
	cd ay; make -f aylib.mk rebuild $(AYBIT) OPT=$(OPT) FLAGS=$(FLAGS); cd ..
aylib: 
	cd ay; make -f aylib.mk $(AYBIT) OPT=$(OPT) FLAGS=$(FLAGS); cd ..
ay/libay.a: 
	cd ay; make -f aylib.mk rebuild $(AYBIT) OPT=$(OPT) $(FLAGS); cd ..
.cpp.o:
	$(CC)  -c $(CFLAGS) $< -o $@
rebuild: clean aylib all

.PHONY : test
test: all
	cd test; rake test

install:
	install -d $(INSTALL_DIR) $(INSTALL_DATA_DIR)
	install $(DIST_FILES) $(INSTALL_DIR)
	install $(DIST_DATA_FILES) $(INSTALL_DATA_DIR)
