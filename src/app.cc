#include "session.h"    /* struct PGsession, open, exec, close */
#include <stdio.h>      /* NULL */
#include "libpq-fe.h"   /* PGresult, PQclear */

#define HOST    "localhost"
#define PORT    "5432"
#define USER    "pti"
#define PWD     "12345678"

int main(int argc, char **argv)
{
    struct PGsession
    ses_1 = { HOST, PORT,   "fly_db", USER, PWD, NULL },
    ses_2 = { HOST, PORT, "hotel_db", USER, PWD, NULL };
    open(&ses_1);
    open(&ses_2);

    PGresult *res;
    res = exec(&ses_2, "BEGIN");
    PQclear(res);
    res = exec(&ses_1, "BEGIN");
    PQclear(res);

    res = exec(&ses_1, "INSERT INTO fly_booking " 
            "(id_, client_name_, flight_number_, from_, to_, date_) VALUES "
            "(2, 'Kelmer', 'UA-1021', 'Kyiv', 'Costa Rica', '05-02-16')");
    PQclear(res);
    res = exec(&ses_1, "COMMIT");
    PQclear(res);

    /* client_name_ has UNIQUE CONSTRAINT */
    res = exec(&ses_2, "INSERT INTO hotel_booking " 
            "(id_, client_name_, hotel_name_, arrival_, departure_) VALUES "
            "(2, 'Kelmer', 'Nayara-Hotel', '05-02-16', '10-02-16')");
    PQclear(res);
    res = exec(&ses_2, "INSERT INTO hotel_booking " 
            "(id_, client_name_, hotel_name_, arrival_, departure_) VALUES "
            "(3, 'Kelmer', 'Toronto-Hotel', '11-02-16', '15-02-16')");
    PQclear(res);
    res = exec(&ses_2, "COMMIT");
    PQclear(res);

    close(&ses_1);
    close(&ses_2);
}
