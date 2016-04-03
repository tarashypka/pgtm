#ifndef TM
#define TM

#include "session.h"    /* PGsession */

void tm_init  (PGsession *ses);
void tm_commit(PGsession *ses);

#endif
