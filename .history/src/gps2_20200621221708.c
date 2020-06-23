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
 


struct gps_location_reading {
  struct gps2_location location;
  int64_t timestamp;
};

struct gps_datetime_reading {
  struct gps2_datetime datetime;
  int64_t timestamp;
};

struct gps_satellites_reading {
  short satellites_tracked;
  int fix_quality;
  int64_t timestamp;
};




struct gps2 {
  uint8_t uart_no;
  gps2_ev_handler handler; 
  void *handler_user_data; 
  struct mbuf *uart_rx_buffer; 
  struct gps_location_reading location_reading;
  struct gps_datetime_reading datetime_reading;
  struct gps_satellites_reading satellites_reading;

};


static struct gps2 *global_gps_device;


/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/

void gps2_get_device_location(struct gps2 *dev, struct gps2_location *location, int64_t *fix_age) {
  *location = dev->location_reading.location;
  *fix_age = mgos_uptime_micros() - dev->location_reading.timestamp;
}


/* date in last full GPRMC sentence */
void gps2_get_device_datetime(struct gps2 *dev, struct gps2_datetime *datetime, int64_t *age ) {
  *datetime = dev->datetime_reading.datetime;
  *age = mgos_uptime_micros() - dev->datetime_reading.timestamp;
}

void gps2_get_device_unixtime(struct gps2 *dev, time_t *unixtime_now, int64_t *microseconds) {
  struct tm time;
  time_t gps_unixtime;
  int64_t age;


  /* construct a time object to represent the last GPRMC sentence from the GPS device */
  time.tm_year = dev->datetime_reading.datetime.year - 1900;
  time.tm_mon = dev->datetime_reading.datetime.month - 1;
  time.tm_mday = dev->datetime_reading.datetime.day;
  
  time.tm_hour = dev->datetime_reading.datetime.hours;
  time.tm_min = dev->datetime_reading.datetime.minutes;
  time.tm_sec = dev->datetime_reading.datetime.seconds;

  /* turn this into unix time */
  gps_unixtime = mktime(&time);
  LOG(LL_DEBUG,("Time before adjustment: %li",gps_unixtime));
  
  age = mgos_uptime_micros() - dev->datetime_reading.timestamp;

  *unixtime_now = gps_unixtime + age/1000000;

  *microseconds = age % 1000000;
}
 
/* speed in last full GPRMC sentence in 100ths of a knot */
void gps2_get_device_speed(struct gps2 *dev, double *speed, int64_t *age) {
  *speed = dev->location_reading.location.speed;
  *age = mgos_uptime_micros()  - dev->location_reading.timestamp;
}
 
/* course in last full GPRMC sentence in 100th of a degree */
void gps2_get_device_course(struct gps2 *dev, double *course, int64_t *age) {
  *course = dev->location_reading.location.course;
  *age = mgos_uptime_micros()  - dev->location_reading.timestamp;
}

/* satellites used in last full GPGGA sentence */
void gps2_get_device_satellites(struct gps2 *dev, int *satellites_tracked, int64_t *age) {
  *satellites_tracked = dev->satellites_reading.satellites_tracked;
  *age = mgos_uptime_micros()  - dev->satellites_reading.timestamp;

}

/* fix quality in last full GPGGA sentence */
void gps2_get_device_fix_quality(struct gps2 *dev, int *fix_quality, int64_t *age) {
  *fix_quality = dev->satellites_reading.fix_quality;
  *age = mgos_uptime_micros()  - dev->satellites_reading.timestamp;

}




void process_rmc_frame(struct gps2 *gps_dev, struct minmea_sentence_rmc rmc_frame) {

  LOG(LL_DEBUG,("Processing RMC frame"));
  /* lon and lat */
  gps_dev->location_reading.location.latitude = minmea_tocoord(&rmc_frame.latitude);
  gps_dev->location_reading.location.longitude = minmea_tocoord(&rmc_frame.longitude);
  gps_dev->location_reading.timestamp = mgos_uptime_micros();
  /* speed */
  gps_dev->location_reading.location.speed = minmea_tofloat(&rmc_frame.speed);
  /* course */
  gps_dev->location_reading.location.course = minmea_tofloat(&rmc_frame.course);
  /* variation */
  gps_dev->location_reading.location.variation = minmea_tofloat(&rmc_frame.variation);
  
  /* date time */
  gps_dev->datetime_reading.datetime.day = rmc_frame.date.day;
  gps_dev->datetime_reading.datetime.month = rmc_frame.date.month;
  gps_dev->datetime_reading.datetime.year = rmc_frame.date.year + CURRENT_CENTURY;
  gps_dev->datetime_reading.datetime.hours = rmc_frame.time.hours;
  gps_dev->datetime_reading.datetime.minutes = rmc_frame.time.minutes;
  gps_dev->datetime_reading.datetime.seconds = rmc_frame.time.seconds;
  gps_dev->datetime_reading.datetime.microseconds = rmc_frame.time.microseconds;
  gps_dev->datetime_reading.timestamp = mgos_uptime_micros();

  if (gps_dev->handler) {
      /* Tell our handler that we've got a location update*/
        gps_dev->handler(gps_dev, 
          GPS_EV_LOCATION_UPDATE, 
          NULL,
          gps_dev->handler_user_data);
      /* Tell our handler that we've got a datetime update*/
        gps_dev->handler(gps_dev, 
            GPS_EV_DATETIME_UPDATE, 
            NULL,
            gps_dev->handler_user_data);  
      
    }
}

