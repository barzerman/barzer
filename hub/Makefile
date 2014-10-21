STD=-std=c++11

lib_objects = \
	bh_expr.o\
	glue.o

go_files = \
	main.go\
	json.go\
	xml.go\
	types.go\
	c.go

BINARY=barzer_hub
LIBNAME=libbh_expr.a
TEST_BINARY=test

CC=g++
CFLAGS=
LDFLAGS=-L. -lstdc++ -lm -lboost_system

.PHONY: clean test

all: $(LIBNAME)
	go build -o $(BINARY) $(go_files)

test: $(LIBNAME) test.o
	$(CC) -o $(TEST_BINARY) $(LDFLAGS) test.o -lbh_expr

$(LIBNAME): $(lib_objects) 
	$(AR) -r $(LIBNAME) $(lib_objects) 
	
.cpp.o:
	$(CC) -g $(STD) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(LIBNAME) $(BINARY) $(TEST_BINARY)

