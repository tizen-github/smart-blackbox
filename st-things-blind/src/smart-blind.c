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

#include <tizen.h>
#include <service_app.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <Ecore.h>

#include "st_things.h"
#include "log.h"
#include "sensor-data.h"
#include "resource.h"

#define JSON_PATH "device_def.json"

#define SENSOR_URI_ILLUMINANCE "/capability/illuminanceMeasurement/main/0"
#define SENSOR_KEY_ILLUMINANCE "illuminance"
#define SENSOR_KEY_RANGE "range"

#define SENSOR_URI_DOOR "/capability/doorControl/main/0"
#define SENSOR_KEY_DOOR "doorState"
#define SENSOR_POWER_INITIALIZING BLIND_DOWN

#define I2C_BUS_NUMBER (1)
#define SENSOR_GATHER_INTERVAL (5.0f)

// If you want to use SmartThings, Please uncomment this define */
//#define USE_ST_SDK

typedef struct app_data_s {
	Ecore_Timer *getter_illuminance;
	sensor_data *illuminance_data;
	sensor_data *power_data;
} app_data;

static app_data *g_ad = NULL;

#define ILLUMINATION_CRITERIA 1000

// HS-53 Servo Motor Duty Cycle : 0.54ms ~ 2.1ms
// Spec Duty Cycle : 0.553ms ~ 2.227ms(https://www.servocity.com/hitec-hs-53-servo)
#define SERVO_MOTOR_DUTY_CYCLE_COUNTER_CLOCKWISE 1.0
#define SERVO_MOTOR_DUTY_CYCLE_CLOCKWISE 2.0

#define BLIND_UP "opening"
#define BLIND_DOWN "closing"

static inline int __get_illuminance(void *data, unsigned int *illuminance_value)
{
	int ret = 0;
	app_data *ad = data;

	retv_if(!ad, -1);
	retv_if(!ad->illuminance_data, -1);

	ret = resource_read_illuminance_sensor(I2C_BUS_NUMBER, illuminance_value);
	retv_if(ret != 0, -1);

	sensor_data_set_uint(ad->illuminance_data, *illuminance_value);
	_D("Illuminance value : %u", *illuminance_value);

#ifdef USE_ST_SDK
	st_things_notify_observers(SENSOR_URI_ILLUMINANCE);
#endif

	return 0;
}

static int __set_servo_motor(void *data, int on)
{
	double duty_cycle = 0;
	int ret = 0;
	const char *power_value = NULL;
	app_data *ad = data;

	retv_if(!ad, -1);
	retv_if(!ad->illuminance_data, -1);

	if (on) {
		duty_cycle = SERVO_MOTOR_DUTY_CYCLE_CLOCKWISE;
		power_value = BLIND_UP;
	} else {
		duty_cycle = SERVO_MOTOR_DUTY_CYCLE_COUNTER_CLOCKWISE;
		power_value = BLIND_DOWN;
	}

	ret = resource_set_servo_motor_value(duty_cycle);
	retv_if(ret != 0, -1);

	sensor_data_set_string(ad->power_data, power_value, strlen(power_value));

#ifdef USE_ST_SDK
	st_things_notify_observers(SENSOR_URI_DOOR);
#endif

	return 0;
}

static Eina_Bool __illuminance_to_servo_motor(void *data)
{
	int ret = 0;
	unsigned int illuminance_value = 0;

	app_data *ad = data;

	if (!ad) {
		_E("failed to get app_data");
		service_app_exit();
	}

	if (!ad->illuminance_data) {
		_E("failed to get illuminance_data");
		service_app_exit();
	}

	ret = __get_illuminance(ad, &illuminance_value);
	retv_if(ret != 0, ECORE_CALLBACK_RENEW);

#if 1 // If you want to control MOTOR from Cloud, please deactivate these codes.
	int on = 0;

	if (illuminance_value < ILLUMINATION_CRITERIA) {
		on = 0;
	} else {
		on = 1;
	}

	ret = __set_servo_motor(ad, on);
	retv_if(ret != 0, ECORE_CALLBACK_RENEW);
#endif

	return ECORE_CALLBACK_RENEW;
}

void gathering_stop(void *data)
{
	app_data *ad = data;

	ret_if(!ad);

	if (ad->getter_illuminance)
		ecore_timer_del(ad->getter_illuminance);
}

void gathering_start(void *data)
{
	app_data *ad = data;

	ret_if(!ad);

	gathering_stop(ad);

	ad->getter_illuminance = ecore_timer_add(SENSOR_GATHER_INTERVAL, __illuminance_to_servo_motor, ad);
	if (!ad->getter_illuminance)
		_E("Failed to add getter_illuminance");
}

#ifdef USE_ST_SDK
static bool handle_reset_request(void)
{
	_D("Received a request for RESET.");
	return false;
}

static void handle_reset_result(bool result)
{
	_D("Reset %s.\n", result ? "succeeded" : "failed");
}

static bool handle_ownership_transfer_request(void)
{
	_D("Received a request for Ownership-transfer.");
	return true;
}

static void handle_things_status_change(st_things_status_e things_status)
{
	_D("Things status is changed: %d", things_status);

	if (things_status == ST_THINGS_STATUS_REGISTERED_TO_CLOUD) {
		ecore_main_loop_thread_safe_call_async(gathering_start, g_ad);
	}
}

