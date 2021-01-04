# GPS Library for Mongoose OS

This library provides a simple and flexible API to obtain location information from GPS devices that output
using NMEA sentences. Callers of the library can register for GPS events or can call directly for the most recent
information.

The events are:

* Location update
* NMEA sentence 
* NMEA string
* GPS status

C Usage for Location:

```
static void location_update_handler(int event, void *event_data, void *userdata) {#
    const struct mgos_gps_location *location = (const struct mgos_gps_location *) event_data;

    double longitude;
    double latitude;
    double altitude;
    double bearing;
    double speed;
    int32_t time;
    int64_t elapsed_time;

    longitude = location.longitude;
    latitude = location.latitude;
    time = location.time;


    LOG(LL_INFO, ("Latitude: %d Longitude %d", longitude, latitude));

    if (mgos_gps_has_location(&location)) {
        altitude = location.altitude;
        LOG (LL_INFO, ("Altitude: %d", altitude));

    } 

    if (mgos_gps_has_bearing(&location)) {
        bearing = location.bearing;
        LOG (LL_INFO, ("Bearing: %d", bearing));

    } 
    if (mgos_gps_has_speed(&Location)) {
        speed = location.speed;
        LOG (LL_INFO, ("Speed: %d", speed));
    }
    elapsed_time = location.elapsed_time;

    
}


mgos_event_add_handler(MGOS_EV_GPS_LOCATION, location_update_handler, NULL); 
```

Javascript Location usage:

```
GPS.addHandler(GPS.LOCATION,
    function(event, eventData, userdata) {
        print("Location update. Longitude: " + eventData.longitude + ", Latitude: " + eventData.latitude);

        if (GPS.hasAltitude(eventData)) {
            print ("Altutude: " + eventData.altitude);
        } 

    }, null );
```

## Acknowledgements

The basic Location API is modelled on the Android Location API, see https://developer.android.com/reference/android/location/package-summary.

The NMEA parsing uses the minmea library, see https://github.com/kosma/minmea


## Old readme starts here

This library provides for Mongoose OS a wrapper for minmea GPS library, https://github.com/kosma/minmea that:

1. Connnects to the GPS chip using UART (and could support other connection types)
2. Provides an Event API to the supported sentences in minmea, and fires the event when the sentence is received:


3. Provides a synchronous API to query for the latest information from the GPS. This API is based on TinyGPS:

```C
long lat, lon;
unsigned long fix_age, time, date, speed, course;
unsigned long chars;
unsigned short sentences, failed_checksum;

/* retrieves +/- lat/long in 100000ths of a degree */
get_position(&lat, &lon, &fix_age);
 
/* time in hhmmsscc, date in ddmmyy */
get_datetime(&date, &time, &fix_age);
 
/* returns speed in 100ths of a knot */
speed();
 
/* course in 100ths of a degree */
course = gps.course();
```

4. Provides an RPC API for the latest information from the GPS.
primitives

On reflection, we need to use the event API so that we can have multiple listeners
to events, e.g.
- Screen
- Storage
- MQTT publisher
and to have different types of event, e.g.
- fix acquired, fix lost
- location 


`struct gps2 *gps2_create_uart(int uart_no, struct mgos_uart_config *cfg, )` 
-- This call creates a new (opaque) object which represents the GPS device. 
uart_no is the UART number, cfg is the UART configuration, see https://mongoose-os.com/docs/mongoose-os/api/core/mgos_uart.h.md

Upon success, a pointer to the object will be returned. If the creation fails, NULL is returned. The pin for the UART interface
 
(If other GPS chips connect using a different interface, eg SPI, we could support this too)

`void gps2_destroy()` -- This cleans up all resources associated with with the GPS device. The caller passes a pointer to the object pointer.

`boolean gps2_`

## RPC interface

`gps.navigation` returns the latest GPS RMC with age in milliseconds

To call the RPC through the UART interface
`call gis.navigation --set-control-lines=false`
