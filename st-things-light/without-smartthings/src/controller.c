/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
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

#include <tizen.h>
#include <service_app.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <Ecore.h>

#include "log.h"
#include "resource/resource_infrared_motion_sensor.h"
#include "resource/resource_led.h"

// Duration for a timer
#define TIMER_GATHER_INTERVAL (5.0f)

// Motion sensor info
#define SENSOR_MOTION_GPIO_NUMBER (46)

// LED info
#define SENSOR_LED_GPIO_NUMBER (130)

typedef struct app_data_s {
	Ecore_Timer *getter_timer;
} app_data;
static app_data *g_ad = NULL;

static int _set_led(int on)
{
	int ret = 0;

	ret = resource_write_led(SENSOR_LED_GPIO_NUMBER, on);
	if (ret != 0) {
		_E("cannot write led data");
		return -1;
	}

	return 0;
}

static Eina_Bool _motion_to_led(void *user_data)
{
	int ret = 0;
	uint32_t value = 0;

	ret = resource_read_infrared_motion_sensor(SENSOR_MOTION_GPIO_NUMBER, &value);
	if (ret != 0) {
		_E("cannot read data from the infrared motion sensor");
		return ECORE_CALLBACK_CANCEL;
	}

	ret = _set_led((int)value);
	if (ret != 0) {
		_E("cannot write led data");
		return ECORE_CALLBACK_CANCEL;
	}
	_I("LED : %u", value);

	return ECORE_CALLBACK_RENEW;
}

static void _stop_gathering(void *data)
{
	app_data *ad = data;
	ret_if(!ad);

	if (ad->getter_timer) {
		ecore_timer_del(ad->getter_timer);
		ad->getter_timer = NULL;
	}
}

static void _start_gathering(void *data)
{
	app_data *ad = data;
	ret_if(!ad);

	if (ad->getter_timer)
		ecore_timer_del(ad->getter_timer);

	ad->getter_timer = ecore_timer_add(TIMER_GATHER_INTERVAL, _motion_to_led, ad);
	if (!ad->getter_timer)
		_E("Failed to add a timer");
}

static bool service_app_create(void *user_data)
{
	return true;
}

static void service_app_control(app_control_h app_control, void *user_data)
{
	_stop_gathering(user_data);
	_start_gathering(user_data);
}

static void service_app_terminate(void *user_data)
{
	app_data *ad = user_data;
	int ret = 0;

	// Turn off LED light with __set_led()
	ret = resource_write_led(SENSOR_LED_GPIO_NUMBER, 0);
	if (ret != 0) {
		_E("cannot write led data");
	}

	// Close Motion and LED resources
	resource_close_infrared_motion_sensor();
	resource_close_led();

	// Free app data
	free(ad);

	FN_END;
}

int main(int argc, char *argv[])
{
	app_data *ad = NULL;
	service_app_lifecycle_callback_s event_callback;

	ad = calloc(1, sizeof(app_data));
	retv_if(!ad, -1);

	g_ad = ad;

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	return service_app_main(argc, argv, &event_callback, ad);
}
