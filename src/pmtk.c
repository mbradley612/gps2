/*
* Plugin to GPS2 to handle MediaTek PMTK sentences. 
*/


#include <stdbool.h>
#include "gps2_internal.h"
#include "pmtk.h"


/* this should go into a separate C file with a plugin callback for the NMEA parsing */

void gps2_send_device_pmtk_command(struct gps2 *gps_dev, struct mg_str command_string) {

  struct mbuf command_buffer;
  struct mg_str crlf;

  crlf = mg_mk_str("\r\n");

  mbuf_init(&command_buffer,command_string.len);

  mbuf_append(&command_buffer,command_string.p,command_string.len);

  mbuf_append(&command_buffer,crlf.p, crlf.len);

  gps2_uart_tx(gps_dev,command_buffer);


  

}

/* send a PMTK command_string to the global GPS 
 
 return false if there is no global device
 
  */ 

void gps2_send_pmtk_command(struct mg_str command_string) {

  if (gps2_get_global_device()) {
    gps2_send_device_pmtk_command(gps2_get_global_device(), command_string);
    return true;
  } else{
    return false;
  }

}


