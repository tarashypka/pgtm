#include "session.h"    /* struct PGsession */
#include <stdio.h>      /* NULL, printf */
#include "libpq-fe.h"   /* PQfunctions, PGtypes */
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

static struct cohort {
    struct PGsession *ses;
    int state;
} cohorts[MAX_COHORTS], *cnext = cohorts, coordinator = { NULL, INIT };

struct cohort *get_cohort(const char *name)
{
    struct cohort *c;

    for (c = cnext; c > cohorts; c--)
        if (!strcmp((c-1)->ses->db_name, name))
            return c-1;
    return NULL;
}

void set_state(struct cohort *c, int state)
{
    c->state = state;
    /* write to log */
}

void tm_init(struct PGsession *ses)
{
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

void tm_commit(PGsession *ses)
{
    void commit_request(struct cohort *);
    bool commit(struct cohort *), abort(struct cohort *);
    struct cohort *c;

    set_state(get_cohort(ses->db_name), QUERY);
    for (c = cnext; c > cohorts; c--)
        if ((c-1)->state == INIT) {
            printf("%8s: tm_commit: cohorts not ready \n", ses->db_name);
            return;
        }
    set_state(&coordinator, QUERY);
    commit_request(cnext-1);
    switch (coordinator.state) {
        case QUERY:
            printf("%8s: tm_commit: prepare error\n", ses->db_name);
            break;
        case WAIT:
            set_state(&coordinator, COMMIT);
            printf("%8s: tm_commit: commit all\n", ses->db_name);
            if (commit(cnext-1))
                set_state(&coordinator, COMMITTED);
            break;
        case ABORT:
            printf("%8s: tm_commit: abort all\n", ses->db_name);
            if (abort(cnext-1))
                set_state(&coordinator, ABORTED);
            break;
        default:
            break;
    }
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

bool abort(struct cohort *c)
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
    return (c->state == ABORT) && (c == cohorts || abort(c-1));
}
