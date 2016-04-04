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
};

void  open(struct PGsession *ses);
void close(struct PGsession *ses);
PGresult *exec(struct PGsession *ses, const char *que);

#endif
