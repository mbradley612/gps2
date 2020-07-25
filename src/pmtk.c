/*
* Plugin to GPS2 to handle MediaTek PMTK sentences. 
*/


#include <stdbool.h>
#include "minmea.h"
#include "pmtk.h"
#include ## need to move gps_dev to an internal header

enum pmtk_sentence_id pmtk_sentence_id(const char *sentence, bool strict) {

    /* we don't need to do the basic valid sentence check, the nmea library has done this for us already */

    /* call the minmea scan function */

     char type[6];
     if (!minmea_scan(sentence, "t", type)){
        //printf("minmea_check2 error");
        return MINMEA_INVALID;
     }

     if (!strcmp(type, "PMTK001"))
         return PMTK_ACK;

    return PMTK_UNKNOWN;
 
}

bool pmtk_parse_ack(struct pmtk_sentence_ack *frame, const char *sentence) {
    char type[7];
    int command_ackd;
    int flag;
    if (!minmea_scan(sentence, "",
            type,
            &frame->command_ackd,
            &frame->flag
            )) 
        return false;
    if (strcmp(type,"PMTK001"))
        return false;
    

}


bool parsePmtkString(struct mg_str line, struct gps2 *gps_dev) {

}