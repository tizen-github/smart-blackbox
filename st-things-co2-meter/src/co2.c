/* ****************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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
 *
 ******************************************************************/

#include <tizen.h>
#include <service_app.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "log.h"
#include "sensor-data.h"
#include "co2-sensor.h"

#define SENSOR_CH_CO2 (0)
#define SENSOR_GATHER_INTERVAL (50) //50ms
#define SENSOR_GATHER_COUNT (60)

//#define USE_ST_SDK

#ifdef USE_ST_SDK

#include "smartthings.h"
#include "smartthings_resource.h"
#include "smartthings_payload.h"

/*  You have to FIX IT !!!  */
#define CERT_FILE "certificate.pem" // cert file name in 'res' directory
#define PRIV_FILE "privatekey.der" // private key file name in 'res' directory


#define SENSOR_URI_CO2 "/capability/airQualitySensor/main/0"
#define SENSOR_KEY_CO2 "airQuality"

#endif /* USE_ST_SDK */

typedef struct app_data_s {
	guint getter_co2;
	sensor_data *co2_data;
#ifdef USE_ST_SDK
	smartthings_h st_master_h;
	smartthings_resource_h st_res_h;
	smartthings_resource_connection_status_e st_res_conn_status;
#endif /* USE_ST_SDK */
} app_data;

static app_data *g_ad = NULL;

#ifdef USE_ST_SDK

/* smartthings resource functions */
static const char *
__resource_error_to_str(smartthings_resource_error_e error)
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

static bool
handle_get_co2(smartthings_payload_h resp_payload, void *user_data)
{
	app_data *ad = user_data;
	unsigned int value = 0;

	retv_if(!ad, false);

	sensor_data_get_uint(ad->co2_data, &value);
	smartthings_payload_set_int(resp_payload, SENSOR_KEY_CO2, (int)value);

	return true;
}

static void
_request_cb(smartthings_resource_h handle, int req_id,
	const char *uri, smartthings_resource_req_type_e req_type,
	smartthings_payload_h payload, void *user_data)
{
	smartthings_payload_h resp_payload = NULL;
	bool result = false;
	int error = SMARTTHINGS_RESOURCE_ERROR_NONE;

	_D("request on %s, type[%d], id[%d]", uri, req_type, req_id);

	smartthings_payload_create(&resp_payload);
	if (!resp_payload) {
		_E("Response payload is NULL");
		return;
	}

	if (req_type == SMARTTHINGS_RESOURCE_REQUEST_GET) {
		if (0 == g_strcmp0(uri, SENSOR_URI_CO2))
			result = handle_get_co2(resp_payload, user_data);
		else
			_E("No matching Resource uri to get");
	} else if (req_type == SMARTTHINGS_RESOURCE_REQUEST_SET) {
			_E("No matching Resource uri to set");
	} else {
		_E("Invalid request type - %d", req_type);
		smartthings_payload_destroy(resp_payload);
		return;
	}

	error = smartthings_resource_send_response(handle, req_id, uri, resp_payload, result);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE) {
			smartthings_payload_destroy(resp_payload);
			_E("smartthings_resource_send_response() failed, [%s]",
				__resource_error_to_str(error));
			return;
	}

	if (req_type == SMARTTHINGS_RESOURCE_REQUEST_SET) {
			error = smartthings_resource_notify(handle, uri, resp_payload);
			if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
				_E("smartthings_resource_notify() failed, [%s]",
				__resource_error_to_str(error));
	}

	if (smartthings_payload_destroy(resp_payload))
		_E("smartthings_payload_destroy failed");

	return;
}

static void
_resource_connection_status_cb(smartthings_resource_error_e error,
	smartthings_resource_h handle,
	smartthings_resource_connection_status_e status, void *user_data)
{
	app_data *ad = user_data;

	_D("result [%s], status=[%d]", __resource_error_to_str(error), status);

	ret_if(!ad);

	ad->st_res_conn_status = status;

	if (status == SMARTTHINGS_RESOURCE_CONNECTION_STATUS_CONNECTED) {
		if (smartthings_resource_set_request_cb(handle, _request_cb, ad)) {
			_E("smartthings_resource_set_request_cb() is failed");
			return;
		}
	} else {
		_E("connection failed");
	}
	return;
}

