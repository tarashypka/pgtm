#include <stdio.h>      /* printf, fprintf, stderr */
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <string.h>     /* strcmp */
#include "session.h"    /* struct PGsession */
#include "tm.h"         /* tm_init, tm_commit */
#include "libpq-fe.h"   /* PQfunctions, PGtypes, 
                           CONNECTION_OK, PGRES_COMMAND_OK */

/* set 0 if for non-distributed transactions */
#define TM_     1

/* open: set PGsession connection */
void open(struct PGsession *ses) {
    ses->conn = PQsetdbLogin(ses->host,
                             ses->port,
                             NULL,          /* default pgoptions */
                             NULL,          /* default pgtty */
                             ses->db_name,
                             ses->login,
                             ses->pwd);
    if (PQstatus(ses->conn) != CONNECTION_OK) {
        fprintf(stderr, "%8s: open: error: connection failed: %s",
                ses->db_name, PQerrorMessage(ses->conn));
        PQfinish(ses->conn);
        exit(EXIT_FAILURE);
    }
    printf("%8s: open: connected\n", ses->db_name);
}

/* close: finish PGsession connection & free memory from it */
void close(struct PGsession *ses) {
    PQfinish(ses->conn);
    printf("%8s: close: disconnected\n", ses->db_name);
}

/* exec: execute pgSQL command, print & return result */
PGresult *exec(struct PGsession *ses, const char *que) {
    PGresult *res;

    if (!(strcmp(que, "BEGIN")) && TM_)
        tm_init(ses);
    else if (!(strcmp(que, "COMMIT")) && TM_) {
        tm_commit(ses);
        return NULL;
    }
    res = PQexec(ses->conn, que);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        fprintf(stderr, "%8s: exec(%s): %s", 
                ses->db_name, que, PQerrorMessage(ses->conn));
    else
        printf("%8s: exec(%s): success\n", ses->db_name, que);
    return res;
}
