TARGET=main
CC=gcc
CFLAGS=-Wall -Wextra

CFILES=$(shell (find -name '*.c'))
OFILES=$(subst .c,.o,$(CFILES))

$(TARGET): $(OFILES) sqlite.c mongoose.c
	$(CC) $(CFLAGS) -o $(TARGET) $(OFILES)

sqlite.c :
mongoose.c :
	sh pull_deps.sh

.c :
	$(shell echo '.c!')

.o : 
	$(shell echo '.c!')
	$(CC) $(CFLAGS) -c -o $@ $<

build: $(TARGET)
	./$(TARGET) build

serve: $(TARGET)
	./$(TARGET) serve

clean: 
	rm $(OFILES) $(TARGET)

full-clean: clean
	rm mongoose.c mongoose.h sqlite3.c sqlite3.h