static int
st_thing_notify_resource(app_data *ad, const char *uri, const char *key, sensor_data *data)
{
	smartthings_resource_h handle = NULL;
	smartthings_payload_h payload = NULL;
	int error = SMARTTHINGS_RESOURCE_ERROR_NONE;
	sensor_data_type_e data_type = SENSOR_DATA_TYPE_NONE;

	retv_if(!ad, -1);
	retv_if(!ad->st_res_h, -1);
	retv_if(ad->st_res_conn_status != SMARTTHINGS_RESOURCE_CONNECTION_STATUS_CONNECTED, -1);
	retv_if(!uri, -1);
	retv_if(!key, -1);
	retv_if(!data, -1);

	handle = ad->st_res_h;

	data_type = sensor_data_get_type(data);
	retv_if(data_type == SENSOR_DATA_TYPE_NONE, -1);

	smartthings_payload_create(&payload);
	if (!payload) {
		_E("failed to create payload is NULL");
		return -1;
	}

	switch (data_type) {
	case SENSOR_DATA_TYPE_INT:
		{
			int value = 0;
			sensor_data_get_int(data, &value);
			smartthings_payload_set_int(payload, key, value);
		}
		break;
	case SENSOR_DATA_TYPE_UINT:
		{
			unsigned int value = 0;
			sensor_data_get_uint(data, &value);
			smartthings_payload_set_int(payload, key, (int)value);
		}
		break;
	case SENSOR_DATA_TYPE_BOOL:
		{
			bool value = 0;
			sensor_data_get_bool(data, &value);
			smartthings_payload_set_bool(payload, key, value);
		}
		break;
	case SENSOR_DATA_TYPE_DOUBLE:
		{
			double value = 0;
			sensor_data_get_double(data, &value);
			smartthings_payload_set_double(payload, key, value);
		}
		break;
	case SENSOR_DATA_TYPE_STR:
		{
			const char *value = NULL;
			sensor_data_get_string(data, &value);
			smartthings_payload_set_string(payload, key, value);
		}
		break;
	case SENSOR_DATA_TYPE_NONE:
	default:
		_E("unsupport data type");
		break;
	}

	error = smartthings_resource_notify(handle, uri, payload);
	if (error != SMARTTHINGS_RESOURCE_ERROR_NONE)
		_E("smartthings_resource_notify() failed, [%s]",
			__resource_error_to_str(error));

	smartthings_payload_destroy(payload);

	return 0;
}

static int st_thing_resource_init(app_data *ad)
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
			__resource_error_to_str(error));
		return -1;
	}

	ad->st_res_h = st_res_h;
	ad->st_res_conn_status = SMARTTHINGS_RESOURCE_CONNECTION_STATUS_DISCONNECTED;

	return 0;
}

static int st_thing_resource_fini(app_data *ad)
{
	retv_if(!ad, -1);

	if (!ad->st_res_h)
		return 0;

	smartthings_resource_unset_request_cb(ad->st_res_h);
	smartthings_resource_deinitialize(ad->st_res_h);

	ad->st_res_h = NULL;
	ad->st_res_conn_status = SMARTTHINGS_RESOURCE_CONNECTION_STATUS_DISCONNECTED;

	return 0;
}

/* smartthings master functions */
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

static void
_thing_status_cb(
		smartthings_h handle, smartthings_status_e status, void *user_data)
{
	_D("status: [%d]", status);
}

