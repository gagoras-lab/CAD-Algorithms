IDIR = include
SDIR = src
CC = gcc
CFLAGS = -I$(IDIR)

# Object files in hidden directory
ODIR = src/.obj
LDIR = lib

# Needed library flags
LIBS = -lcudd -lutil -lm -ggdb3 -ltcl -lreadline

# Dependancies
_DEPS = functions.h structs.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# Objects
_OBJ = gnu_tcl_shell.o tcl_functions.o circuit_parser.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# Make object files
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

tcl_circuit_parser: $(OBJ)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LIBS)

# Clean object files
.PHONY: clean
clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

# Run executable
.PHONY: run
run:
	./tcl_circuit_parser

# Run executable (using valgrind)
.PHONY: run_valgrind
run_valgrind:
	valgrind --leak-check=full ./tcl_circuit_parser