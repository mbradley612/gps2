#include "minmea.h"
#include "mgos_gps.h"

struct mgos_gps {
    int                             uart_no;
    struct mgos_uart_config *       ucfg;
    size_t                          dataAvailable;
    struct minmea_sentence_rmc *    lastFrame;
    struct mgos_gps_reading *       latest_reading;
    void *                          user_data;
};

