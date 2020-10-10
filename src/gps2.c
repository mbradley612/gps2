/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "mgos.h"
#include "time.h"
#include "mgos_time.h"
#include "gps2.h"


#include "minmea.h"

#define CURRENT_CENTURY 2000

#define GPS2_PMTK 1
 





struct gps2 {
  uint8_t uart_no;
  gps2_ev_handler handler; 
  gps2_proprietary_sentence_parser proprietary_sentence_parser;
  void *handler_user_data; 

  struct mbuf *uart_rx_buffer;
  struct mbuf *uart_tx_buffer; 
  int64_t latest_rx_timestamp;
  struct mgos_uart_config  uart_config;
  int disconnect_timeout;
  mgos_timer_id disconnect_timer_id;
  
  struct minmea_sentence_rmc latest_rmc;
  int64_t latest_rmc_timestamp;

  struct minmea_sentence_gga latest_gga;
  int64_t latest_gga_timestamp;

};


static struct gps2 *global_gps_device;


/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/

void gps2_get_device_latest_rmc(struct gps2 *dev,struct gps2_rmc *latest_rmc, int64_t *age) {
  latest_rmc->datetime.year = dev->latest_rmc.date.year;
  latest_rmc->datetime.month = dev->latest_rmc.date.month;
  latest_rmc->datetime.day = dev->latest_rmc.date.day;
  latest_rmc->datetime.hours = dev->latest_rmc.time.hours;
  latest_rmc->datetime.minutes = dev->latest_rmc.time.minutes;
  latest_rmc->datetime.seconds = dev->latest_rmc.time.seconds;
  latest_rmc->datetime.microseconds = dev->latest_rmc.time.microseconds;

  latest_rmc->latitude = minmea_tocoord(&(dev->latest_rmc.latitude));
  latest_rmc->longitude = minmea_tocoord(&(dev->latest_rmc.longitude));
  latest_rmc->speed = minmea_tofloat(&(dev->latest_rmc.speed));
  latest_rmc->course = minmea_tofloat(&(dev->latest_rmc.course));
  latest_rmc->variation = minmea_tofloat(&(dev->latest_rmc.variation));
  

  *age = mgos_uptime_micros() - dev->latest_rmc_timestamp;
}

void gps2_get_device_latest_gga(struct gps2 *dev,struct gps2_gga *latest_gga, int64_t *age) {
  latest_gga->time.hours = dev->latest_gga.time.hours;
  latest_gga->time.minutes = dev->latest_gga.time.minutes;
  latest_gga->time.seconds = dev->latest_gga.time.seconds;
  latest_gga->time.microseconds = dev->latest_gga.time.microseconds;

  latest_gga->latitude = minmea_tocoord(&(dev->latest_gga.latitude));
  latest_gga->longitude = minmea_tocoord(&(dev->latest_gga.longitude));
  latest_gga->fix_quality = dev->latest_gga.fix_quality;
  latest_gga->satellites_tracked = dev->latest_gga.satellites_tracked;
  latest_gga->hdop = minmea_tofloat(&dev->latest_gga.hdop);
  latest_gga->altitude = minmea_tofloat(&dev->latest_gga.altitude);
  latest_gga->altitude_units = dev ->latest_gga.altitude_units;
  latest_gga->height = minmea_tofloat(&dev -> latest_gga.height);
  latest_gga->height_units = dev -> latest_gga.height_units;
  latest_gga->dgps_age = dev -> latest_gga.dgps_age;

  

  *age = mgos_uptime_micros() - dev->latest_gga_timestamp;
}


void gps2_get_device_unixtime_from_latest_rmc(struct gps2 *dev, time_t *unixtime_now, int64_t *microseconds) {
  struct tm time;
  time_t gps_unixtime;
  int64_t age;


  /* construct a time object to represent the last GPRMC sentence from the GPS device */
  time.tm_year = dev->latest_rmc.date.year - 1900;
  time.tm_mon = dev->latest_rmc.date.month - 1;
  time.tm_mday = dev->latest_rmc.date.day;
  
  time.tm_hour = dev->latest_rmc.time.hours;
  time.tm_min = dev->latest_rmc.time.minutes;
  time.tm_sec = dev->latest_rmc.time.seconds;

  /* turn this into unix time */
  gps_unixtime = mktime(&time);
  
  age = mgos_uptime_micros() - dev->latest_rmc_timestamp;

  *unixtime_now = gps_unixtime + age/1000000;

  *microseconds = age % 1000000;
}




