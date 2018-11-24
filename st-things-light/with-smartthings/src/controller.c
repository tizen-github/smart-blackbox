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
#define SENSOR_LED_ON "on"
#define SENSOR_LED_OFF "off"

#define USE_ST_SDK
#ifdef USE_ST_SDK
#include "smartthings.h"
#include "smartthings_resource.h"
#include "smartthings_payload.h"

// Cert file name in 'res' directory
#define CERT_FILE "certificate.pem"
// Private key file name in 'res' directory
#define PRIV_FILE "privatekey.der"

#define SENSOR_MOTION_URI "/capability/motionSensor/main/0"
#define SENSOR_MOTION_KEY "value"
#define SENSOR_LED_URI "/capability/switch/main/0"
#define SENSOR_LED_KEY "power"
#endif /* USE_ST_SDK */

typedef struct app_data_s {
	Ecore_Timer *getter_timer;
	uint32_t motion_data;
	int led_data;
#ifdef USE_ST_SDK
	smartthings_h st_master_h;
	smartthings_resource_h st_res_h;
	smartthings_status_e status;
#endif /* USE_ST_SDK */
} app_data;
static app_data *g_ad = NULL;

#ifdef USE_ST_SDK
static const char * _resource_error_to_str(smartthings_resource_error_e error)
{
	const char *err_str = NULL;

	switch (error) {
	case SMARTTHINGS_RESOURCE_ERROR_NONE:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_NONE";
		break;
	case SMARTTHINGS_RESOURCE_ERROR_INVALID_PARAMETER:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_INVALID_PARAMETER";
		break;
	case SMARTTHINGS_RESOURCE_ERROR_OUT_OF_MEMORY:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_OUT_OF_MEMORY";
		break;
	case SMARTTHINGS_RESOURCE_ERROR_PERMISSION_DENIED:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_PERMISSION_DENIED";
		break;
	case SMARTTHINGS_RESOURCE_ERROR_NO_DATA:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_NO_DATA";
		break;
	case SMARTTHINGS_RESOURCE_ERROR_NOT_SUPPORTED:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_NOT_SUPPORTED";
		break;
	case SMARTTHINGS_RESOURCE_ERROR_OPERATION_FAILED:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_NOT_SUPPORTED";
		break;
	case SMARTTHINGS_RESOURCE_ERROR_SERVICE_UNAVAILABLE:
		err_str = "SMARTTHINGS_RESOURCE_ERROR_SERVICE_UNAVAILABLE";
		break;
	default:
		err_str = "Unknown error";
		break;
	}

	return err_str;
}
#endif

static Eina_Bool _get_motion_sensor_data(void *user_data)
{
	int ret = 0;
	uint32_t value = 0;
	app_data *ad = user_data;

	if (!ad) {
		_E("failed to get app_data");
		return ECORE_CALLBACK_CANCEL;
	}

	ret = resource_read_infrared_motion_sensor(SENSOR_MOTION_GPIO_NUMBER, &value);
	if (ret != 0) {
		_E("cannot read data from the infrared motion sensor");
		return ECORE_CALLBACK_CANCEL;
	}
	ad->motion_data = value;

	_D("Detected motion value is: %u", value);

#ifdef USE_ST_SDK
	int error = SMARTTHINGS_RESOURCE_ERROR_NONE;
	smartthings_payload_h resp_payload = NULL;

	if (ad->status != SMARTTHINGS_STATUS_REGISTERED_TO_CLOUD)
		return ECORE_CALLBACK_RENEW;

	error = smartthings_payload_create(&resp_payload);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE || !resp_payload) {
		_E("smartthings_payload_create() failed, [%s]",
			_resource_error_to_str(error));
		return ECORE_CALLBACK_CANCEL;
	}

	error = smartthings_payload_set_bool(resp_payload, SENSOR_MOTION_KEY, (bool)ad->motion_data);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
		_E("smartthings_payload_set_bool() failed, [%s]",
			_resource_error_to_str(error));

	error = smartthings_resource_notify(ad->st_res_h, SENSOR_MOTION_URI, resp_payload);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
		_E("smartthings_resource_notify() failed, [%s]",
			_resource_error_to_str(error));

	if (smartthings_payload_destroy(resp_payload))
		_E("smartthings_payload_destroy() failed");
#endif

	return ECORE_CALLBACK_RENEW;
}

