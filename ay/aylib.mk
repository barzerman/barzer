FLAGS := $(FLAGS) 
CFLAGS := $(FLAGS) -g -I.
LIBNAME=libay.a

objects = ay_cmdproc.o ay_shell.o ay_util.o ay_string_pool.o ay_util_time.o

all: $(objects)
	$(AR) -r  $(LIBNAME) $(objects)
clean: 
	rm -f $(objects) $(LIBNAME)

.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $@
rebuild: clean all
