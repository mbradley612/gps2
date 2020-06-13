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
#include "mgos_time.h"
#include "gps2.h"

#include "minmea.h"

struct gps_position {
  float lat;
  float lon;
  int64_t timestamp;
};

struct gps_datetime {
  uint8_t day;
  uint8_t month;
  uint8_t year;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t microseconds;

  int64_t timestamp;
};

struct gps_speed {
  double speed;
  int64_t timestamp;
};

struct gps_course {
  float course;
  int64_t timestamp;
};

struct gps_satellites {
  short satellites_tracked;
  int64_t timestamp;
};

struct gps_fix_quality {
  int fix_quality;
  int64_t timestamp;
};




struct gps2 {
  uint8_t uart_no;
  gps2_ev_handler handler; //shouldn't be here
  void *handler_user_data; // shouldn't be here
  struct mbuf *uart_rx_buffer; 
  struct gps_position position;
  struct gps_datetime datetime;
  struct gps_speed speed;
  struct gps_course course;
  struct gps_satellites satellites;
  struct gps_fix_quality fix_quality;

};





/* lat/long in MILLIONTHs of a degree and age of fix in milliseconds */
void gps2_get_position(struct gps2 *dev, float *lat, float *lon, int64_t *fix_age) {
  *lat = dev->position.lat;
  *lon = dev->position.lon;
  *fix_age = mgos_uptime_micros() - dev->position.timestamp;
}


/* date in last full GPRMC sentence */
void gps2_get_datetime(struct gps2 *dev, int *year, int *month, int *day, int *hours, int *minutes, int *seconds, int *microseconds, int64_t *age ) {
  *year = dev->datetime.year;
  *month = dev->datetime.month;
  *day = dev->datetime.day;
  *hours = dev->datetime.hours;
  *minutes = dev->datetime.minutes;
  *seconds = dev->datetime.seconds;
  *microseconds = dev->datetime.microseconds;
  *age = mgos_uptime_micros() - dev->datetime.timestamp;
}
 
/* speed in last full GPRMC sentence in 100ths of a knot */
void gps2_get_speed(struct gps2 *dev, double *speed, int64_t *age) {
  *speed = dev->speed.speed;
  *age = mgos_uptime_micros()  - dev->speed.timestamp;
}
 
/* course in last full GPRMC sentence in 100th of a degree */
void gps2_get_course(struct gps2 *dev, double *course, int64_t *age) {
  *course = dev->course.course;
  *age = mgos_uptime_micros()  - dev->course.timestamp;
}

/* satellites used in last full GPGGA sentence */
void gps2_get_satellites(struct gps2 *dev, int *satellites_tracked, int64_t *age) {
  *satellites_tracked = dev->satellites.satellites_tracked;
  *age = mgos_uptime_micros()  - dev->satellites.timestamp;

}

/* fix quality in last full GPGGA sentence */
void gps2_get_fix_quality(struct gps2 *dev, int *fix_quality, int64_t *age) {
  *fix_quality = dev->fix_quality.fix_quality;
  *age = mgos_uptime_micros()  - dev->fix_quality.timestamp;

}


