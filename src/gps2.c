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

void gps2_uart_dispatcher(int uart_no, void *arg){
    struct gps2 *gps_dev;

    gps_dev = arg;

    // check that we've got the create uart
    assert(gps_dev->uart_no == uart_no);



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
    ucfg.baud_rate = cfg->uart_no;

    // these are all hard coded. The alternative would be
    // to define the UART in the calling client. 
    ucfg.num_data_bits = 8;
    ucfg.parity = MGOS_UART_PARITY_NONE;
    ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
    ucfg.rx_buf_size = 512;
    ucfg.tx_buf_size = 128;

    if (!mgos_uart_configure(gps_dev->uart_no, &ucfg)) goto err;
    mgos_uart_set_rx_enabled(gps_dev->uart_no, true);

    LOG(LL_INFO, ("UART%d initialized %u,%d%c%d", gps_dev->uart_no, ucfg.baud_rate,
                ucfg.num_data_bits,
                ucfg.parity == MGOS_UART_PARITY_NONE ? 'N' : ucfg.parity + '0',
                ucfg.stop_bits));

    mgos_uart_set_dispatcher(gps_dev->uart_no,gps2_uart_dispatcher,gps_dev);


    LOG(LL_INFO, ("Initialized GPS module"));
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