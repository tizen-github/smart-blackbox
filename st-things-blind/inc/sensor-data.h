/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * Contact: Jin Yoon <jinny.yoon@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SENSOR_DATA_H__
#define  __SENSOR_DATA_H__

#include <stdbool.h>

typedef enum {
	SENSOR_DATA_TYPE_NONE = 0,
	SENSOR_DATA_TYPE_INT,
	SENSOR_DATA_TYPE_UINT,
	SENSOR_DATA_TYPE_BOOL,
	SENSOR_DATA_TYPE_DOUBLE,
	SENSOR_DATA_TYPE_STR,
} sensor_data_type_e;

typedef struct __sensor_data_s sensor_data;

sensor_data *sensor_data_new(sensor_data_type_e type);
void sensor_data_free(sensor_data *data);

int sensor_data_set_int(sensor_data *data, int value);
int sensor_data_set_uint(sensor_data *data, unsigned int value);
int sensor_data_set_bool(sensor_data *data, bool value);
int sensor_data_set_double(sensor_data *data, double value);
int sensor_data_set_string(sensor_data *data, const char *value, unsigned int size);

int sensor_data_get_int(sensor_data *data, int *value);
int sensor_data_get_uint(sensor_data *data, unsigned int *value);
int sensor_data_get_bool(sensor_data *data, bool *value);
int sensor_data_get_double(sensor_data *data, double *value);
int sensor_data_get_string(sensor_data *data, const char **value);

#endif /* __SENSOR_DATA_H__ */
