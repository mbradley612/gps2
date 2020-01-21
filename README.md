# Mongoose OS GPS Library

This library provides a simple C (and in future Javascript) API for interfacing with a GPS module that emits NMEA sentences over UART. Callers can use the API to extract:

* position
* date
* time
* altitude
* speed
* course
* number of satellite
* precision

It is modelled on the Mongoose OS IMU library https://github.com/mongoose-os-libs/imu using the basic framework of Alvaro Viebrantz's [GPS library]() (https://github.com/alvarowolfx/gps). The key fields are based on the Arduino TinyGPSplus library http://arduiniana.org/libraries/tinygpsplus/)

## Anatomy

This library connects the UART interface of the chip to the UART interface of the GPS module, reads the NMEA sentences and updates the values so that application authors can simply and easily process the output of a GPS module. The library makes uses of enhancements to sensor-utils to offer conversions to required units.

## GPS API

### GPS primitives

`struct mgos_gps *mgos_gps_create()` -- This call creates a new (opaque) object which represents the GPS device. Upon success, a pointer to the object will be returned. If the creation fails, NULL is returned.

`void mgos_gps_destroy()` -- This cleans up all resources associated with with the GPS device. The caller passes a pointer to the object pointer.

`bool mgos_gps_get()` -- This call returns returns the latest GPS data from the GPS module. It returns true of the read succeeded, in which case the struct of type `mgos_gps_reading` pointed to by reading is populated.

### GPS reading

The GPS reading has the following primitive types:

*  latitude - latitude in degrees (double)
* longitude - longitude in degrees (double)
* date - raw date in DDMMYY format (u32)
* time - raw time in HHMMSSCC format (u32)
* speed - raw speed in 100ths of a knot (i32)
* course - raw course in 100ths of a degree (i32)
* altitude - altitude in metres (double)
* satellites - number of satellites in use (u32)
* hdop - horizontal dimension of precision in 100ths (i32)



The reading can be converted to different formats using sensor-utils. (add functionality and complete)