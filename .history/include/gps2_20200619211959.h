
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

struct gps2_location {
  float latitude;
  float longitude;
  double speed;
  double course;
  float variation;
};

struct gps2_datetime {
  int year;
  int month;
  int day;
  int hours;
  int minutes;
  int seconds;
  int microseconds;  

};


typedef void (*gps2_ev_handler)(struct gps2 *gps,
                                            int ev, void *ev_data,
                                            void *user_data);

/*
* Functions for using the global instance
*/


/*
* Functions for managing and accessing individual gps devices
*/

/* set the event handler. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */
void gps2_set_ev_handler(gps2_ev_handler handler, void *handler_user_data);

/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/
void gps2_get_location( struct gps2_location *location, int64_t *fix_age);
 
/* date and time */
void gps2_get_datetime(s struct gps2_datetime int64_t *age );

/* unix time now in milliseconds and microseconds adjusted for age*/
void gps2_get_unixtime(time_t *unix_time, int64_t *microseconds);

/* satellites used in last full GPGGA sentence */
void gps2_get_satellites( int *satellites_tracked, int64_t *age);

/* fix quality in last full GPGGA sentence */
void gps2_get_fix_quality(int *fix_quality, int64_t *age);





struct gps2_cfg {
  struct mgos_uart_config;
  gps2_ev_handler handler;
  void *handler_user_data;

};

struct gps2;

void gps2_config_set_default(struct gps2_cfg *cfg);

struct gps2 *gps2_create_uart(struct gps2_cfg *cfg);

void gps2_destroy(struct gps2 *dev);
 
/* set the event handler. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */
void gps2_set_ev_handler(gps2_ev_handler handler, void *handler_user_data);

/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/
void gps2_get_location(struct gps2 *dev, struct gps2_location *location, int64_t *fix_age);
 
/* date and time */
void gps2_get_datetime(struct gps2 *dev, struct gps2_datetime int64_t *age );

/* unix time now in milliseconds and microseconds adjusted for age*/
void gps2_get_unixtime(struct gps2 *dev, time_t *unix_time, int64_t *microseconds);

/* satellites used in last full GPGGA sentence */
void gps2_get_satellites(struct gps2 *dev, int *satellites_tracked, int64_t *age);

/* fix quality in last full GPGGA sentence */
void gps2_get_fix_quality(struct gps2 *dev, int *fix_quality, int64_t *age);

