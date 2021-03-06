PYINCLUDE := $(shell /usr/bin/python2.7-config --includes)
PYLIBS := $(shell /usr/bin/python2.7-config --libs)
UNAME := $(shell uname -a | cut -d' ' -f1)
FLAGS := $(FLAGS)
BOOST_SYSLIB=-lboost_system -lboost_filesystem -lboost_regex-mt
BOOST_THREADLIB=-lboost_thread
ifeq ($(IS64),yes)
	BITMODE=-m64
	AYBIT="IS64=yes"
endif
ifeq ($(UNAME),Darwin)
    BOOST_SYSLIB=/opt/local/lib/libboost_system-mt.dylib /opt/local/lib/libboost_filesystem-mt.dylib /opt/local/lib/libboost_regex-mt.dylib
    BOOST_LIB=boost_python-mt
    BOOST_THREADLIB=-lboost_thread-mt
    C11LIB_SHIT=-stdlib=libc++
else
    BOOST_LIB=boost_python
endif
ifeq ($(IS32),yes)
	BITMODE=-m32
	AYBIT="IS32=yes"
endif
#ifeq ($(CX),yes)
#    C11=-std=c++0x
#endif
C11=-std=c++11
ifeq ($(CC),clang++)
    CLANG_WARNSUPPRESS=-Wno-array-bounds
endif
WARNSUPPRESS=-Wno-parentheses -Wnon-virtual-dtor $(CLANG_WARNSUPPRESS)
CFLAGS :=$(CFLAGS) $(BITMODE) $(OPT) $(WARNSUPPRESS) $(C11) $(C11LIB) \
	-I/opt/local/include -I/usr/include -g -I. -I./ay -I./lg_ru -fpic $(PYINCLUDE)
LINKFLAGS := $(FLAGS)
BINARY=barzer.exe
LIBNAME=libbarzer
SHARED_LIBNAME=libbarzer.so
PYTHON_LIBNAME=util/python_util.so
#libs = -Lay -lay -L/opt/local/lib -L/opt/local/lib/boost -L/usr/lib 
libs = -Lay -Lsnowball -lay -lsnowlib -L/usr/local/lib -L/opt/local/lib -L/usr/lib \
	$(BOOST_SYSLIB) $(BOOST_THREADLIB) -lexpat -lstdc++
ECHO = echo
lib_objects = \
batch/barzer_batch_processor.o \
function/barzer_el_function_topic.o \
function/barzer_el_function_date.o \
barzer_el_function_holder.o \
barzer_shell_01.o \
barzer_question_parm.o \
barzer_server_request_filter.o \
barzer_beni.o \
barzer_basic_types_range.o \
barzer_el_pattern_range.o \
barzer_el_rewrite_types.o \
barzer_el_pattern.o \
barzer_el_pattern_token.o \
barzer_el_pattern_datetime.o \
barzer_el_pattern_entity.o \
barzer_el_pattern_number.o \
barzer_el_pattern_entity.o \
mongoose/mongoose.o \
barzer_universe.o \
barzer_http.o \
barzer_global_pools.o \
barzer_json_output.o \
barzer_el_cast.o \
barzer_meaning.o \
barzer_tokenizer.o \
barzer_barz.o \
barzer_entity.o \
barzer_basic_types.o \
barzer_datelib.o \
barzer_date_util.o \
barzer_dtaindex.o \
barzer_el_analysis.o \
barzer_el_btnd.o \
barzer_el_chain.o \
barzer_el_function.o \
barzer_el_function_util.o \
barzer_el_matcher.o \
barzer_el_parser.o \
barzer_el_rewriter.o \
barzer_el_trie.o \
barzer_el_trie_processor.o \
barzer_el_trie_shell.o \
barzer_el_trie_walker.o \
barzer_el_wildcard.o \
barzer_el_xml.o \
barzer_el_proc.o \
barzer_language.o \
barzer_lexer.o \
barzer_loader_xml.o \
barzer_parse.o \
barzer_parse_types.o \
barzer_server.o \
barzer_server_request.o \
barzer_server_response.o \
barzer_settings.o \
barzer_shell.o \
barzer_storage_types.o \
barzer_token.o \
barzer_bzspell.o \
barzer_autocomplete.o \
barzer_emitter.o \
barzer_locale.o \
barzer_number.o \
barzer_barzxml.o \
barzer_el_rewrite_control.o \
barzer_relbits.o \
barzer_spellheuristics.o \
barzer_geoindex.o \
barzer_spell_features.o \
barzer_el_trie_ruleidx.o \
zurch/zurch_loader_longxml.o \
zurch_route.o \
zurch_barzer.o \
zurch_server.o \
zurch_settings.o \
zurch_classifier.o \
zurch_docidx.o \
zurch_phrasebreaker.o \
zurch_tokenizer.o \
zurch_docdataindex.o \
autotester/barzer_at_autotester.o \
autotester/barzer_at_comparators.o \
lg_en/barzer_en_date_util.o \
lg_en/barzer_en_lex.o \
lg_ru/barzer_ru_date_util.o \
lg_ru/barzer_ru_lex.o \
lg_ru/barzer_ru_stemmer.o

