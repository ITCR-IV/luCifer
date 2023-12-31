CC=gcc
CFLAGS=-g -I$(IDIR) -Wall -Wextra

IDIR=include
ODIR=obj
SRCDIR=src

LIBS=-lmicrohttpd -lfreeimage -lm

# Headers dentro de include/
_DEPS = parse.h connection_handler.h process_image.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# Object files (.o) que haya que compilar 
_OBJ = main.o parse.o connection_handler.o process_image.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

server.exe: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean install

clean:
	rm -f $(ODIR)/*.o core *~ $(INCDIR)/*~ 
	rmdir $(ODIR)

install: server.exe
	sudo cp server.exe /usr/bin/ImageServer
	sudo cp ImageServer.service /usr/lib/systemd/system/
	sudo mkdir -p /etc/ImageServer
	sudo cp default_config.conf /etc/ImageServer/config.conf
