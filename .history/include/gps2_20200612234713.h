
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

/* the event mechanism is modelled on mgos_fingerprint, see
* https://github.com/mongoose-os-libs/fingerprint
*/


#define GPS_EV_NONE 0x0000
#define GPS_EV_INITIALIZED 0x0001
#define GPS_EV_LOCATION_UPDATE 0x0002
#define GPS_EV_DATETIME_UPDATE 0x0003
#define GPS_EV_FIX_ACQUIRED 0x0004
#define GPS_EV_FIX_LOST 0x0005




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
 

/* lat/long in degrees and age of fix in milliseconds */
void gps2_get_position(struct gps2 *dev, float *lat,float *lon, int64_t *fix_age);
 
/* date and time */
void gps2_get_datetime(struct gps2 *dev, int *year, int *month, int *day, int *hours, int *minutes, int *seconds, int *microseconds, int64_t *age );

/* unix time in microseconds */
void gps2_get_unixtime(struct gps2 *dev, int64_t *unix_time, int64_t *unix_time_age);

/* speed in last full GPRMC sentence in 100ths of a knot */
/* TODO change to metres per second */
void gps2_get_speed(struct gps2 *dev, double *speed, int64_t *age);
 
/* course in last full GPRMC sentence in 100th of a degree */
void gps2_get_course(struct gps2 *dev, double *course, int64_t *age);

/* satellites used in last full GPGGA sentence */
void gps2_get_satellites(struct gps2 *dev, int *satellites_tracked, int64_t *age);

/* fix quality in last full GPGGA sentence */
void gps2_get_fix_quality(struct gps2 *dev, int *fix_quality, int64_t *age);