void process_rmc_frame(struct gps2 *gps_dev, struct minmea_sentence_rmc rmc_frame) {

  LOG(LL_DEBUG,("Processing RMC frame"));
  /* lon and lat */
  gps_dev->latest_rmc = rmc_frame;
  gps_dev->latest_rmc_timestamp = mgos_uptime_micros();

  if (gps_dev->handler) {
      /* Tell our handler that we've got a location update*/
        gps_dev->handler(gps_dev, 
          GPS_EV_RMC, 
          NULL,
          gps_dev->handler_user_data);
     
      
    }
}

void process_gga_frame(struct gps2 *gps_dev, struct minmea_sentence_gga gga_frame) {

  int previous_fix_quality;

  LOG(LL_DEBUG,("Processing RMC frame"));
  /* we don't process the latest lat and lon as we use the RMC frame to obtain 
  a complete location reading */


  previous_fix_quality = gps_dev->latest_gga.fix_quality;

   /* satellites tracked */
  
  
  gps_dev->latest_gga = gga_frame;
  gps_dev->latest_gga_timestamp = mgos_uptime_micros();





  if (gps_dev->handler) {
      /* Tell our handler that we've got a location update*/
        gps_dev->handler(gps_dev, 
          GPS_EV_GGA, 
          NULL,
          gps_dev->handler_user_data);
      
      
    }
  

  /* check if we've just lost our fix */
  if (gga_frame.fix_quality ==0 && previous_fix_quality >0) {
    if (gps_dev->handler) {
      /* Tell our handler that we've lost our fix*/
        gps_dev->handler(gps_dev, 
            GPS_EV_FIX_LOST, 
            NULL,
            gps_dev->handler_user_data);  
      
    }

  }

  /* check if we've aquired our fix */
  if (gga_frame.fix_quality > 0 && previous_fix_quality ==0) {
    if (gps_dev->handler) {
      /* Tell our handler that we've got a fix */
        gps_dev->handler(gps_dev, 
          GPS_EV_FIX_ACQUIRED, 
          NULL,
          gps_dev->handler_user_data);
      
      
    }
  }

}


void parseNmeaString(struct mg_str line, struct gps2 *gps_dev) {
  

  enum minmea_sentence_id sentence_id;

  
  /* parse the sentence */
  sentence_id = minmea_sentence_id(line.p, false);

  

  switch (sentence_id) {
    case MINMEA_SENTENCE_RMC: {
      struct minmea_sentence_rmc frame;
      if (minmea_parse_rmc(&frame, line.p)) {
        process_rmc_frame(gps_dev, frame);
      }
    } break;
    case MINMEA_SENTENCE_GGA: {
      struct minmea_sentence_gga frame;
      if (minmea_parse_gga(&frame, line.p)) {
        process_gga_frame(gps_dev, frame);
      }
    } break;
    
    case MINMEA_SENTENCE_PROPRIETARY: {

      LOG(LL_DEBUG,("NMEA library says proprietary sentence"));
      // if we have a callback handler, call it
      if (gps_dev->proprietary_sentence_parser !=NULL) {
        gps_dev->proprietary_sentence_parser(line,gps_dev);

      }
      // call a callback function with line and gps_dev
    } break;
    case MINMEA_UNKNOWN: {
      LOG(LL_DEBUG,("NMEA library says sentence unknown"));
      
    } break;
    default: {
      /* do nothing */
      ;
    } break;
  }

  
  
  if (sentence_id) {
    LOG(LL_DEBUG,("NMEA sentence id: %d",sentence_id));
  } else {
    LOG(LL_DEBUG,("NMEA library failed to parse sentence"));
  }
  (void)gps_dev;

}





/*
* NMEA strings end CR LF i.e. "\n"
* see https://en.wikipedia.org/wiki/NMEA_0183
*/

