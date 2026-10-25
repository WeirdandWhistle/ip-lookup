TARGET=main
CC=gcc
CFLAGS=-Wall -Wextra

CFILES=$(shell (find -name '*.c'))
OFILES=$(subst .c,.o,$(CFILES))

$(TARGET): $(OFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OFILES)

.c :
	$(shell echo '.c!')

.o : 
	$(shell echo '.c!')
	$(CC) $(CFLAGS) -c -o $@ $<

build: $(TARGET)
	./$(TARGET) build

clean: 
	rm $(OFILES) $(TARGET)