static int _set_led_data(app_data *ad, int state) {
	int ret = 0;

	ad->led_data = state;
	ret = resource_write_led(SENSOR_LED_GPIO_NUMBER, state);
	if (ret != 0) {
		_E("cannot write led data");
		return -1;
	}
	_I("LED : %d",state);

	return 0;
}

#ifdef USE_ST_SDK
/* SmartThings resource functions */
static bool _handle_get_motion(smartthings_payload_h resp_payload, void *user_data)
{
	int error = SMARTTHINGS_RESOURCE_ERROR_NONE;
	app_data *ad = user_data;

	retv_if(!ad, false);

	_D("Received a GET request for MOTION");

	error = smartthings_payload_set_bool(resp_payload, SENSOR_MOTION_KEY, (bool)ad->motion_data);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
		_E("smartthings_payload_set_bool() failed, [%s]",
			_resource_error_to_str(error));

	return true;
}

static bool _handle_get_led(smartthings_payload_h resp_payload, void *user_data)
{
	app_data *ad = user_data;
	int error = SMARTTHINGS_RESOURCE_ERROR_NONE;
	char *str = NULL;

	retv_if(!ad, false);

	_D("Received a GET request for LED");

	if (ad->led_data)
		str = SENSOR_LED_ON;
	else
		str = SENSOR_LED_OFF;

	error = smartthings_payload_set_string(resp_payload, SENSOR_LED_KEY, str);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
		_E("smartthings_payload_set_string() failed, [%s]",
			_resource_error_to_str(error));

	_D("Power : %s", str);

	return true;
}

static bool _handle_set_led(smartthings_payload_h payload, smartthings_payload_h resp_payload, void *user_data)
{
	app_data *ad = user_data;
	char *str = NULL;
	char *res_str = NULL;
	int ret = 0;
	int error = SMARTTHINGS_RESOURCE_ERROR_NONE;

	retv_if(!ad, false);

    _D("Received a SET request");

    error = smartthings_payload_get_string(payload, SENSOR_LED_KEY, &str);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
		_E("smartthings_payload_get_string() failed, [%s]",
			_resource_error_to_str(error));

	if (strncmp(str, SENSOR_LED_ON, strlen(SENSOR_LED_ON))) {
		ret = _set_led_data(ad, 0);
		res_str = SENSOR_LED_OFF;
	} else {
		ret = _set_led_data(ad, 1);
		res_str = SENSOR_LED_ON;
	}

	free(str);
	if (ret != 0) {
		_E("cannot set LED");
		return false;
	}

	error = smartthings_payload_set_string(resp_payload, SENSOR_LED_KEY, res_str);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
		_E("smartthings_payload_set_string() failed, [%s]",
			_resource_error_to_str(error));

    return true;
}

static void _request_cb(smartthings_resource_h handle, int req_id,
	const char *uri, smartthings_resource_req_type_e req_type,
	smartthings_payload_h payload, void *user_data)
{
	smartthings_payload_h resp_payload = NULL;
	bool result = false;
	int error = SMARTTHINGS_RESOURCE_ERROR_NONE;

	_D("request on %s, type[%d], id[%d]", uri, req_type, req_id);

	error = smartthings_payload_create(&resp_payload);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE || ! resp_payload)
		_E("smartthings_payload_create() failed, [%s]",
			_resource_error_to_str(error));

	if (req_type == SMARTTHINGS_RESOURCE_REQUEST_GET) {
		if (!strncmp(uri, SENSOR_MOTION_URI, strlen(SENSOR_MOTION_URI)))
			result = _handle_get_motion(resp_payload, user_data);
		else if (!strncmp(uri, SENSOR_LED_URI, strlen(SENSOR_LED_URI)))
			result = _handle_get_led(resp_payload, user_data);
		else
			_E("No matching Resource uri to get");
	} else if (req_type == SMARTTHINGS_RESOURCE_REQUEST_SET) {
		if (!strncmp(uri, SENSOR_LED_URI, strlen(SENSOR_LED_URI)))
			result = _handle_set_led(payload, resp_payload, user_data);
		else
			_E("No matching Resource uri to get");
	} else {
		_E("Invalid request type - %d", req_type);
		smartthings_payload_destroy(resp_payload);
		return;
	}

	error = smartthings_resource_send_response(handle, req_id, uri, resp_payload, result);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE) {
			_E("smartthings_resource_send_response() failed, [%s]",
				_resource_error_to_str(error));
			smartthings_payload_destroy(resp_payload);
			return;
	}

	if (req_type == SMARTTHINGS_RESOURCE_REQUEST_SET) {
			error = smartthings_resource_notify(handle, uri, resp_payload);
			if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
				_E("smartthings_resource_notify() failed, [%s]",
						_resource_error_to_str(error));
	}

	if (smartthings_payload_destroy(resp_payload))
		_E("smartthings_payload_destroy() failed");

	return;
}

