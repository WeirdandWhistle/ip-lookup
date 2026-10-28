#!/bin/bash

if [ -f "sqlite3.c" ]; then
    echo "DEPS proably exist. Exiting..."
    exit 0
fi

curl -Z -OLS https://raw.githubusercontent.com/cesanta/mongoose/refs/heads/master/mongoose.c \
    -OLS https://raw.githubusercontent.com/cesanta/mongoose/refs/heads/master/mongoose.h \
    -OLS https://sqlite.org/2026/sqlite-amalgamation-3540100.zip

unzip sqlite-amalgamation-3540100.zip
mv sqlite-amalgamation-3540100/sqlite3.c ./sqlite3.c
mv sqlite-amalgamation-3540100/sqlite3.h ./sqlite3.h
rm -r sqlite-amalgamation-3540100.zip sqlite-amalgamation-3540100