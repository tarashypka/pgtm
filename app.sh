#! /bin/zsh

PG_H_FILES=$(pg_config --includedir)
   PG_LIBS=$(pg_config --libdir)

gcc -ansi ./src/{app.c,session.c,tm.c} \
    -o ./bin/app.o \
    -I$PG_H_FILES \
    -L$PG_LIBS \
    -lpq
./bin/app.o $*
