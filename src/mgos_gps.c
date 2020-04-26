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
void parseGpsData(char *line, struct mgos_gps *gps)
{
    struct minmea_reading * latest_nmea_reading;

    latest_nmea_reading = gps->latest_reading;



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
         
            latest_nmea_reading->longitude = frame.longitude;
            latest_nmea_reading->latitude = frame.latitude;
            latest_nmea_reading->speed = frame.speed;
            latest_nmea_reading->course = frame.course;
            latest_nmea_reading->date = frame.date;
            latest_nmea_reading->time = frame.time;
            latest_nmea_reading->variation = frame.variation;
            
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
    // we just take the satellites tracked and fix quality
    // from the GGA sentence.
    {
        struct minmea_sentence_gga frame;
        if (minmea_parse_gga(&frame, lineNmea))
        {
            latest_nmea_reading->satellites_tracked = frame.satellites_tracked;
            latest_nmea_reading->fix_quality = frame.fix_quality;
            
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

/*
* This is the callback from the timer. In its current form, 
*/
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

// this callback is called whenever there is data in the UART input buffer or
// space available in the UART output buffer.

// the callback updates the dataAvailable field in the gps structure that manages
// the state of this GPS device.

void uart_dispatcher_cb(int uart_no, void *arg)
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


bool mgos_gps_setup(struct mgos_gps * gps, int uart_no, int baud_rate, int update_interval)
{

    
    struct mgos_uart_config ucfg;

    gps->uart_no = uart_no;
    gps->update_interval = update_interval;
    gps->ucfg->baud_rate = baud_rate;

    ucfg = * gps->ucfg;

    mgos_uart_config_set_defaults(uart_no, &ucfg);

    gps->ucfg->num_data_bits = 8;
    gps->ucfg->parity = MGOS_UART_PARITY_NONE;
    gps->ucfg->stop_bits = MGOS_UART_STOP_BITS_1;
    if (!mgos_uart_configure(gps->uart_no, &ucfg))
    {
        return false;
    }

    // create a mgos timer to read from the UART. We pass our gps structure as a parameter to the
    // callback.

    mgos_set_timer(gps->update_interval /* ms */, true /* repeat */, gps_read_cb, gps /* arg */);

    // configure the UART dispatcher, gets called when there is data in the input buffer or space 
    // available in the output buffer. Our callback reads the data in the buffer. The gps structure
    // is passed as an argument to the callback.

    // see https://mongoose-os.com/docs/mongoose-os/api/core/mgos_uart.h.md

    mgos_uart_set_dispatcher(gps->uart_no, uart_dispatcher_cb, gps /* arg */);
    mgos_uart_set_rx_enabled(gps->uart_no, true);

    return true;
}





// Private functions end

// Public functions follow


// may need an internal structure to store stuff.
struct mgos_gps *mgos_gps_create(int uart_no, int baud_rate, int update_interval) {
  struct mgos_gps *gps;

  gps = calloc(1, sizeof(struct mgos_gps));
  if (!gps) {
    return NULL;
  }
  memset(gps, 0, sizeof(struct mgos_gps));

  gps->uart_no = uart_no;

  gps->latest_reading = calloc(1, sizeof(struct minmea_reading));
  if (!gps->latest_reading) {
      return NULL;
  }
  memset(gps->latest_reading,0,sizeof(struct minmea_reading));

  mgos_gps_setup(gps, uart_no, baud_rate, update_interval);

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


bool mgos_gps_get(struct mgos_gps *gps, struct mgos_gps_reading *latest_gps_reading) {

    struct minmea_reading * latest_nmea_reading;

    latest_nmea_reading = gps->latest_reading;


    latest_gps_reading->longitude = latest_nmea_reading->longitude.value;
    latest_gps_reading->latitude = latest_nmea_reading->latitude.value;
    latest_gps_reading->satellites_tracked = latest_nmea_reading->satellites_tracked;
    latest_gps_reading->speed = latest_nmea_reading->speed.value;
    latest_gps_reading->fix_quality = latest_nmea_reading->fix_quality;

    return false;
   
}




bool mgos_gps2_init(void) {
  return true;
}