#include "session.h"    /* PGsession */
#include <string.h>     /* strcmp, strcpy, strcat */
#include <stdio.h>      /* printf */
#include "libpq-fe.h"   /* PGconn, PGresult, PQresultstate, PQclear, 
                           PQcmdStatus */

#define MAX_COHORTS     5

/* cohort/coordinator states */
#define INIT        0
#define QUERY       1
#define WAIT        2
#define ABORT       3
#define COMMIT      4
#define ABORTED     5
#define COMMITTED   6

static int state = INIT;   /* coordinator state */

struct cohort {
    PGsession *ses;
    int state;
} cohorts[MAX_COHORTS], *cnext = cohorts;

struct cohort *get_cohort(const char *name) {
    struct cohort *c;

    for (c = cnext; c > cohorts; c--)
        if (!strcmp((c-1)->ses->db_name, name))
            return c-1;
    return NULL;
}

void tm_init(PGsession *ses) {
    if (get_cohort(ses->db_name) != NULL)
        printf("%8s: tm_init: error: duplicate cohort\n", ses->db_name);
    else if (cnext >= cohorts + MAX_COHORTS)
        printf("%8s: tm_init: error: only %d cohorts allowed\n", 
                ses->db_name, MAX_COHORTS);
    else {
        struct cohort c = { ses, INIT };
        *cnext++ = c;
    }
}

void tm_commit(PGsession *ses) {
    int commit_request_all(void), commit_all(void), abort_all(void);
    struct cohort *c;

    get_cohort(ses->db_name)->state = QUERY;
    for (c = cnext; c > cohorts; c--)
        if ((c-1)->state == INIT) {
            printf("%8s: tm_commit: cohorts not ready \n", ses->db_name);
            return;
        }
    state = QUERY;
    switch (commit_request_all()) {
        case QUERY:
            printf("%8s: tm_commit: prepare error\n", ses->db_name);
            break;
        case WAIT:
            printf("%8s: tm_commit: commit all\n", ses->db_name);
            state = COMMIT;
            commit_all();
            break;
        case ABORT:
            printf("%8s: tm_commit: abort all\n", ses->db_name);
            abort_all();
            break;
        default:
            break;
    }
}

int commit_request_all(void) {
    int commit_request(struct cohort *c);
    struct cohort *c;

    for (c=cnext; c>cohorts && (state=commit_request(c-1)) == WAIT; c--)
        ;
    return state;
}

void commit_all(void) {
    int commit(struct cohort *c);
    struct cohort *c;

    for (c = cnext; c > cohorts; c--)
        if (commit(c-1) != COMMIT)
            return;
    state = COMMITTED;
    cnext = cohorts;
}

void abort_all(void) {
    int abort(struct cohort *c);
    struct cohort *c;

    for (c = cnext; c > cohorts; c--)
        if (abort(c-1) != ABORT) 
            return;
    state = ABORTED;
    cnext = cohorts;
}

int commit_request(struct cohort *c) {
    char *status;
    char que[100];

    strcpy(que, "PREPARE TRANSACTION '");
    strcat(que, c->ses->db_name);
    strcat(que, "'");
    status = PQcmdStatus(exec(c->ses, que));
    if (!strcmp(status, "PREPARE TRANSACTION"))
        return c->state = WAIT;
    return c->state = (!strcmp(status, "ROLLBACK")) ? ABORT : QUERY;
}

int commit(struct cohort *c) {
    char *status;
    char que[100];

    strcpy(que, "COMMIT PREPARED '");
    strcat(que, c->ses->db_name);
    strcat(que, "'");
    status = PQcmdStatus(exec(c->ses, que));
    return c->state = (!strcmp(status, "COMMIT PREPARED")) ? COMMIT : WAIT;
}

int abort(struct cohort *c) {
    char *status;
    char que[100];

    if (c->state == ABORT)  /* already aborted */
        return ABORT;
    else if (c->state == QUERY) {   /* commit request not sent */
        status = PQcmdStatus(exec(c->ses, "ROLLBACK"));
        return c->state = (!strcmp(status, "ROLLBACK")) ? ABORT : WAIT;
    }
    strcpy(que, "ROLLBACK PREPARED '");
    strcat(que, c->ses->db_name);
    strcat(que, "'");
    status = PQcmdStatus(exec(c->ses, que));
    return c->state = (!strcmp(status, "ROLLBACK PREPARED")) ? ABORT : WAIT;
}
