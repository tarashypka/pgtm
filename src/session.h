#ifndef SESSION_H
#define SESSION_H

#include <libpq-fe.h>   /* PGconn, PGresult */

typedef enum { false, true } bool;

struct PGsession {
    const char *host;
    const char *port;
    const char *db_name;
    const char *login;
    const char *pwd;
    PGconn *conn;
    char logs[1000];
};

void  open(struct PGsession *ses);
void close(struct PGsession *ses);
PGresult *exec(struct PGsession *ses, const char *que);
const char *gettime(void);

#endif