objects = $(lib_objects) barzer.o
objects_python=zurch_python.o barzer_python.o util/pybarzer.o

INSTALL_DIR = /usr/share/barzer
INSTALL_DATA_DIR = $(INSTALL_DIR)/data
BARZEREXE_OBJ=barzer.o

all: snowball/libsnowlib.a ay/libay.a $(LIBNAME).a $(BARZEREXE_OBJ)
	$(CC) $(C11LIB) $(BITMODE) $(LINKFLAGS) -o  $(BINARY) $(BARZEREXE_OBJ) $(LIBNAME).a $(libs)
lib: ay/libay.a $(LIBNAME).a $(lib_bjects)
	$(AR) -r $(LIBNAME).a $(lib_objects) ay/libay.a
sharedlib: ay/libay.a $(lib_objects)
	$(CC) $(C11LIB) -shared -dylib -o $(LIBNAME).so $(lib_objects) $(libs)
pybarzer: $(objects_python) $(LIBNAME).a
	$(CC) $(C11LIB) -shared -o pybarzer.so -l$(BOOST_LIB) $(objects_python) $(LIBNAME).a $(libs)  -l$(BOOST_LIB) $(PYLIBS) 
barzer_python.o: barzer_python.cpp
    $(CC) -DBARZER_HOME=$(INSTALL_DIR) -c $(CFLAGS) $< -o $@
$(LIBNAME).a: ay/libay.a $(lib_objects)
	$(AR) -r $(LIBNAME).a $(lib_objects) ay/libay.a
$(PYTHON_LIBNAME): ay/libay.a $(lib_objects)
	cd util; make -f util.mk rebuild; cd ..
clean:
	rm -f $(objects) $(objects_python) $(BINARY) $(LIBNAME).a
cleanall: clean cleanaylib
	rm -f $(objects) $(BINARY) $(objects_python)
cleanaylib:
	rm -f ay/*.o ay/*.a
aylib_rebuild:
	cd ay; make -f aylib.mk rebuild $(AYBIT) OPT=$(OPT) C11=$(C11) FLAGS=$(FLAGS); cd ..
aylib:
	cd ay; make -f aylib.mk $(AYBIT) OPT=$(OPT) C11=$(C11) FLAGS=$(FLAGS); cd ..
ay/libay.a:
	cd ay; make -f aylib.mk rebuild $(AYBIT) OPT=$(OPT) C11=$(C11) $(FLAGS); cd ..
snowlib:
	cd snowball; make; cd ..
snowball/libsnowlib.a:
	cd snowball; make; cd ..
.cpp.o:
	$(CC) $(C11LIB) -DBARZER_HOME=$(INSTALL_DIR) -c $(CFLAGS) $< -o $@
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

