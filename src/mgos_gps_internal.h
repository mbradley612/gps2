#include "minmea.h"
#include "mgos_gps.h"

struct mgos_gps {
    int                             uart_no;
    int                             baud_rate; 
    int                             update_interval;
    struct mgos_uart_config *       ucfg;
    size_t                          dataAvailable;
    struct mgos_gps_reading *       latest_reading;
    void *                          user_data;
};

