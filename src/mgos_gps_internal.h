#include "minmea.h"
#include "mgos_gps.h"



// needs a boolean or booleans to indicate if values populated.
struct minmea_reading {
    struct minmea_float latitude;
    struct minmea_float longitude;
    struct minmea_date date;
    struct minmea_time time;
    struct minmea_float speed;
    struct minmea_float course;
    struct minmea_float altitude; char altitude_units;
    int satellites_tracked; 
    struct minmea_float variation;
    int fix_quality;

};


