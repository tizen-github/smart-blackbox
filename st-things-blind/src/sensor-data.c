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

#include <stdlib.h>
#include <pthread.h>
#include "log.h"
#include "sensor-data.h"

struct __sensor_data_s {
	sensor_data_type_e type;
	union {
		int int_val;
		unsigned int uint_val;
		bool b_val;
		double d_val;
		char *str_val;
	} value;
	pthread_mutex_t mutex;
};

sensor_data *sensor_data_new(sensor_data_type_e type)
{
	sensor_data *data = NULL;
	retv_if(type == SENSOR_DATA_TYPE_NONE, NULL);

	data = calloc(1, sizeof(sensor_data));
	retv_if(!data, NULL);

	data->type = type;
	pthread_mutex_init(&data->mutex, NULL);

	return data;
}

void sensor_data_free(sensor_data *data)
{
	ret_if(!data);

	if (data->type == SENSOR_DATA_TYPE_STR) {
		pthread_mutex_lock(&data->mutex);
		free(data->value.str_val);
		data->value.str_val = NULL;
		pthread_mutex_unlock(&data->mutex);
	}
	pthread_mutex_destroy(&data->mutex);

	free(data);
}

int sensor_data_set_int(sensor_data *data, int value)
{
	retv_if(!data, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_INT, -1);

	pthread_mutex_lock(&data->mutex);
	data->value.int_val = value;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_set_uint(sensor_data *data, unsigned int value)
{
	retv_if(!data, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_UINT, -1);

	pthread_mutex_lock(&data->mutex);
	data->value.uint_val = value;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_set_bool(sensor_data *data, bool value)
{
	retv_if(!data, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_BOOL, -1);

	pthread_mutex_lock(&data->mutex);
	data->value.b_val = value;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_set_double(sensor_data *data, double value)
{
	retv_if(!data, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_DOUBLE, -1);

	pthread_mutex_lock(&data->mutex);
	data->value.d_val = value;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_set_string(sensor_data *data, const char *value, unsigned int size)
{
	char *temp = NULL;
	retv_if(!data, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_STR, -1);
	retv_if(!value, -1);
	retv_if(size == 0, -1);

	temp = strndup(value, size);
	retv_if(!temp, -1);

	pthread_mutex_lock(&data->mutex);
	free(data->value.str_val);
	data->value.str_val = temp;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_get_int(sensor_data *data, int *value)
{
	retv_if(!data, -1);
	retv_if(!value, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_INT, -1);

	pthread_mutex_lock(&data->mutex);
	*value = data->value.int_val;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_get_uint(sensor_data *data, unsigned int *value)
{
	retv_if(!data, -1);
	retv_if(!value, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_UINT, -1);

	pthread_mutex_lock(&data->mutex);
	*value = data->value.uint_val;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_get_bool(sensor_data *data, bool *value)
{
	retv_if(!data, -1);
	retv_if(!value, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_BOOL, -1);

	pthread_mutex_lock(&data->mutex);
	*value = data->value.b_val;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_get_double(sensor_data *data, double *value)
{
	retv_if(!data, -1);
	retv_if(!value, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_DOUBLE, -1);

	pthread_mutex_lock(&data->mutex);
	*value = data->value.d_val;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int sensor_data_get_string(sensor_data *data, const char **value)
{
	retv_if(!data, -1);
	retv_if(!value, -1);
	retv_if(data->type != SENSOR_DATA_TYPE_STR, -1);

	pthread_mutex_lock(&data->mutex);
	*value = data->value.str_val;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}
