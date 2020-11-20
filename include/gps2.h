
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

#include "mgos.h"

#define GPS_EV_NONE 0x0000
#define GPS_EV_INITIALIZED 0x0001
#define GPS_EV_RMC 0x0002
#define GPS_EV_GGA 0x0003
#define GPS_EV_CONNECTED 0x0004
#define GPS_EV_TIMEDOUT 0x0005
#define GPS_EV_FIX_ACQUIRED 0x0006
#define GPS_EV_FIX_LOST 0x0007


#define MGOS_EV_GPS_BASE MGOS_EVENT_BASE('G','P','S')

enum mgos_gps_event {
  MGOS_EV_GPS_LOCATION =
    MGOS_EV_GPS_BASE, /* event_data: strict mgos_gps_location */
  MGOS_EV_GPS_NMEA_SENTENCE, /* event_data: strict mgos_gps_nmea_sentence */
  MGOS_EV_GPS_NMEA_STRING, /* event_data: strict mgos_gps_nmea_string */
  MGOS_EV_GPS_STATUS /* event_data: strict mgos_gps_status */
  
};

struct mgos_gps_location {
  float latitude;
  float longitude;
  float altitude;
  float bearing;
  float speed;
  time_t time;
  int64_t elapsed_time;
  char values_filled;

};

int mgos_gps_has_bearing(const struct mgos_gps_location *location);

int mgos_gps_has_speed(const struct  mgos_gps_location *location);

int mgos_gps_has_altitude(const struct  mgos_gps_location *location);

enum mgos_gps_gga_fix_quality {
  MGOS_GPS_GGA_FIX_NOT_AVAILABLE = 0,
  MGOS_GPS_GGA_GPS_FIX = 1,
  MGOS_GPS_GGA_DGPS_FIX = 2
};



/* ############################################################################# */



struct gps2;




typedef void (*gps2_ev_handler)(struct gps2 *gps,
                                            int ev, void *ev_data,
                                            void *user_data);
struct gps2_datetime {
  int year;
  int month;
  int day;
  int hours;
  int minutes;
  int seconds;
  int microseconds;  

};


struct gps2_time {
  int hours;
  int minutes;
  int seconds;
  int microseconds;  

};


struct gps2_rmc {
  struct gps2_datetime datetime;
  float latitude;
  float longitude;
  float speed;
  float course;
  float variation;
};


struct gps2_gga {
  struct gps2_time time;
  float latitude;
  float longitude;
  int fix_quality;
  int satellites_tracked;
  float hdop;
  float altitude;
  char altitude_units;
  float height;
  char height_units;
  int dgps_age;

};
             
/*
* Callback for the event handler
*/
typedef void (*gps2_ev_handler)(struct gps2 *gps,
                                            int ev, void *ev_data,
                                            void *user_data);


/*
* Functions for accessing the global instance. The global instance is created
* automatically if you provide system configuration. The minimum system configuration
* is to specify the UART number. 
*/



/* set the event handler for the global device. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */
void gps2_set_ev_handler(gps2_ev_handler handler, void *handler_user_data);

/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/
void gps2_get_latest_rmc(struct gps2_rmc *latest_rmc, int64_t *age);

/* satellites used in last full GPGGA sentence */
void gps2_get_latest_gga(struct gps2_gga *latest_gga, int64_t *age);


/* get the global gps2 device. Returns null if creating the UART handler has failed */
struct gps2 *gps2_get_global_device();





/*
* Functions for managing and accessing individual gps devices
*/


struct gps2;

/* create the gps2 device on a uart and set the event handler. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */

struct gps2 *gps2_create_uart(uint8_t uart_no, struct mgos_uart_config *ucfg, gps2_ev_handler handler, 
  void *handler_user_data);

void gps2_destroy_device(struct gps2 *dev);

/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/
void gps2_get_device_latest_rmc(struct gps2 *dev, struct gps2_rmc *latest_rmc, int64_t *fix_age);
 
/* date and time */
void gps2_get_device_latest_gga(struct gps2 *dev, struct gps2_gga *latest_gga, int64_t *age );

/* unix time now in milliseconds and microseconds adjusted for age from the last rmc sentence*/
void gps2_get_device_unixtime_from_latest_rmc(struct gps2 *dev, time_t *unix_time, int64_t *microseconds);



/* set the event handler for a device. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */
void gps2_set_device_ev_handler(struct gps2 *dev, gps2_ev_handler handler, void *handler_user_data);

/* set the UART baud after initialisation */
bool gps2_set_device_uart_baud(struct gps2 *dev, int baud_rate);

/* set the UART baud after initialisation on the global device*/
bool gps2_set_uart_baud(int baud_rate);

/* enable the disconnect timer. Implemented as a software timer runs on a repeat cycle. 
  If no NMEA sentence is received between callbacks, the disconnect event fires. Set to
  0 to clear the timer for the device */
 
 void gps2_enable_disconnect_timer(int disconnect_timeout);

 /* set the disconnect timeout on a device */
void gps2_enable_device_disconnect_timer(struct gps2 *dev, int disconnect_timeout);

/* disable the disconnect timer */
void gps2_disable_disconnect_timer();

/* disable the disconnect timer on a device*/
void gps2_disable_disconnect_timer(struct gps2 *dev);






/* send a proprietary string command_string to the global GPS */

void gps2_send_command(struct mg_str command_string);

void gps2_send_device_command(struct gps2 *gps_dev, struct mg_str command_string);


/* callback for when the gps device emits a proprietary sentence */

typedef void (*gps2_proprietary_sentence_parser)(struct mg_str line, struct gps2 *gps_dev);        


void gps2_set_proprietary_sentence_parser(gps2_proprietary_sentence_parser prop_sentence_parser);

void gps2_set_device_proprietary_sentence_parser(struct gps2 *gps_dev, gps2_proprietary_sentence_parser prop_sentence_parser);


