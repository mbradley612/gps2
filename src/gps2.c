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

void parseNmeaString(char *bufferLine, struct gps2 *gps_dev) {
  char nmeaLine[MINMEA_MAX_LENGTH];

  // this all seems quite inefficient. Lots of copying of strings.

  // now parse the sentence
  enum minmea_sentence_id sentence_id = minmea_sentence_id(bufferLine, false);

  // and log the result
  LOG(LL_DEBUG,("NMEA sentence id: %d",sentence_id));

  (void)nmeaLine;

}



// NMEA strings end CR LF i.e. "\n"
// see https://en.wikipedia.org/wiki/NMEA_0183

void gps2_uart_dispatcher(int uart_no, void *arg){
    struct gps2 *gps_dev;
    size_t rx_available;
    struct mbuf rx_buffer;
    
    gps_dev = arg;

    // check that we've got the create uart
    assert(gps_dev->uart_no == uart_no);

    // find out how many bytes are available to read
    rx_available = mgos_uart_read_avail(uart_no);

    // if there's nothing to read, return now
    if (rx_available ==0) {
      return;
    }

    // initialize the buffer
    mbuf_init(&rx_buffer,rx_available);

    // read the buffer
    mgos_uart_read_mbuf(uart_no,&rx_buffer,rx_available);

    // if we've got anything in the buffer, tokenize on "\n"
    if (rx_buffer.len > 0)
    {
      char *buffer_line;
      buffer_line = strtok(rx_buffer.buf, "\n");
      while (buffer_line != NULL)
      {
        LOG(LL_DEBUG,("GPS lineNmea: %s", buffer_line));
        parseNmeaString(buffer_line, gps_dev);
      }
    }
    // 
    mbuf_free(&rx_buffer);

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

    if (!mgos_uart_configure(gps_dev->uart_no, &ucfg)) goto err;
    
    LOG(LL_DEBUG,("Debug logging statament"));

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