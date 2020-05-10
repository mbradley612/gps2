
/**
* RMC (Recommended Minimum: position, velocity, time)
* GGA (Fix Data)
* GSA (DOP and active satellites)
* GLL (Geographic Position: Latitude/Longitude)
* GST (Pseudorange Noise Statistics)
* GSV (Satellites in view)
* VTG (Track made good and Ground speed)
* ZDA (Time & Date - UTC, day, month, year and local time zone)
*/

// the event mechanism is modelled on mgos_net, see 
// https://mongoose-os.com/docs/mongoose-os/api/core/mgos_net.h.md

#define EVENT_GRP_GPS MGOS_EVENT_BASE('G', 'P', 'S')

// simpler approach would be to map this straight on to the NMEA sentences. This
// would allow the calling application to have full control.

// simpler apps can use the synchronous API.

enum GPS_EVENT {
  GPS_EV_INITIALIZED = EVENT_GRP_GPS,
  GPS_EV_LOCATION_UPDATE,
  GPS_EV_FIX_ACQUIRED,
  GPS_EV_FIX_LOST,
};



struct gps_time {
  int hours;
  int minutes;
  int seconds;
  int microseconds;
};

struct gps_date {
  int day;
  int month;
  int year;
};

struct gps_float {
  int_least32_t value;
  int_least32_t scale;
};

struct gps_info {
  struct gps_time time;
  struct gps_date date;
  struct gps_float latitude;
  struct gps_float speed;
  struct gps_float course;
  struct gps_float variation;

};

struct gps2;

typedef void (*gps2_ev_handler)(struct gps2 *gps,
                                            int ev, void *ev_data,
                                            void *user_data);

struct gps2_cfg {
  int8_t uart_no;
  int32_t uart_baud_rate;
  gps2_ev_handler handler;
  void *handler_user_data;

};

struct gps2;

void gps2_config_set_default(struct gps2_cfg *cfg);

struct gps2 *gps2_create_uart(struct gps2_cfg *cfg);

void mgos_gps_destroy();
