#ifndef TM_H
#define TM_H

#include "session.h"    /* struct PGsession */

void tm_init  (PGsession *ses);
void tm_commit(PGsession *ses);

#endif
