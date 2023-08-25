CC=gcc
CFLAGS=-g -I$(IDIR) -Wall -Wextra

IDIR=include
ODIR=obj
SRCDIR=src

LIBS=-lmicrohttpd

# Headers dentro de include/
_DEPS = parse.h connection_handler.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# Object files (.o) que haya que compilar 
_OBJ = main.o parse.o connection_handler.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

server.exe: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o core *~ $(INCDIR)/*~ 
	rmdir $(ODIR)
