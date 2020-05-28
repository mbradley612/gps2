
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
  GPS_EV_DATETIME_UPDATE,
  GPS_EV_FIX_ACQUIRED,
  GPS_EV_FIX_LOST,
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

void gps2_destroy(struct gps2 *dev);
 

/* lat/long in MILLIONTHs of a degree and age of fix in milliseconds */
void gps2_get_position(struct gps2 *dev, unsigned long *lat, unsigned long *lon, unsigned long *fix_age);
 
/* date as ddmmyy, time as hhmmsscc, and age in milliseconds */
/* check, is there an mgos preferred way of handling date and time? */
void gps2_get_datetime(struct gps2 *dev, int *year, int *month, int *day, int *hours, int *minutes, int *seconds, int *microseconds, unsigned long *age );
 
/* speed in last full GPRMC sentence in 100ths of a knot */
void gps2_get_speed(struct gps2 *dev, unsigned long *speed, unsigned long *age);
 
/* course in last full GPRMC sentence in 100th of a degree */
void gps2_course(struct gps2 *dev, unsigned long *course, unsigned long *age);

/* satellites used in last full GPGGA sentence */
void gps2_satellites(struct gps2 *dev, unsigned long *course, unsigned long *age);

/* fix quality in last full GPGGA sentence */
void gps2_get_fix_quality(struct gps2 *dev, int *fix_quality)