void gps2_uart_rx_callback(int uart_no, struct gps2 *gps_dev, size_t rx_available) {
  struct mg_str line_buffer;
  struct mg_str line_buffer_nul;
  size_t line_length;
  const char *terminator_ptr;  
  

  const struct mg_str crlf = mg_mk_str("\r\n");

  /* read the UART into our line buffer. */
  mgos_uart_read_mbuf(uart_no,gps_dev->uart_rx_buffer,rx_available);


  /* if we've got anything in the buffer, look for the first "\n" */
  if (gps_dev->uart_rx_buffer->len > 0) {

    
    /* Look for a CRLF */
    line_buffer = mg_mk_str_n(gps_dev->uart_rx_buffer->buf,gps_dev->uart_rx_buffer->len);
    terminator_ptr = mg_strstr(line_buffer, crlf);
      
    while (terminator_ptr != NULL)
    {
      
      /* calculate the length of the line using pointer arithmetic */
      line_length = crlf.len + (terminator_ptr - gps_dev->uart_rx_buffer->buf);

      
      /* create a string from the line. */
      line_buffer = mg_mk_str_n(gps_dev->uart_rx_buffer->buf, line_length);

      line_buffer_nul = mg_strdup_nul(line_buffer);
      
      LOG(LL_DEBUG,("RX line is %s",line_buffer_nul.p));


      /* if we weren't already connected, set connected to 1 and fire a connected event */
      if (gps_dev->latest_rx_timestamp ==0) {
        if (gps_dev->handler) {
      /* Tell our handler that we've lost our fix*/
        gps_dev->handler(gps_dev, 
            GPS_EV_CONNECTED, 
            NULL,
            gps_dev->handler_user_data);  
      
        }


      }

      /* set our latest rx timestamp to uptime*/
      gps_dev->latest_rx_timestamp = mgos_uptime_micros();

      

      /* parse the line */
      parseNmeaString(line_buffer_nul, gps_dev);

      /* free out line_buffer_nul */
      mg_strfree(&line_buffer_nul);
      
      /* remove the line from the beginning of the buffer */
      mbuf_remove(gps_dev->uart_rx_buffer, line_length);

      /* Look for the next crlf*/
      line_buffer = mg_mk_str_n(gps_dev->uart_rx_buffer->buf,gps_dev->uart_rx_buffer->len);
      terminator_ptr = mg_strstr(line_buffer, crlf);

    }
  }



}

void gps2_uart_dispatcher(int uart_no, void *arg){
    struct gps2 *gps_dev;
    size_t rx_available;
    size_t tx_available;
    size_t length_to_write;
    struct mg_str tx_string;
    struct mg_str tx_string_nul;
    
    gps_dev = arg;


    // check that we've got the correct uart
    assert(gps_dev->uart_no == uart_no);

    // find out how many bytes are available to read
    rx_available = mgos_uart_read_avail(uart_no);

    // if we've got something to read, process it now
    if (rx_available > 0) {
      

      gps2_uart_rx_callback(uart_no,gps_dev,rx_available);
      
    }

    /* check if we've got anything to write */
    if (gps_dev->uart_tx_buffer->len > 0 ) {

      // find out how many bytes are available to write
      tx_available = mgos_uart_write_avail(uart_no);

      // write what we've got in our tx buffer up to the tx available

      length_to_write = tx_available < gps_dev->uart_tx_buffer->len ? tx_available : gps_dev->uart_tx_buffer->len;

      mgos_uart_write(uart_no,gps_dev->uart_tx_buffer->buf, length_to_write); 

            /* create a string from the line. */
      tx_string = mg_mk_str_n(gps_dev->uart_tx_buffer->buf, length_to_write);


      tx_string_nul = mg_strdup_nul(tx_string);

      LOG(LL_DEBUG,("TX line us %s",tx_string_nul.p));
      


      // and flush
      mgos_uart_flush(uart_no);


      // remove what we've written from the buffer

      mbuf_remove(gps_dev->uart_tx_buffer,length_to_write);

      /* if we didn't write everything then when the tx buffer empties this dispatcher will be called and we 
      can can write again */


    }


 

}

void gps2_uart_tx(struct gps2 *gps_dev, struct mbuf buffer) {
  /* append to the tx buffer */
  mbuf_append(gps_dev->uart_tx_buffer,buffer.buf,buffer.len);

  /* call the dispatcher */
  gps2_uart_dispatcher(gps_dev->uart_no, gps_dev);

}

void gps2_send_device_command(struct gps2 *gps_dev, struct mg_str command_string) {

  struct mbuf command_buffer;
  struct mg_str crlf;

  crlf = mg_mk_str("\r\n");
  mbuf_init(&command_buffer,command_string.len);
  mbuf_append(&command_buffer,command_string.p,command_string.len);
  mbuf_append(&command_buffer,crlf.p, crlf.len);
  gps2_uart_tx(gps_dev,command_buffer);

}

/* send a  command_string to the global GPS 
 
 return false if there is no global device
 
  */ 

void gps2_send_command(struct mg_str command_string) {

  if (gps2_get_global_device()) {
    gps2_send_device_command(gps2_get_global_device(), command_string);
    return true;
  } else{
    return false;
  }

}