void process_gga_frame(struct gps2 *gps_dev, struct minmea_sentence_gga gga_frame) {

  int previous_fix_quality;

  LOG(LL_DEBUG,("Processing RMC frame"));
  /* we don't process the latest lat and lon as we use the RMC frame to obtain 
  a complete location reading */

   /* satellites tracked */
  gps_dev->satellites_reading.satellites_tracked = gga_frame.satellites_tracked;
  /* fix quality */

  previous_fix_quality = gps_dev->satellites_reading.fix_quality;
  gps_dev->satellites_reading.fix_quality = gga_frame.fix_quality;
  gps_dev->satellites_reading.timestamp = mgos_uptime_micros();



  if (gps_dev->handler) {
      /* Tell our handler that we've got a location update*/
        gps_dev->handler(gps_dev, 
          GPS_EV_LOCATION_UPDATE, 
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

  LOG(LL_DEBUG,("Inside gps2 uart rx callback for UART %i", uart_no));

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
      
      LOG(LL_DEBUG,("Line is %s",line_buffer_nul.p));

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
    
    gps_dev = arg;

    LOG(LL_DEBUG,("GPS2 UART dispatcher called for UART %i", uart_no));

    // check that we've got the correct uart
    assert(gps_dev->uart_no == uart_no);

    // find out how many bytes are available to read
    rx_available = mgos_uart_read_avail(uart_no);

    // if we've got something to read, process it now
    if (rx_available > 0) {
      

      gps2_uart_rx_callback(uart_no,gps_dev,rx_available);
      
    }


    LOG(LL_DEBUG,("GPS2 UART dispatcher exiting for UART %i", uart_no));
 

}

struct gps2 *gps2_create_uart(
  uint8_t uart_no, struct mgos_uart_config *ucfg, gps2_ev_handler handler, void *handler_user_data) {

    struct gps2 *gps_dev = calloc(1, sizeof(struct gps2));
      
    struct mbuf *uart_rx_buffer = calloc(1, sizeof(struct mbuf));

    /* check we have a uart config. If not, return null */
    if (ucfg == NULL) {
      return NULL;
    }

    gps_dev->uart_no = uart_no;
    gps_dev->handler = handler;
    gps_dev->handler_user_data = handler_user_data;

    /* we set the initial size of our receive buffer to the size of the UART receive buffer. It will grow
    automatically if required */
    mbuf_init(uart_rx_buffer,512);

    gps_dev->uart_rx_buffer = uart_rx_buffer;
    
    if (!mgos_uart_configure(gps_dev->uart_no, ucfg)) goto err;
    

    LOG(LL_INFO, ("UART%d initialized %u,%d%c%d", gps_dev->uart_no, ucfg->baud_rate,
                ucfg->num_data_bits,
                ucfg->parity == MGOS_UART_PARITY_NONE ? 'N' : ucfg->parity + '0',
                ucfg->stop_bits));

    // set our callback for the UART for the GPS
    mgos_uart_set_dispatcher(gps_dev->uart_no,gps2_uart_dispatcher,gps_dev);
    
    mgos_uart_set_rx_enabled(gps_dev->uart_no, true);

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

  mgos_uart_config_set_defaults(uart_no,&ucfg);
  

  ucfg.num_data_bits = 8;
  ucfg.parity = MGOS_UART_PARITY_NONE;
  ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
  ucfg.tx_buf_size = 512; /*mgos_sys_config_get_gps_uart_tx_buffer_size();*/
  ucfg.rx_buf_size = 128; /*mgos_sys_config_get_gps_uart_rx_buffer_size();*/

  global_gps_device = gps2_create_uart(uart_no, &ucfg, NULL, NULL);

  return global_gps_device;


}

/* set the event handler. The handler callback will be called when the GPS is initialized, when a GPS fix
  is acquired or lost and whenever there is a location update */
void gps2_set_device_ev_handler(struct gps2 *device, gps2_ev_handler handler, void *handler_user_data) {
  device->handler = handler;
  device->handler_user_data = handler_user_data;
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
void gps2_get_location(struct gps2_location *location, int64_t *fix_age) {
  gps2_get_device_location(gps2_get_global_device(), location, fix_age);
}
 
/* date and time */
void gps2_get_datetime(struct gps2_datetime *datetime, int64_t *age ) {
  gps2_get_device_datetime(gps2_get_global_device(), datetime, age);
}

/* unix time now in milliseconds and microseconds adjusted for age*/
void gps2_get_unixtime(time_t *unix_time, int64_t *microseconds) {
  gps2_get_device_unixtime(gps2_get_global_device(),unix_time,microseconds);
}

/* satellites used in last full GPGGA sentence */
void gps2_get_satellites( int *satellites_tracked, int64_t *age) {
  gps2_get_device_satellites(gps2_get_global_device(),satellites_tracked,age);
}

/* fix quality in last full GPGGA sentence */
void gps2_get_fix_quality(int *fix_quality, int64_t *age) {
  gps2_get_global_device(gps2_get_global_device(),fix_quality,age);
}





enum mgos_init_result mgos_gps2_init(void) {
  uint8_t gps_config_uart_no;

  gps_config_uart_no = mgos_sys_config_get_gps_uart_no();

  /* check if we have a UART > 0. If so, create the global instance */  
  if (gps_config_uart_no > 0) {
    if (create_global_device(gps_config_uart_no)) {
      LOG(LL_INFO,("Successfully created global GPS device on UART %i", gps_config_uart_no));
    } else {
      LOG(LL_ERROR,("Failed to create global instance with uart %i",gps_config_uart_no));
      return MGOS_INIT_APP_INIT_FAILED;
    } 
  }
  LOG(LL_DEBUG,("About to return success from init"));
  return true;
    
  
}