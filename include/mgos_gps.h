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


#ifdef __cplusplus
extern "C" {
#endif

struct mgos_gps;

/*
latitude - latitude in degrees (double)
longitude - longitude in degrees (double)
date - raw date in DDMMYY format (u32)
time - raw time in HHMMSSCC format (u32)
speed - raw speed in 100ths of a knot (i32)
course - raw course in 100ths of a degree (i32)
altitude - altitude in metres (double)
satellites - number of satellites in use (u32)
hdop - horizontal dimension of precision in 100ths (i32)
*/
struct mgos_gps_reading {
    double latitude;
    double longitude;
    int date;
    int time;
    int speed;
    int course;
    double altitude;
    int satellites;
    int hdop;

}

struct mgos_gps *mgos_gps_create(void);

void mgos_gps_destroy(struct mgos_gps **gps);

bool mgos_gps_get(struct mgos_gps *gps, struct mgos_gps_reading *gps_reading)

#ifdef __cplusplus
}
#endif
