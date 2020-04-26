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

#pragma once

#include "mgos.h"
#include "minmea.h"

#ifdef __cplusplus
extern "C" {
#endif

struct minmea_reading;

struct mgos_gps {
    int                             uart_no;
    int                             baud_rate; 
    int                             update_interval;
    struct mgos_uart_config *       ucfg;
    int                             dataAvailable;
    struct nmea_reading *           latest_reading;
    void *                          user_data;
};

/*
The fields in this structure are populated with the values from the MNEA sentences as they are parsed.
*/

// needs a boolean or booleans to indicate if values populated.
struct mgos_gps_reading {
    float latitude;
     float longitude;
     int date;
     int time;
     float speed;
     float course;
     float altitude;
    int satellites_tracked; 
    float variation;
    int fix_quality;

};



struct mgos_gps *mgos_gps_create(int uart_no, int baud_rate, int update_interval);

void mgos_gps_destroy(struct mgos_gps **gps);

// populates the structure latest_gps_reading with the latest values read from the
// GPS module.

bool mgos_gps_get(struct mgos_gps *gps, struct mgos_gps_reading *latest_gps_reading);

bool mgos_gps2_init(void);

#ifdef __cplusplus
}
#endif
