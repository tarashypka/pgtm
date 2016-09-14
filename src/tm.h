#ifndef TM_H
#define TM_H

#include "session.h"    /* struct PGsession */

void tm_init  (struct PGsession *ses);
void tm_commit(struct PGsession *ses);

#endif
