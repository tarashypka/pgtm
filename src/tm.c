#include "tm.h"         /* struct PGsession, gettime, tm_init, tm_commit */
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <stdio.h>      /* NULL, printf, fopen, fclose */
#include <libpq-fe.h>   /* PQfunctions, PGtypes */
#include <string.h>     /* strcmp, strcat, strcpy */

#define MAX_COHORTS     5

/* cohort's possible states */
#define INIT        0
#define QUERY       1
#define WAIT        2
#define ABORT       3
#define COMMIT      4
#define ABORTED     5
#define COMMITTED   6

const char *states[10] = 
{ "INIT", "QUERY", "WAIT", "ABORT", "COMMIT", "ABORTED", "COMMITTED" };

static struct cohort {
    struct PGsession *ses;
    int state;
} cohorts[MAX_COHORTS], *cnext = cohorts, coordinator = { NULL, INIT };

static FILE *logs;

static const char *clogs = 
"/home/deoxys37/Workspace/C/tm/logs/coordinator.logs";

static struct cohort *get_cohort(const char *name)
{
    struct cohort *c;
    for (c = cnext; c > cohorts; c--)
        if (!strcmp((c-1)->ses->db_name, name))
            return c-1;
    return NULL;
}

static void set_state(struct cohort *c, int state)
{
    logs = fopen((c->ses) ? c->ses->logs : clogs, "a+"); 
    fprintf(logs, "%s: ", gettime());
    fprintf(logs, "restate %s --> %s\n", states[c->state], states[state]);
    fclose(logs);
    c->state = state;
}

void tm_init(struct PGsession *ses)
{
    logs = fopen(ses->logs, "a+");
    if (get_cohort(ses->db_name) != NULL)
        fprintf(logs, "tm_init: error: duplicate cohort\n");
    else if (cnext >= cohorts + MAX_COHORTS)
        fprintf(logs, "tm_init: error: only %d cohorts allowed\n", 
                MAX_COHORTS);
    else {
        struct cohort c = { ses, INIT };
        *cnext++ = c;
    }
    fclose(logs);
}

void tm_commit(struct PGsession *ses)
{
    FILE *logs = fopen(ses->logs, "a+");
    void commit_request(struct cohort *);
    bool commit(struct cohort *), abortr(struct cohort *);
    struct cohort *c;

    set_state(get_cohort(ses->db_name), QUERY);
    fprintf(logs, "%s: ", gettime());
    for (c = cnext; c > cohorts; c--)
        if ((c-1)->state == INIT) {
            fprintf(logs, "cohorts not ready \n");
            fclose(logs);
            return;
        }
    set_state(&coordinator, QUERY);
    commit_request(cnext-1);
    switch (coordinator.state) {
        case QUERY:
            fprintf(logs, "tm_commit: prepare error\n");
            break;
        case WAIT:
            set_state(&coordinator, COMMIT);
            fprintf(logs, "tm_commit: commit all\n");
            if (commit(cnext-1))
                set_state(&coordinator, COMMITTED);
            break;
        case ABORT:
            fprintf(logs, "tm_commit: abort all\n");
            if (abortr(cnext-1))
                set_state(&coordinator, ABORTED);
            break;
        default:
            break;
    }
    fclose(logs);
}

void commit_request(struct cohort *c)
{
    char *status, que[100] = "PREPARE TRANSACTION '";

    strcat(que, c->ses->db_name);
    strcat(que, "'");
    status = PQcmdStatus(exec(c->ses, que));
    if (!strcmp(status, "PREPARE TRANSACTION")) {
        set_state(c, WAIT);
        if (c > cohorts)
            commit_request(c-1);
        else
            set_state(&coordinator, WAIT);
    }
    else if (!strcmp(status, "ROLLBACK")) {
        set_state(c, ABORT);
        set_state(&coordinator, ABORT);
    }
    else
        set_state(&coordinator, QUERY);
}

bool commit(struct cohort *c)
{
    char *status, que[100] = "COMMIT PREPARED '";

    strcat(que, c->ses->db_name);
    strcat(que, "'");
    status = PQcmdStatus(exec(c->ses, que));
    set_state(c, !strcmp(status, "COMMIT PREPARED") ? COMMIT : WAIT);
    return (c->state == COMMIT) && (c == cohorts || commit(c-1));
}

bool abortr(struct cohort *c)
{
    char *status, que[100];

    if (c->state == QUERY) {    /* commit request not sent */
        status = PQcmdStatus(exec(c->ses, "ROLLBACK"));
        set_state(c, !strcmp(status, "ROLLBACK") ? ABORT : QUERY);
    }
    if (c->state != WAIT)       /* already aborted */
        return c->state == ABORT;
    strcpy(que, "ROLLBACK PREPARED '");
    strcat(que, c->ses->db_name);
    strcat(que, "'");
    status = PQcmdStatus(exec(c->ses, que));
    set_state(c, !strcmp(status, "ROLLBACK PREPARED") ? ABORT : WAIT);
    return (c->state == ABORT) && (c == cohorts || abortr(c-1));
}