static bool handle_get_request(st_things_get_request_message_s* req_msg, st_things_representation_s* resp_rep)
{
	_D("resource_uri [%s]", req_msg->resource_uri);
	retv_if(!g_ad, false);

	if (0 == strcmp(req_msg->resource_uri, SENSOR_URI_ILLUMINANCE)) {
		if (req_msg->has_property_key(req_msg, SENSOR_KEY_ILLUMINANCE)) {
			unsigned int value = 0;
			sensor_data_get_uint(g_ad->illuminance_data, &value);
			resp_rep->set_int_value(resp_rep, SENSOR_KEY_ILLUMINANCE, value);
		}
		return true;
	} else if (0 == strcmp(req_msg->resource_uri, SENSOR_URI_DOOR)) {
		if (req_msg->has_property_key(req_msg, SENSOR_KEY_DOOR)) {
			const char *str = NULL;
			sensor_data_get_string(g_ad->power_data, &str);
			if (!str) {
				str = SENSOR_POWER_INITIALIZING;
			}
			resp_rep->set_str_value(resp_rep, SENSOR_KEY_DOOR, str);
			_D("Power : %s", str);
		}
		return true;
	}
	_E("not supported uri");
	return false;
}

static bool handle_set_request(st_things_set_request_message_s* req_msg, st_things_representation_s* resp_rep)
{
	_D("resource_uri [%s]", req_msg->resource_uri);
	retv_if(!g_ad, false);

		if (0 == strcmp(req_msg->resource_uri, SENSOR_URI_DOOR)) {
		int ret = 0;
		char *str = NULL;

		if (req_msg->rep->get_str_value(req_msg->rep, SENSOR_KEY_DOOR, &str)) {
			retv_if(!str, false);
			_D("set [%s:%s] == %s", SENSOR_URI_DOOR, SENSOR_KEY_DOOR, str);

			sensor_data_set_string(g_ad->power_data, str, strlen(str));
			resp_rep->set_str_value(resp_rep, SENSOR_KEY_DOOR, str);

			if (0 == strcmp(str, "opening")) {
				ret = __set_servo_motor(g_ad, 1);
			} else {
				ret = __set_servo_motor(g_ad, 0);
			}
			free(str);
			retv_if(ret != 0, false);
		} else {
			_E("cannot get a string value");
		}

		return true;
	}
	return false;
}

static int __st_things_init(void)
{
	bool easysetup_complete = false;
	char app_json_path[128] = {'\0', };
	char *app_res_path = NULL;
	char *app_data_path = NULL;

	app_res_path = app_get_resource_path();
	if (!app_res_path) {
		_E("app_res_path is NULL!!");
		return -1;
	}

	app_data_path = app_get_data_path();
	if (!app_data_path) {
		_E("app_data_path is NULL!!");
		free(app_res_path);
		return -1;
	}

	snprintf(app_json_path, sizeof(app_json_path), "%s%s", app_res_path, JSON_PATH);

	if (0 != st_things_set_configuration_prefix_path(app_res_path, app_data_path)) {
		_E("st_things_set_configuration_prefix_path() failed!!");
		free(app_res_path);
		free(app_data_path);
		return -1;
	}

	free(app_res_path);
	free(app_data_path);

	if (0 != st_things_initialize(app_json_path, &easysetup_complete)) {
		_E("st_things_initialize() failed!!");
		return -1;
	}

	_D("easysetup_complete:[%d] ", easysetup_complete);

	st_things_register_request_cb(handle_get_request, handle_set_request);
	st_things_register_reset_cb(handle_reset_request, handle_reset_result);
	st_things_register_user_confirm_cb(handle_ownership_transfer_request);
	st_things_register_things_status_change_cb(handle_things_status_change);

	return 0;
}

static int __st_things_deinit(void)
{
	st_things_deinitialize();
	return 0;
}

static int __st_things_start(void)
{
	st_things_start();
	return 0;
}

static int __st_things_stop(void)
{
	st_things_stop();
	return 0;
}
#endif /* USE_ST_SDK */

static bool service_app_create(void *user_data)
{
	app_data *ad = (app_data *)user_data;

	ad->illuminance_data = sensor_data_new(SENSOR_DATA_TYPE_UINT);
	if (!ad->illuminance_data)
		return false;

	ad->power_data = sensor_data_new(SENSOR_DATA_TYPE_STR);
	if (!ad->power_data)
		return false;
	sensor_data_set_string(g_ad->power_data, SENSOR_POWER_INITIALIZING, strlen(SENSOR_POWER_INITIALIZING));

#ifdef USE_ST_SDK
	if (__st_things_init())
		return false;
#endif

	return true;
}

static void service_app_control(app_control_h app_control, void *user_data)
{
#ifdef USE_ST_SDK
	__st_things_start();
#else
	gathering_start(user_data);
#endif
}

static void service_app_terminate(void *user_data)
{
	app_data *ad = (app_data *)user_data;

	resource_close_illuminance_sensor();
	resource_close_servo_motor();

#ifdef USE_ST_SDK
	__st_things_stop();
	__st_things_deinit();
#else
	gathering_stop(ad);
#endif

	sensor_data_free(ad->illuminance_data);
	free(ad);
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

