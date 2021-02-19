
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
#include "minmea.h"


#define MGOS_EV_GPS_BASE MGOS_EVENT_BASE('G','P','S')

enum mgos_gps_event {
  MGOS_EV_GPS_LOCATION =
    MGOS_EV_GPS_BASE, /* event_data: strict mgos_gps_location */
  MGOS_EV_GPS_NMEA_SENTENCE, /* event_data: strict mgos_gps_nmea_sentence */
  MGOS_EV_GPS_NMEA_STRING
  
};

/* mgos_gps_locatoin built from RMC*/

struct mgos_gps_location {
  float latitude;
  float longitude;
  float bearing;
  float speed;
  float variation;
  time_t time;
  int microseconds;
  int64_t elapsed_time;

};


struct mgos_gps_nmea_sentence {
  enum minmea_sentence_id sentence_id;
  const char *nmea_string; 
};

void mgos_gps_get_latest_location(struct mgos_gps_location *location);



/* ############################################################################# */



struct gps2;

 

/*
* Functions for accessing the global instance. The global instance is created
* automatically if you provide system configuration. The minimum system configuration
* is to specify the UART number. 
*/







/* get the global gps2 device. Returns null if creating the UART handler has failed */
struct gps2 *gps2_get_global_device();





/*
* Functions for managing and accessing individual gps devices
*/


struct gps2;

/* create the gps2 device on a uart and set the event handler. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */

struct gps2 *gps2_create_uart(uint8_t uart_no, struct mgos_uart_config *ucfg);

void gps2_destroy_device(struct gps2 *dev);


/* set the UART baud after initialisation */
bool gps2_set_device_uart_baud(struct gps2 *dev, int baud_rate);

/* set the UART baud after initialisation on the global device*/
bool gps2_set_uart_baud(int baud_rate);



void mgos_gps_device_get_latest_location(struct gps2 *dev, struct mgos_gps_location *location);




/* send a proprietary string command_string to the global GPS */

int gps2_send_command(struct mg_str command_string);

void gps2_send_device_command(struct gps2 *gps_dev, struct mg_str command_string);