static void _resource_connection_status_cb(
	smartthings_resource_h handle,
	smartthings_resource_connection_status_e status, void *user_data)
{
	app_data *ad = user_data;

	_D("status=[%d]", status);

	ret_if(!ad);

	if (status == SMARTTHINGS_RESOURCE_CONNECTION_STATUS_CONNECTED) {
		int error = SMARTTHINGS_RESOURCE_ERROR_NONE;
		error = smartthings_resource_set_request_cb(handle, _request_cb, ad);
		if (error != SMARTTHINGS_RESOURCE_ERROR_NONE) {
			_E("smartthings_resource_set_request_cb() is failed");
			return;
		}
	} else {
		_E("connection failed");
	}
}

static int _init_resource(app_data *ad)
{
	smartthings_resource_h st_res_h = NULL;
	int error = 0;

	retv_if(!ad, -1);
	if (ad->st_res_h) {
		_I("Already initialized!");
		return 0;
	}

	error = smartthings_resource_initialize(&st_res_h,
				_resource_connection_status_cb, ad);
	if (error) {
		_E("smartthings_resource_initialize() is failed, [%s]",
			_resource_error_to_str(error));
		return -1;
	}

	ad->st_res_h = st_res_h;

	return 0;
}

static int _fini_resource(app_data *ad)
{
	retv_if(!ad, -1);

	if (!ad->st_res_h)
		return 0;

	smartthings_resource_unset_request_cb(ad->st_res_h);
	smartthings_resource_deinitialize(ad->st_res_h);

	ad->st_res_h = NULL;

	return 0;
}

/* SmartThings master functions */
static const char *__master_error_to_str(smartthings_error_e error)
{
	const char *err_str = NULL;

	switch (error) {
	case SMARTTHINGS_ERROR_NONE:
		err_str = "SMARTTHINGS_ERROR_NONE";
		break;
	case SMARTTHINGS_ERROR_INVALID_PARAMETER:
		err_str = "SMARTTHINGS_ERROR_INVALID_PARAMETER";
		break;
	case SMARTTHINGS_ERROR_OUT_OF_MEMORY:
		err_str = "SMARTTHINGS_ERROR_OUT_OF_MEMORY";
		break;
	case SMARTTHINGS_ERROR_PERMISSION_DENIED:
		err_str = "SMARTTHINGS_ERROR_PERMISSION_DENIED";
		break;
	case SMARTTHINGS_ERROR_NO_DATA:
		err_str = "SMARTTHINGS_ERROR_NO_DATA";
		break;
	case SMARTTHINGS_ERROR_NOT_SUPPORTED:
		err_str = "SMARTTHINGS_ERROR_NOT_SUPPORTED";
		break;
	case SMARTTHINGS_ERROR_OPERATION_FAILED:
		err_str = "SMARTTHINGS_ERROR_OPERATION_FAILED";
		break;
	case SMARTTHINGS_ERROR_SERVICE_UNAVAILABLE:
		err_str = "SMARTTHINGS_ERROR_SERVICE_UNAVAILABLE";
		break;
	default:
		err_str = "Unknown error";
		break;
	}

	return err_str;
}

static void _user_confirm_cb(smartthings_h handle, void *user_data)
{
	if (smartthings_send_user_confirm(handle, true) != 0)
		_E("smartthings_send_user_confirm() is failed");
}

static void _reset_confirm_cb(smartthings_h handle, void *user_data)
{
	if (smartthings_send_reset_confirm(handle, true) != 0)
		_E("smartthings_send_reset_confirm() is failed");
}

static void _reset_result_cb(smartthings_h handle, bool result, void *user_data)
{
	_I("reset result = [%d]", result);
}

