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
  unsigned long lat;
  unsigned long lon;
  unsigned long timestamp;
};

struct gps_datetime {
  unsigned long date;
  unsigned long time;
  unsigned long timestamp;
};

struct gps_speed {
  unsigned long speed;
  unsigned long timestamp;
};

struct gps_course {
  unsigned long course;
  unsigned long timestamp;
};

struct gps_satellites {
  short satellites;
  unsigned long timestamp;
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

};

void process_rmc_frame(struct gps2 *gps_dev, struct minmea_sentence_rmc rmc_frame) {
  gps_dev->position.lat = rmc_frame.latitude.value;
  gps_dev->position.lon = rmc_frame.longitude.value;
  gps_dev->position.timestamp = mgos_uptime() / 1000;
  /* speed */
  /* course */
  /* date */
}

void parseNmeaString(struct mg_str line, struct gps2 *gps_dev) {
  

  enum minmea_sentence_id sentence_id;
  
  /* parse the sentence */
  sentence_id = minmea_sentence_id(line.p, false);

  switch (sentence_id) {
    case MINMEA_SENTENCE_RMC: {
      struct minmea_sentence_rmc frame;
      if (minmea_parse_rmc(&frame, line)) {
        process_rmc_frame(gps_dev, frame);
      }
    }
  }
  
  if (sentence_id) {
    LOG(LL_DEBUG,("NMEA sentence id: %d",sentence_id));
  } else {
    LOG(LL_DEBUG,("NMEA library failed to parse sentence"));
  }
  (void)gps_dev;

}


/* lat/long in MILLIONTHs of a degree and age of fix in milliseconds */
void gps2_get_position(struct gps2 *dev, unsigned long lat, unsigned long lon, unsigned long fix_age);
 
/* date as ddmmyy, time as hhmmsscc, and age in milliseconds */
/* check, is there an mgos preferred way of handling date and time? */
void gps2_get_datetime(struct gps2 *dev, unsigned long date, unsigned long time, unsigned long age);
 
/* speed in last full GPRMC sentence in 100ths of a knot */
void gps2_speed(struct gps2 *dev, unsigned long speed, unsigned long age);
 
/* course in last full GPRMC sentence in 100th of a degree */
void gps2_course(struct gps2 *dev, unsigned long course, unsigned long age);

/* satellites used in last full GPGGA sentence */
void gps2_satellites(struct gps2 *dev, unsigned long course, unsigned long age);



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

    size_t uart_rx_buffer_ptr;

    uart_rx_buffer_ptr = (size_t)gps_dev->uart_rx_buffer;


    
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
  if (!mgos_event_register_base(EVENT_GRP_GPS, "gps")) {
    return MGOS_INIT_OK;
  }
  return MGOS_INIT_APP_INIT_FAILED;
}