/* set the UART baud after initialisation */
/* returns true if successful */

bool gps2_set_device_uart_baud(struct gps2 *dev, int baud_rate) {

  int current_baud;

  // capture the current baud
  current_baud = dev->uart_config.baud_rate;

  // update the baud on UART config on our device
  dev->uart_config.baud_rate = baud_rate;

  // reset the timestamp for the last received data. This will
  // force a connected event
  dev->latest_rx_timestamp = 0;

  // apply it to the UART device
  if (mgos_uart_configure(dev->uart_no, &(dev->uart_config))) {
    return true;
  } else {
    // revert back the baud
    dev->uart_config.baud_rate = current_baud;
    return false;

  }


}

/* set the UART baud after initialisation on the global device*/
bool gps2_set_uart_baud(int baud_rate) {
  if (gps2_get_global_device()) {
    gps2_set_device_uart_baud(gps2_get_global_device(), baud_rate);
    return true;
  } else{
    return false;
  }
}


struct gps2 *gps2_create_uart(
  uint8_t uart_no, struct mgos_uart_config *ucfg, gps2_ev_handler handler, void *handler_user_data) {

    struct gps2 *gps_dev = calloc(1, sizeof(struct gps2));
      
    struct mbuf *uart_rx_buffer = calloc(1, sizeof(struct mbuf));
    struct mbuf *uart_tx_buffer = calloc(1, sizeof(struct mbuf));


    /* check we have a uart config. If not, return null */
    if (ucfg == NULL) {
      return NULL;
    }
    


    gps_dev->uart_no = uart_no;
    gps_dev->handler = handler;
    gps_dev->handler_user_data = handler_user_data;
    memcpy(&(gps_dev->uart_config),ucfg,sizeof(struct mgos_uart_config));


    /* we set the initial size of our receive buffer to the size of the UART receive buffer. It will grow
    automatically if required */
    mbuf_init(uart_rx_buffer,512);

    gps_dev->uart_rx_buffer = uart_rx_buffer;
    gps_dev->uart_tx_buffer = uart_tx_buffer;
    
    
    if (!mgos_uart_configure(gps_dev->uart_no, &(gps_dev->uart_config))) goto err;
    

    //if (!mgos_uart_configure(gps_dev->uart_no, ucfg)) goto err;

    LOG(LL_INFO, ("UART%d initialized %u,%d%c%d", gps_dev->uart_no, ucfg->baud_rate,
                ucfg->num_data_bits,
                ucfg->parity == MGOS_UART_PARITY_NONE ? 'N' : ucfg->parity + '0',
                ucfg->stop_bits));

    // set our callback for the UART for the GPS
    mgos_uart_set_dispatcher(gps_dev->uart_no,gps2_uart_dispatcher,gps_dev);
    
    mgos_uart_set_rx_enabled(gps_dev->uart_no, true);

    /* set our latest rx timestamp to 0 */
    gps_dev->latest_rx_timestamp = 0;

    /* set our disconnect timout to 0. This disables the functionality. 
      * call gps2_set_disconnect_timeout to turn on this feature.
    */
    gps_dev->disconnect_timeout = 0;

    LOG(LL_INFO, ("Initialized GPS device"));
    if (gps_dev->handler) {
          gps_dev->handler(gps_dev, 
          GPS_EV_INITIALIZED, 
          NULL,
          gps_dev->handler_user_data);
    }
    return gps_dev;


    err:
      if (gps_dev) free(gps_dev);
      return NULL;


}



static struct gps2 *create_global_device(uint8_t uart_no) {

  struct mgos_uart_config ucfg;
  int disconnect_timeout;

  mgos_uart_config_set_defaults(uart_no,&ucfg);

  ucfg.num_data_bits = 8;
  ucfg.parity = MGOS_UART_PARITY_NONE;
  ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
  ucfg.baud_rate  = mgos_sys_config_get_gps_uart_baud();
  ucfg.tx_buf_size = mgos_sys_config_get_gps_uart_tx_buffer_size();
  ucfg.rx_buf_size = mgos_sys_config_get_gps_uart_rx_buffer_size();

  global_gps_device = gps2_create_uart(uart_no, &ucfg, NULL, NULL);

  disconnect_timeout = mgos_sys_config_get_gps_uart_disconnect_timeout();
  if (disconnect_timeout > 0) {
    gps2_enable_disconnect_timer(disconnect_timeout);
  }