static void _thing_status_cb(
		smartthings_h handle, smartthings_status_e status, void *user_data)
{
	app_data *ad = user_data;
	_D("status: [%d]", status);
	ad->status = status;
}

//static void _things_connection_status_cb(smartthings_error_e error,
static void _things_connection_status_cb(
		smartthings_h handle, smartthings_connection_status_e status,
		void *user_data)
{
	_D("status = [%d]", status);

	if (status == SMARTTHINGS_CONNECTION_STATUS_CONNECTED) {
		int err = 0;
		bool is_es_completed = false;
		const char* dev_name = "motion-light-app";
		int wifi_mode = SMARTTHINGS_WIFI_MODE_11B
			| SMARTTHINGS_WIFI_MODE_11G
			| SMARTTHINGS_WIFI_MODE_11N;

		int wifi_freq = SMARTTHINGS_WIFI_FREQ_24G | SMARTTHINGS_WIFI_FREQ_5G;

		err = smartthings_set_device_property(
					handle, dev_name, wifi_mode, wifi_freq);
		if (err) {
			_E("smartthings_set_device_property() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_set_certificate_file(handle, CERT_FILE, PRIV_FILE);
		if (err) {
			_E("smartthings_set_certificate_file() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_set_user_confirm_cb(handle, _user_confirm_cb, NULL);
		if (err) {
			_E("smartthings_set_user_confirm_cb() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_set_reset_confirm_cb(handle, _reset_confirm_cb, NULL);
		if (err) {
			_E("smartthings_set_reset_confirm_cb() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_set_reset_result_cb(handle, _reset_result_cb, NULL);
		if (err) {
			_E("smartthings_set_reset_result_cb() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_set_status_changed_cb(handle, _thing_status_cb, user_data);
		if (err) {
			_E("smartthings_set_status_changed_callback() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_start(handle);
		if (err) {
			_E("smartthings_start() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_get_easysetup_status(handle, &is_es_completed);
		if (err) {
			_E("smartthings_get_easysetup_status() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		if (is_es_completed == true) {
			_I("Easysetup is already done");
			return;
		}

		_I("Easysetup is starting now");
		err = smartthings_start_easysetup(handle);
		if (err) {
			_E("smartthings_start_easysetup() is failed, [%s]",
				__master_error_to_str(err));
			smartthings_stop(handle);
			return;
		}
	} else {
			_E("connection failed");
	}
}

static int _init_master(app_data *ad)
{
	int err = 0;
	smartthings_h st_handle = NULL;

	retv_if(!ad, -1);

	if (ad->st_master_h) {
		_I("Already initialized!");
		return 0;
	}

	err = smartthings_initialize(&st_handle, _things_connection_status_cb, ad);
	if (err) {
		_E("smartthings_initialize() is failed, [%s]",
			__master_error_to_str(err));
		return -1;
	}

	ad->st_master_h = st_handle;

	return 0;
}

int _fini_master(app_data *ad)
{
	retv_if(!ad, -1);

	if (!ad->st_master_h) {
		_I("handle is already NULL");
		return 0;
	}

	smartthings_unset_user_confirm_cb(ad->st_master_h);
	smartthings_unset_reset_confirm_cb(ad->st_master_h);
	smartthings_unset_reset_result_cb(ad->st_master_h);
	smartthings_unset_status_changed_cb(ad->st_master_h);

	smartthings_stop_easysetup(ad->st_master_h);
	smartthings_stop(ad->st_master_h);

	if (smartthings_deinitialize(ad->st_master_h) != 0)  {
		_E("smartthings_deinitialize() is failed");
		return -1;
	}
	ad->st_master_h = NULL;

	return 0;
}
#endif /* USE_ST_SDK */

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

	ad->getter_timer = ecore_timer_add(TIMER_GATHER_INTERVAL, _get_motion_sensor_data, ad);
	if (!ad->getter_timer)
		_E("Failed to add a timer");
}

static bool service_app_create(void *user_data)
{
#ifdef USE_ST_SDK
	app_data *ad = user_data;

	if (_init_master(ad))
		return false;

	if (_init_resource(ad)) {
		_fini_master(ad);
		return false;
	}
#endif

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

#ifdef USE_ST_SDK
	_fini_resource(ad);
	_fini_master(ad);
#endif

	// Turn off LED light with __set_led()
	_set_led_data(ad, 0);

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
