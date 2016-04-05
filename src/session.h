#ifndef SESSION_H
#define SESSION_H

#include "libpq-fe.h"   /* PGconn, PGresult */

struct PGsession {
    const char *host;
    const char *port;
    const char *db_name;
    const char *login;
    const char *pwd;
    PGconn *conn;
    char logs[1000];
};

void  open(PGsession *ses);
void close(PGsession *ses);
PGresult *exec(PGsession *ses, const char *que);
const char *gettime(void);

#endif
