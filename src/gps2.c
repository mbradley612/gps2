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
  void *handler_user_data; 

  struct mbuf *uart_rx_buffer;
  struct mbuf *uart_tx_buffer; 
  struct mgos_uart_config  uart_config;

  struct mgos_gps_location latest_location;

};


static struct gps2 *global_gps_device;





void process_rmc_frame(struct gps2 *dev, struct minmea_sentence_rmc rmc_frame) {
  struct mgos_gps_location location;
  struct tm time;

  LOG(LL_DEBUG,("Processing RMC frame"));
  /* lon and lat */

  /* check we have a fix */
  if (rmc_frame.valid == true) {

    time.tm_year = rmc_frame.date.year + 100;
    time.tm_mon = rmc_frame.date.month - 1;
    time.tm_mday = rmc_frame.date.day;
    
    time.tm_hour = rmc_frame.time.hours;
    time.tm_min = rmc_frame.time.minutes;
    time.tm_sec = rmc_frame.time.seconds;
    

    location.time = mktime(&time);

    location.microseconds = rmc_frame.time.microseconds;


    location.longitude = minmea_tocoord(&(rmc_frame.longitude));
    location.latitude = minmea_tocoord(&(rmc_frame.latitude));
    location.speed = minmea_tofloat(&(rmc_frame.speed));    
    location.bearing = minmea_tofloat(&(rmc_frame.course));
    location.variation = minmea_tofloat(&(rmc_frame.variation));
    
    
    location.elapsed_time = mgos_uptime_micros();

    dev->latest_location = location;

    mgos_event_trigger(MGOS_EV_GPS_LOCATION,&location);

  }
}

void parseNmeaString(struct mg_str line, struct gps2 *gps_dev) {


  struct mgos_gps_nmea_sentence sentence;

  enum minmea_sentence_id sentence_id;

  
  /* parse the sentence */
  sentence_id = minmea_sentence_id(line.p, false);

  sentence.sentence_id = sentence_id;
  sentence.nmea_string = line.p;
  
  mgos_event_trigger(MGOS_EV_GPS_NMEA_SENTENCE, &sentence);

  

  switch (sentence_id) {
    case MINMEA_SENTENCE_RMC: {
      struct minmea_sentence_rmc frame;
      if (minmea_parse_rmc(&frame, line.p)) {
        process_rmc_frame(gps_dev, frame);
          /* fire the trigger for the RMC sentence */
          

      }
    } break;
    default: {
      /* do nothing */
      ;
    } break;
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
  uint8_t uart_no, struct mgos_uart_config *ucfg) {

    struct gps2 *gps_dev = calloc(1, sizeof(struct gps2));
      
    struct mbuf *uart_rx_buffer = calloc(1, sizeof(struct mbuf));
    struct mbuf *uart_tx_buffer = calloc(1, sizeof(struct mbuf));


    /* check we have a uart config. If not, return null */
    if (ucfg == NULL) {
      return NULL;
    }
    


    gps_dev->uart_no = uart_no;
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



    LOG(LL_INFO, ("Initialized GPS device"));
  
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
  ucfg.baud_rate  = mgos_sys_config_get_gps_uart_baud();
  ucfg.tx_buf_size = mgos_sys_config_get_gps_uart_tx_buffer_size();
  ucfg.rx_buf_size = mgos_sys_config_get_gps_uart_rx_buffer_size();

  global_gps_device = gps2_create_uart(uart_no, &ucfg);

  return global_gps_device;


}


/* get the global gps2 device. Returns null if creating the UART handler has failed */
struct gps2 *gps2_get_global_device() {
  return global_gps_device;
}




/* location including speed and course and age of fix in milliseconds 
   this is derived from the most recent RMC sentence*/
void mgos_gps_get_latest_location(struct mgos_gps_location *latest_location) {
  mgos_gps_device_get_latest_location(gps2_get_global_device(),latest_location);
};

void mgos_gps_device_get_latest_location(struct gps2 *dev, struct mgos_gps_location *latest_location) {
  latest_location->latitude = dev->latest_location.latitude;
  latest_location->longitude = dev->latest_location.longitude;
  latest_location->bearing = dev->latest_location.bearing;
  latest_location->speed = dev->latest_location.speed;
  latest_location->variation = dev->latest_location.variation;
  latest_location->time = dev->latest_location.time;
  latest_location->elapsed_time = dev->latest_location.elapsed_time; 
}


enum mgos_init_result mgos_gps2_init(void) {
  uint8_t gps_config_uart_no;
  uint8_t gps_config_uart_baud;

  mgos_event_register_base(MGOS_EV_GPS_BASE, __FILE__);

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