TARGET=main
CC=gcc
CFLAGS=-Wall -Wextra

CFILES=$(shell (find -name '*.c'))
OFILES=$(subst .c,.o,$(CFILES))

$(TARGET): sqlite3.c mongoose.c $(OFILES) 
	$(CC) $(CFLAGS) -o $(TARGET) $(OFILES)

sqlite3.c : pull-deps
	$(CC) $(CFLAGS) -c -o sqlite3.o sqlite3.c
mongoose.c : pull-deps
	$(CC) $(CFLAGS) -c -o mongoose.o mongoose.c

.c :
	$(shell echo '.c!')

.o : 
	$(shell echo '.o!')
	$(CC) $(CFLAGS) -c -o $@ $<

pull-deps:
	sh pull_deps.sh

build: $(TARGET)
	./$(TARGET) build

serve: $(TARGET)
	./$(TARGET) serve

clean: 
	rm -f $(OFILES) $(TARGET)

full-clean: clean
	rm mongoose.c mongoose.h sqlite3.c sqlite3.h


