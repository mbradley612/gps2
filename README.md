# GPS2 Library for Mongoose OS

This library provides for Mongoose OS a wrapper for minmea GPS library, https://github.com/kosma/minmea that:

1. Connnects to the GPS chip using UART (and could support other connection types)
2. Provides an Event API to the supported sentences in minmea, and fires the event when the sentence is received:

* RMC (Recommended Minimum: position, velocity, time)
* GGA (Fix Data)
* GSA (DOP and active satellites)
* GLL (Geographic Position: Latitude/Longitude)
* GST (Pseudorange Noise Statistics)
* GSV (Satellites in view)
* VTG (Track made good and Ground speed)
* ZDA (Time & Date - UTC, day, month, year and local time zone)

3. Provides a synchronous API to query for the latest information from the GPS.

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
