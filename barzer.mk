FLAGS := $(FLAGS) 
ifeq ($(IS64),yes)
	BITMODE=-m64
	AYBIT="IS64=yes"
endif 
ifeq ($(IS32),yes)
	BITMODE=-m32
	AYBIT="IS32=yes"
endif 
CFLAGS :=$(CFLAGS) $(BITMODE) $(OPT) -Wall -Wno-parentheses -Wnon-virtual-dtor \
	-I/opt/local/include -I/usr/include -g -I. -I./ay -fpic
LINKFLAGS := $(FLAGS) 
BINARY=barzer.exe
LIBNAME=libbarzer
SHARED_LIBNAME=libbarzer.so
PYTHON_LIBNAME=util/python_util.so
LIB_HUNSPELL=-lhunspell-1.2
#libs = -Lay -lay -L/opt/local/lib -L/opt/local/lib/boost -L/usr/lib $(LIB_HUNSPELL) 
libs = -Lay -lay -L/opt/local/lib -L/usr/lib $(LIB_HUNSPELL) \
	-lboost_system -lboost_filesystem -lboost_thread-mt -lexpat -lstdc++
ECHO = echo
lib_objects = \
barzer_el_function_util.o \
barzer_bzspell.o \
barzer_el_proc.o \
barzer_spell.o \
barzer_el_analysis.o \
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

objects = $(lib_objects) barzer.o

INSTALL_DIR = /usr/share/barzer
INSTALL_DATA_DIR = $(INSTALL_DIR)/data

all: ay/libay.a $(objects)
	$(CC) $(BITMODE) $(LINKFLAGS) -o  $(BINARY) $(objects) $(libs)
lib: ay/libay.a $(lib_objects)
	$(AR) -r $(LIBNAME).a $(lib_objects)
sharedlib: ay/libay.a $(lib_objects)
	$(CC) -shared -Wl,-soname,$(LIBNAME).so -o $(LIBNAME).so $(lib_objects) $(libs) 
$(PYTHON_LIBNAME): ay/libay.a $(lib_objects)
	cd util; make -f util.mk rebuild; cd ..
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
	$(CC) -DBARZER_HOME=$(INSTALL_DIR) -c $(CFLAGS) $< -o $@
rebuild: clean aylib util all

.PHONY : test util
test: $(BINARY)
	cd test; rake test
util:
	cd util; make -f util.mk rebuild; cd ..


install:
	install -d $(INSTALL_DIR) $(INSTALL_DIR)/util $(INSTALL_DATA_DIR)/configs $(INSTALL_DATA_DIR)/entities $(INSTALL_DATA_DIR)/rules
	install -m 0755 $(BINARY) $(INSTALL_DIR)
	install -m 0755 $(PYTHON_LIBNAME) $(INSTALL_DIR)/util
	install -m 0644 data/configs/* $(INSTALL_DATA_DIR)/configs
	install -m 0644 data/entities/* $(INSTALL_DATA_DIR)/entities
	install -m 0644 data/rules/* $(INSTALL_DATA_DIR)/rules
	
