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
#include "mgos_gps.h"
#include "mgos_gps_internal.h"

#include "minmea.h"

#include "common/mbuf.h"
#include "common/platform.h"
#include "mgos_app.h"
#include "mgos_gpio.h"
#include "mgos_timers.h"
#include "mgos_uart.h"
#include "common/json_utils.h"



// Private functions follow
static void parseGpsData(char *line, struct mgos_gps *gps)
{
    char lineNmea[MINMEA_MAX_LENGTH];
    strncpy(lineNmea, line, sizeof(lineNmea) - 1);
    strcat(lineNmea, "\n");
    lineNmea[sizeof(lineNmea) - 1] = '\0';

    enum minmea_sentence_id id = minmea_sentence_id(lineNmea, false);
    //printf("sentence id = %d from line %s\n", (int) id, lineNmea);
    switch (id)
    {
    case MINMEA_SENTENCE_RMC:
    {
        struct minmea_sentence_rmc frame;
        if (minmea_parse_rmc(&frame, lineNmea))
        {
            gps->lastFrame = &frame;
            /*
      printf("$RMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",
             frame.latitude.value, frame.latitude.scale,
             frame.longitude.value, frame.longitude.scale,
             frame.speed.value, frame.speed.scale);
      printf("$RMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\n",
             minmea_rescale(&frame.latitude, 1000),
             minmea_rescale(&frame.longitude, 1000),
             minmea_rescale(&frame.speed, 1000));
      printf("$RMC floating point degree coordinates and speed: (%f,%f) %f\n",
             minmea_tocoord(&frame.latitude),
             minmea_tocoord(&frame.longitude),
             minmea_tofloat(&frame.speed));
      */
        }
    }
    break;

    case MINMEA_SENTENCE_GGA:
    {
        struct minmea_sentence_gga frame;
        if (minmea_parse_gga(&frame, lineNmea))
        {
            printf("$GGA: fix quality: %d\n", frame.fix_quality);
        }
    }
    break;

    case MINMEA_SENTENCE_GSV:
    {
        struct minmea_sentence_gsv frame;
        if (minmea_parse_gsv(&frame, lineNmea))
        {
            //printf("$GSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
            printf("$GSV: sattelites in view: %d\n", frame.total_sats);
            /*for (int i = 0; i < 4; i++)
        printf("$GSV: sat nr %d, elevation: %d, azimuth: %d, snr: %d dbm\n",
               frame.sats[i].nr,
               frame.sats[i].elevation,
               frame.sats[i].azimuth,
               frame.sats[i].snr);
      */
        }
    }
    break;
    case MINMEA_INVALID:
    {
        break;
    }
    case MINMEA_UNKNOWN:
    {
        break;
    }
    case MINMEA_SENTENCE_GSA:
    {
        break;
    }
    case MINMEA_SENTENCE_GLL:
    {
        break;
    }
    case MINMEA_SENTENCE_GST:
    {
        break;
    }
    case MINMEA_SENTENCE_VTG:
    {
        break;
    }
    case MINMEA_SENTENCE_ZDA:
    {
        break;
    }
    }
}

static void gps_read_cb(void *arg)
{
    struct mgos_gps * gps = arg;


    if (gps->dataAvailable > 0)
    {
        struct mbuf rxb;
        mbuf_init(&rxb, 0);
        mgos_uart_read_mbuf(gps->uart_no, &rxb, gps->dataAvailable);
        if (rxb.len > 0)
        {
            char *pch;
            //printf("%.*s", (int) rxb.len, rxb.buf);
            pch = strtok(rxb.buf, "\n");
            while (pch != NULL)
            {
                //printf("GPS lineNmea: %s\n", pch);
                parseGpsData(pch,gps);
                pch = strtok(NULL, "\n");
            }
        }
        mbuf_free(&rxb);

        gps->dataAvailable = 0;
    }

    
}

int esp32_uart_rx_fifo_len(int uart_no);

static void uart_dispatcher(int uart_no, void *arg)
{
    struct mgos_gps * gps = arg;
    assert(uart_no == gps->uart_no);
    size_t rx_av = mgos_uart_read_avail(uart_no);
    if (rx_av > 0)
    {
        gps->dataAvailable = rx_av;
    }
    (void)arg;
}


bool mgos_gps_setup(struct mgos_gps * gps)
{

    
    struct mgos_uart_config ucfg;

    ucfg = * gps->ucfg;

    mgos_uart_config_set_defaults(gps->uart_no, &ucfg);

    gps->ucfg->baud_rate = mgos_sys_config_get_gps_baud_rate();
    gps->ucfg->num_data_bits = 8;
    //ucfg.parity = MGOS_UART_PARITY_NONE;
    //ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
    if (!mgos_uart_configure(gps->uart_no, &ucfg))
    {
        return false;
    }

    mgos_set_timer(mgos_sys_config_get_gps_update_interval() /* ms */, true /* repeat */, gps_read_cb, &gps /* arg */);

    mgos_uart_set_dispatcher(gps->uart_no, uart_dispatcher, &gps /* arg */);
    mgos_uart_set_rx_enabled(gps->uart_no, true);

    return true;
}

bool mgos_gps_init(void)
{
    return true;
}




// Private functions end

// Public functions follow


// may need an internal structure to store stuff.
struct mgos_gps *mgos_gps_create(int uart_no) {
  struct mgos_gps *gps;

  gps = calloc(1, sizeof(struct mgos_gps));
  if (!gps) {
    return NULL;
  }
  memset(gps, 0, sizeof(struct mgos_gps));

  gps->uart_no = uart_no;

  mgos_gps_setup(gps);

  return gps;
}

void mgos_imu_destroy(struct mgos_gps **gps) {
  // do any additional cleaning up here
  if (!*gps) {
    return;
  }
  
  if ((*gps)->user_data) {
    free((*gps)->user_data);
  }


  free(*gps);
  *gps = NULL;
  return;
}


