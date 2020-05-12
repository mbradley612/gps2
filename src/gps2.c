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
#include "gps2.h"

#include "minmea.h"

struct latest_gps_info {
  int satellites_tracked;
};


struct gps2 {
  uint8_t uart_no;
  gps2_ev_handler handler; //shouldn't be here
  void *handler_user_data; // shouldn't be here
  struct latest_gps_info latest_info;
  struct mbuf *uart_rx_buffer; 
  

};

void parseNmeaString(struct mg_str line, struct gps2 *gps_dev) {
  


  // now parse the sentence
  /*
  enum minmea_sentence_id sentence_id = minmea_sentence_id(lineNmea, false);
  */
  // and log the result
  /*
  LOG(LL_DEBUG,("NMEA sentence id: %s",line.p));
*/
  LOG(LL_DEBUG,("NMEA sentence: %s",line.p));
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



    LOG(LL_DEBUG,("Buffer address is %x",(int)gps_dev->uart_rx_buffer)); 

    LOG(LL_DEBUG,("Buffer address is %x",(int)(*gps_dev).uart_rx_buffer));

    // check the size of our buffer
    /*
    LOG(LL_DEBUG,("Buffer address is %x, size is %d, buffer length is %d, rx available is: %d",
      (size_t)gps_dev->uart_rx_buffer,
      *(gps_dev->uart_rx_buffer).size, 
      *(gps_dev->uart_rx_buffer).len,
      rx_available));
    */

   struct mbuf *uart_rx_buffer;

   uart_rx_buffer = gps_dev->uart_rx_buffer;


   LOG(LL_DEBUG,("Buffer address is %x, buffer size is %d",
      (int)&uart_rx_buffer,
      gps_dev->uart_rx_buffer->size));


    LOG(LL_DEBUG,("UART no is %d, rx_available is: %d",uart_no, rx_available));
    
    

    // read the UART into our line buffer.
    mgos_uart_read_mbuf(uart_no,gps_dev->uart_rx_buffer,rx_available);
    LOG(LL_DEBUG,("Successfully read the UART buffer into our rx buffer"));
    
    

    // output the contents of the buffer

    LOG(LL_DEBUG,("Buffer contents is %s",mg_mk_str_n(gps_dev->uart_rx_buffer->buf,gps_dev->uart_rx_buffer->len).p));
    return;


    size_t line_length;
    char *terminator_ptr;
  
    

    // if we've got anything in the buffer, look for the first "\n"
    if (gps_dev->uart_rx_buffer->len > 0)
    {
      
     

      terminator_ptr = strstr(gps_dev->uart_rx_buffer->buf,"\n");
      
      while (terminator_ptr != NULL)
      {
        LOG(LL_DEBUG,("We've got a string to read"));

        /* calculate the length of the line using pointer arithmetic */
        line_length = terminator_ptr - gps_dev->uart_rx_buffer->buf;

        /* create the string of the line*/
        //struct mg_str line = mg_mk_str_n(uart_rx_buffer.buf, line_length);
        
        /* parse the line */
        //parseNmeaString(line, gps_dev);

        /* free the mg_str struct for the line */
        
        //mg_strfree(&line);
        
        /* remove the line from the beginning of the buffer */
        // mbuf_remove(&uart_rx_buffer, line_length);
        // terminator_ptr = strstr(uart_rx_buffer.buf,"\n");
      }
    }
    // 

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



    LOG(LL_DEBUG,("On initialization, buffer address is %x, size is %d, buffer length is %d",
      (int)uart_rx_buffer,
      uart_rx_buffer->size, 
      uart_rx_buffer->len));
    /* and set out gps_Dev to point to it */
    gps_dev->uart_rx_buffer = uart_rx_buffer;

    size_t uart_rx_buffer_ptr;

    uart_rx_buffer_ptr = (size_t)gps_dev->uart_rx_buffer;

    LOG(LL_DEBUG,("And buffer address is now %x",
      (size_t)uart_rx_buffer_ptr));

    
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