void process_rmc_frame(struct gps2 *gps_dev, struct minmea_sentence_rmc rmc_frame) {

  LOG(LL_DEBUG,("Processing RMC frame"));
  gps_dev->position.lat = minmea_tocoord(&rmc_frame.latitude);
  gps_dev->position.lon = minmea_tocoord(&rmc_frame.longitude);
  gps_dev->position.timestamp = mgos_uptime_micros();
  /* speed */
  gps_dev->speed.speed = minmea_tofloat(&rmc_frame.speed);
  gps_dev->speed.timestamp = mgos_uptime_micros();
  /* course */
  gps_dev->course.course = minmea_tofloat(&rmc_frame.course);
  gps_dev->course.timestamp = mgos_uptime_micros();
  /* date time */
  gps_dev->datetime.day = rmc_frame.date.day;
  gps_dev->datetime.month = rmc_frame.date.month;
  gps_dev->datetime.year = rmc_frame.date.year;
  gps_dev->datetime.hours = rmc_frame.time.hours;
  gps_dev->datetime.minutes = rmc_frame.time.minutes;
  gps_dev->datetime.seconds = rmc_frame.time.seconds;
  gps_dev->datetime.microseconds = rmc_frame.time.microseconds;
  gps_dev->datetime.timestamp = mgos_uptime_micros();

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
  gps_dev->position.lat = minmea_tocoord(&gga_frame.latitude);
  gps_dev->position.lon = minmea_tocoord(&gga_frame.longitude);
  gps_dev->position.timestamp = mgos_uptime_micros();

   /* satellites tracked */
  gps_dev->satellites.satellites_tracked = gga_frame.satellites_tracked;
  gps_dev->satellites.timestamp = mgos_uptime_micros();

    /* fix quality */
  previous_fix_quality = gps_dev->fix_quality.fix_quality;
  gps_dev->fix_quality.fix_quality = gga_frame.fix_quality;



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



// NMEA strings end CR LF i.e. "\n"
// see https://en.wikipedia.org/wiki/NMEA_0183

// take a look at example-uart-c uart_dispatcher. Note the comment:
/*
 * Dispatcher can be invoked with any amount of data (even none at all) and
 * at any time. Here we demonstrate how to process input line by line.
 * 
 * It looks like mgos_uart_read_mbuf appends the read to the _end_ of the
 * buffer. mbuf_remove then removes the string from the buffer.
 * 
 * So I think we need to look for a "\n", copy that 
 * 
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

    // check that we've got the correct uart
    assert(gps_dev->uart_no == uart_no);

    // find out how many bytes are available to read
    rx_available = mgos_uart_read_avail(uart_no);

    // if we've got something to read, process it now
    if (rx_available > 0) {
      

      gps2_uart_rx_callback(uart_no,gps_dev,rx_available);
      
    }
 

}

// this has followed the pattern of mgos_fingerprint
// see https://github.com/mongoose-os-libs/fingerprint
void gps2_config_set_default(struct gps2_cfg *cfg) {
  // we haven't got any defaults to set in this version
  
}


struct gps2 *gps2_create_uart(
        struct gps2_cfg *cfg) {

    struct gps2 *gps_dev = calloc(1, sizeof(struct gps2));
    struct mgos_uart_config ucfg;
    
    struct mbuf *uart_rx_buffer = calloc(1, sizeof(struct mbuf));

    

    // if we didn't get a conf or we didn't allocate
    // memory for our device, return null

    if (!gps_dev || !cfg) return NULL;

    gps_dev->uart_no = cfg->uart_no;
    gps_dev->handler = cfg->handler;
    gps_dev->handler_user_data = cfg->handler_user_data;

    // initialize UART
    
    mgos_uart_config_set_defaults(gps_dev->uart_no, &ucfg);
    ucfg.baud_rate = cfg->uart_baud_rate;

    // these are all hard coded. The alternative would be
    // to define the UART in the calling client. 
    ucfg.num_data_bits = 8;
    ucfg.parity = MGOS_UART_PARITY_NONE;
    ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
    ucfg.rx_buf_size = 512;
    ucfg.tx_buf_size = 128;
    

    /* initialize a line buffer */
    
    mbuf_init(uart_rx_buffer,512);



    
    /* and set out gps_Dev to point to it */
    gps_dev->uart_rx_buffer = uart_rx_buffer;

    
    if (!mgos_uart_configure(gps_dev->uart_no, &ucfg)) goto err;
    

    LOG(LL_INFO, ("UART%d initialized %u,%d%c%d", gps_dev->uart_no, ucfg.baud_rate,
                ucfg.num_data_bits,
                ucfg.parity == MGOS_UART_PARITY_NONE ? 'N' : ucfg.parity + '0',
                ucfg.stop_bits));

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


enum mgos_init_result mgos_gps2_init(void) {
  
  return true;
  
}