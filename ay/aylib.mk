CFLAGS := -I.
LIBNAME=aylib.a

objects = ay_cmdproc.o ay_shell.cpp

all: $(objects)
	$(AR) -r  $(LIBNAME) $(objects)
clean: 
	rm -f $(objects) $(LIBNAME)

.cpp.o:
	c++ -c $(CFLAGS) $< -o $@
