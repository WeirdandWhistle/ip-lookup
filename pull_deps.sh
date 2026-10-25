#!/bin/bash
curl -Z -OLS https://raw.githubusercontent.com/cesanta/mongoose/refs/heads/master/mongoose.c \
    -OLS https://raw.githubusercontent.com/cesanta/mongoose/refs/heads/master/mongoose.h \ 
    -OLS https://sqlite.org/2026/sqlite-amalgamation-3540100.zip \

unzip sqlite-amalgamation-3540100.zip
mv sqlite-amalgamation-3540100/sqlite.c ./sqlite.c
mv sqlite-amalgamation-3540100/sqlite.h ./sqlite.h
rm -r sqlite-amalgamation-3540100.zip sqlite-amalgamation-3540100