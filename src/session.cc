#include "session.h"    /* struct PGsession, logs */
#include "libpq-fe.h"   /* PQfunctions, PGtypes */
#include <stdio.h>      /* NULL, fprintf, stderr, printf, fopen, fclose */
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <string.h>     /* strcmp */
#include "tm.h"         /* tm_init, tm_commit */
#include <time.h>       /* time_t, time, strftime */

/* should be 0, if transaction is non-distributed */
#define TM          1

static FILE *logs;

/* open: set PGsession connection */
void open(PGsession *ses)
{
    ses->conn = PQsetdbLogin(ses->host,
                             ses->port,
                             NULL,          /* default pgoptions */
                             NULL,          /* default pgtty */
                             ses->db_name,
                             ses->login,
                             ses->pwd);
    strcat(ses->logs, ses->db_name);
    strcat(ses->logs, ".log");
    logs = fopen(ses->logs, "a+");
    fprintf(logs, "%s: ", gettime());
    if (PQstatus(ses->conn) != CONNECTION_OK) {
        fprintf(logs, "open: error: connection failed: %s",
                PQerrorMessage(ses->conn));
        fclose(logs);
        PQfinish(ses->conn);
        exit(EXIT_FAILURE);
    }
    fprintf(logs, "open: connected\n");
    fclose(logs);
}

/* close: finish PGsession connection & free memory from it */
void close(PGsession *ses)
{
    PQfinish(ses->conn);
    logs = fopen(ses->logs, "a+");
    fprintf(logs, "%s: ", gettime());
    fprintf(logs, "close: disconnected\n");
    fclose(logs);
}

/* exec: execute pgSQL command, print & return result */
PGresult *exec(PGsession *ses, const char *que)
{
    PGresult *res;

    if (!(strcmp(que, "BEGIN")) && TM)
        tm_init(ses);
    else if (!(strcmp(que, "COMMIT")) && TM) {
        tm_commit(ses);
        return NULL;
    }
    res = PQexec(ses->conn, que);
    logs = fopen(ses->logs, "a+");
    fprintf(logs, "%s: ", gettime());
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        fprintf(logs, "exec(%s): %s", 
                que, PQerrorMessage(ses->conn));
    else
        fprintf(logs, "exec(%s): %s\n", que, PQcmdStatus(res));
    fclose(logs);
    return res;
}

const char *gettime(void)
{
    static char local_t[1000];
    const time_t curr_t = time(NULL);

    strftime(local_t, 100, "%c", localtime(&curr_t));
    return local_t;
}
