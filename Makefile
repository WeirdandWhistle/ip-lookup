TARGET=main
CC=gcc
CFLAGS=-Wall -Wextra -O3
SRC=src

CFILES=$(shell (find -name '*.c'))
OFILES=$(subst .c,.o,$(CFILES))

$(TARGET): $(SRC)/sqlite3.o $(SRC)/mongoose.o $(OFILES) 
	$(CC) $(CFLAGS) -o $(TARGET) $(OFILES)


# sqlite3.o : pull-depss
# 	$(CC) $(CFLAGS) -c -o sqlite3.o sqlite3.c
# mongoose.o : pull-deps
# 	$(CC) $(CFLAGS) -c -o mongoose.o mongoose.c

$(SRC)/sqlite3.c : pull-deps
$(SRC)/mongoose.c : pull-deps

.c :
	$(shell (echo '.c!'))

.o : .c
	$(shell (echo '.o!'))
	$(CC) $(CFLAGS) -c -o $@ $<

pull-deps:
	cd $(SRC) && sh pull_deps.sh

build: $(TARGET)
	./$(TARGET) build

serve: $(TARGET)
	./$(TARGET) serve

clean: 
	rm -f $(OFILES) $(TARGET)

full-clean: clean
	rm $(SRC)/mongoose.c $(SRC)/mongoose.h $(SRC)/sqlite3.c $(SRC)/sqlite3.h