static void
_things_connection_status_cb(smartthings_error_e error,
	smartthings_h handle, smartthings_connection_status_e status,
	void *user_data)
{
	_D("result [%s], status = [%d]", __master_error_to_str(error), status);

	if (status == SMARTTHINGS_CONNECTION_STATUS_CONNECTED) {
		int err = 0;
		bool is_es_completed = false;
		const char* dev_name = "co2-app";
		int wifi_mode = SMARTTHINGS_WIFI_MODE_11B
			| SMARTTHINGS_WIFI_MODE_11G
			| SMARTTHINGS_WIFI_MODE_11N;

		int wifi_freq = SMARTTHINGS_WIFI_FREQ_24G | SMARTTHINGS_WIFI_FREQ_5G;

		err = smartthings_set_device_property(
					handle, dev_name, wifi_mode, wifi_freq);
		if (err) {
			_E("smartthings_initialize() is failed, [%s]",
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
			_E("smartthings_set_reset_confirm_cb() is failed, [%s]",
				__master_error_to_str(err));
			return;
		}

		err = smartthings_set_status_changed_cb(handle, _thing_status_cb, NULL);
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
	return;
}

static int st_thing_master_init(app_data *ad)
{
	int err = 0;
	smartthings_h st_handle = NULL;

	retv_if(!ad, -1);

	if (ad->st_master_h) {
		_I("Already initialized!");
		return 0;
	}

	err = smartthings_initialize(&st_handle, _things_connection_status_cb, NULL);
	if (err) {
		_E("smartthings_initialize() is failed, [%s]",
			__master_error_to_str(err));
		return -1;
	}

	ad->st_master_h = st_handle;

	return 0;
}

int st_thing_master_fini(app_data *ad)
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

static gboolean __get_co2(gpointer user_data)
{
	int ret = 0;
	unsigned int value = 0;
	static unsigned int sum = 0;
	static unsigned int count = 0;

	app_data *ad = user_data;

	if (!ad) {
		_E("failed to get app_data");
		service_app_exit();
		return FALSE;
	}

	if (!ad->co2_data) {
		_E("failed to get co2_data");
		service_app_exit();
		ad->getter_co2 = 0;
		return FALSE;
	}

	ret = co2_sensor_read(SENSOR_CH_CO2, &value);
	retv_if(ret != 0, TRUE);

	count++;
	sum += value;

	if (count == SENSOR_GATHER_COUNT) {
		unsigned int avg = 0;
		avg = sum/SENSOR_GATHER_COUNT;

		_D("co2 avg - [%u], [%u ppm]", avg, co2_sensor_value_to_ppm(avg));

		sensor_data_set_uint(ad->co2_data, avg);

#ifdef USE_ST_SDK
		st_thing_notify_resource(ad, SENSOR_URI_CO2, SENSOR_KEY_CO2, ad->co2_data);
#endif
		count = 0;
		sum = 0;
	}

	return TRUE;
}

static void gathering_stop(void *data)
{
	app_data *ad = data;
	ret_if(!ad);

	if (ad->getter_co2) {
		g_source_remove(ad->getter_co2);
		ad->getter_co2 = 0;
	}
}

static void gathering_start(void *data)
{
	app_data *ad = data;
	ret_if(!ad);
	ad->getter_co2 = g_timeout_add(SENSOR_GATHER_INTERVAL, __get_co2, ad);
	if (!ad->getter_co2)
		_E("Failed to add getter_co2");
}

static bool service_app_create(void *user_data)
{
	app_data *ad = (app_data *)user_data;

	ad->co2_data = sensor_data_new(SENSOR_DATA_TYPE_UINT);
	if (!ad->co2_data)
		return false;

#ifdef USE_ST_SDK
	if (st_thing_master_init(ad))
		return false;

	if (st_thing_resource_init(ad)) {
		st_thing_master_fini(ad);
		return false;
	}
#endif

	return true;
}

static void service_app_control(app_control_h app_control, void *user_data)
{
	gathering_stop(user_data);
	gathering_start(user_data);
}

static void service_app_terminate(void *user_data)
{
	app_data *ad = (app_data *)user_data;

	if (!ad)
		return;

#ifdef USE_ST_SDK
	st_thing_resource_fini(ad);
	st_thing_master_fini(ad);
#endif

	sensor_data_free(ad->co2_data);
	co2_sensor_close();
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

