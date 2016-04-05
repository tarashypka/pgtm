#include "session.h"    /* struct PGsession, open, exec, close */
#include <stdio.h>      /* NULL */
#include "libpq-fe.h"   /* PGresult, PQclear */

#define HOST    "localhost"
#define PORT    "5432"
#define USER    "pti"
#define PWD     "12345678"

#define LOGS_PATH   "/home/deoxys37/Workspace/C/tm/logs/"

int main(int argc, char **argv)
{
    PGsession
    ses_1 = { HOST, PORT,   "fly_db", USER, PWD, NULL, LOGS_PATH },
    ses_2 = { HOST, PORT, "hotel_db", USER, PWD, NULL, LOGS_PATH };
    open(&ses_1);
    open(&ses_2);

    exec(&ses_2, "BEGIN");
    exec(&ses_1, "BEGIN");

    exec(&ses_1, "INSERT INTO fly_booking " 
            "(id_, client_name_, flight_number_, from_, to_, date_) VALUES "
            "(2, 'Kelmer', 'UA-1021', 'Kyiv', 'Costa Rica', '05-02-16')");
    exec(&ses_1, "COMMIT");

    /* client_name_ has UNIQUE CONSTRAINT */
    exec(&ses_2, "INSERT INTO hotel_booking " 
            "(id_, client_name_, hotel_name_, arrival_, departure_) VALUES "
            "(2, 'Kelmer', 'Nayara-Hotel', '05-02-16', '10-02-16')");
    exec(&ses_2, "INSERT INTO hotel_booking " 
            "(id_, client_name_, hotel_name_, arrival_, departure_) VALUES "
            "(3, 'Kelmer', 'Toronto-Hotel', '11-02-16', '15-02-16')");
    exec(&ses_2, "COMMIT");

    close(&ses_1);
    close(&ses_2);
}
