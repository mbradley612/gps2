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


/*
The fields in this structure are updated with the latest readings from the NMEA sentences as
thye come in.
*/
struct mgos_gps_reading {
    struct minmea_float latitude;
    struct minmea_float longitude;
    struct minmea_date date;
    struct minmea_time time;
    struct minmea_float speed;
    struct minmea_float course;
    struct minmea_float altitude; char altitude_units;
    int satellites_tracked; 
    struct minmea_float variation;
    int fix_quality;

};

struct mgos_gps *mgos_gps_create(int uart_no, int baud_rate, int update_interval);

void mgos_gps_destroy(struct mgos_gps **gps);

bool mgos_gps_get(struct mgos_gps *gps, struct mgos_gps_reading *latest_gps_reading);

bool mgos_gps2_init(void);

#ifdef __cplusplus
}
#endif
