# GPS2 Library for Mongoose OS

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
gps.get_position(&lat, &lon, &fix_age);
 
/* time in hhmmsscc, date in ddmmyy */
gps.get_datetime(&date, &time, &fix_age);
 
/* returns speed in 100ths of a knot */
speed = gps.speed();
 
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

? do we also want to support MQTT through the event API?