  return global_gps_device;


}

/* set the event handler. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */
void gps2_set_device_ev_handler(struct gps2 *dev, gps2_ev_handler handler, void *handler_user_data) {
  dev->handler = handler;
  dev->handler_user_data = handler_user_data;
}

/* get the global gps2 device. Returns null if creating the UART handler has failed */
struct gps2 *gps2_get_global_device() {
  return global_gps_device;
}

/* set the event handler. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */
void gps2_set_ev_handler(gps2_ev_handler handler, void *handler_user_data) {
  gps2_get_global_device()->handler = handler;
  gps2_get_global_device()->handler_user_data = handler_user_data;
}


/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/
void gps2_get_latest_rmc(struct gps2_rmc *latest_rmc, int64_t *age) {
  gps2_get_device_latest_rmc(gps2_get_global_device(),latest_rmc, age);
};

/* satellites used in last full GPGGA sentence */
void gps2_get_latest_gga(struct gps2_gga *latest_gga, int64_t *age) {
  gps2_get_device_latest_gga(gps2_get_global_device(),latest_gga, age);

};

/* unix time now in milliseconds and microseconds adjusted for age*/
void gps2_get_unixtime_from_latest_rmc(time_t *unix_time, int64_t *microseconds) {
  gps2_get_device_unixtime_from_latest_rmc(gps2_get_global_device(),unix_time,microseconds);
}




void gps2_set_proprietary_sentence_parser(gps2_proprietary_sentence_parser prop_sentence_parser) {
  gps2_get_global_device()->proprietary_sentence_parser = prop_sentence_parser;

}

void gps2_set_device_proprietary_sentence_parser(struct gps2 *gps_dev, gps2_proprietary_sentence_parser prop_sentence_parser) {
  gps_dev->proprietary_sentence_parser = prop_sentence_parser;

}


/*
* Callback for the disconnect timer for a device
*/
void gps2_disconnect_timer_callback(void *arg) {
  struct gps2 *gps_dev;
  
  int64_t uptime_now;

  gps_dev = arg;

  /* if we are disconnected, return now */
  if (gps_dev->latest_rx_timestamp ==0) {
    return;
  }

  /* check to see if we have received a sentence within the disconnect timeout */
  uptime_now = mgos_uptime_micros();


  if ((uptime_now - gps_dev->latest_rx_timestamp) > (gps_dev->disconnect_timeout) * 1000 ) {
    /* if we have timed out, set the latest_rx_timestamp to 0 and fire the timedout event */
    gps_dev->latest_rx_timestamp = 0;
    
    if (gps_dev->handler) {
          gps_dev->handler(gps_dev, 
          GPS_EV_TIMEDOUT, 
          NULL,
          gps_dev->handler_user_data);
    }
  }





}


/* set the disconnect timeout. If no NMEA sentence is received within this time, the API will fire
 a disconnect event */
void gps2_enable_disconnect_timer(int disconnect_timeout) {
  gps2_enable_device_disconnect_timer(gps2_get_global_device(),disconnect_timeout);

}

 /* set the disconnect timeout on a device */
void gps2_enable_device_disconnect_timer(struct gps2 *dev, int disconnect_timeout) {
  dev->disconnect_timeout = disconnect_timeout;
  dev->disconnect_timer_id = mgos_set_timer(disconnect_timeout,MGOS_TIMER_REPEAT,gps2_disconnect_timer_callback,dev);

}




enum mgos_init_result mgos_gps2_init(void) {
  uint8_t gps_config_uart_no;
  uint8_t gps_config_uart_baud;

  gps_config_uart_no = mgos_sys_config_get_gps_uart_no();
  gps_config_uart_baud = mgos_sys_config_get_gps_uart_baud();

  /* check if we have a UART > 0. If so, create the global instance */  
  if (gps_config_uart_no > 0 && gps_config_uart_baud > 0) {
    if (create_global_device(gps_config_uart_no)) {
      LOG(LL_INFO,("Successfully created global GPS device on UART %i", gps_config_uart_no));
    } else {
      if (gps_config_uart_baud ==0) {
        LOG(LL_ERROR,("You must set the baud rate in config: gps.uart.baud"));
      }
      LOG(LL_ERROR,("Failed to create global instance with uart %i",gps_config_uart_no));
      return MGOS_INIT_APP_INIT_FAILED;
    } 
  }
  LOG(LL_DEBUG,("About to return success from init"));
  return true;
    
  
}