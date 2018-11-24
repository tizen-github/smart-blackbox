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
#include <stdint.h>
#include <Ecore.h>

#include "log.h"
#include "resource_illuminance_sensor.h"
#include "resource_servo_motor.h"

// Peripheral info for Illuminance Sensor
#define I2C_BUS_NUMBER (1)
#define SENSOR_GATHER_INTERVAL (5.0f)
#define ILLUMINATION_CRITERIA (1000)

// Peripheral info for Servo-motor(HS-53)
// Duty Cycle : 0.54ms ~ 2.1ms
// Spec Duty Cycle : 0.553ms ~ 2.227ms(https://www.servocity.com/hitec-hs-53-servo)
#define SERVO_MOTOR_DUTY_CYCLE_COUNTER_CLOCKWISE 1.0
#define SERVO_MOTOR_DUTY_CYCLE_CLOCKWISE 2.0

typedef struct app_data_s {
	uint32_t illuminance_value;
	Ecore_Timer *getter_illuminance;
} app_data;

static app_data *g_ad = NULL;

static int _get_illuminance(void *data)
{
	int ret = 0;
	app_data *ad = data;

	if (!ad) {
		_E("No user data");
		return -1;
	}

	ret = resource_read_illuminance_sensor(I2C_BUS_NUMBER, &ad->illuminance_value);
	if (ret != 0) {
		_E("cannot read illuminance sensor");
		return -1;
	}

	_D("Illuminance value : %u", ad->illuminance_value);

	return 0;
}


static int _set_servo_motor(int on)
{
	double duty_cycle = 0;
	int ret = 0;

	if (on)
		duty_cycle = SERVO_MOTOR_DUTY_CYCLE_CLOCKWISE;
	else
		duty_cycle = SERVO_MOTOR_DUTY_CYCLE_COUNTER_CLOCKWISE;

	ret = resource_set_servo_motor_value(duty_cycle);
	if (ret != 0) {
		_E("cannot set servo motor");
		return -1;
	}

	return 0;
}

static Eina_Bool __illuminance_to_servo_motor(void *data)
{
    int ret = 0;
    int on = 0;
    app_data *ad = data;

    if (!ad) {
        _E("failed to get app_data");
        return ECORE_CALLBACK_CANCEL;
    }

    ret = _get_illuminance(ad);
    if (ret != 0) {
        _E("cannot get illuminance");
        return ECORE_CALLBACK_CANCEL;
    }

    if (ad->illuminance_value < ILLUMINATION_CRITERIA) {
        on = 0;
    } else {
        on = 1;
    }

    ret = _set_servo_motor(on);
    if (ret != 0) {
        _E("cannot set servo motor");
        return ECORE_CALLBACK_RENEW;
    }

    return ECORE_CALLBACK_RENEW;
}

static void _stop_gathering(void *data)
{
	app_data *ad = data;

	ret_if(!ad);

	if (ad->getter_illuminance) {
		ecore_timer_del(ad->getter_illuminance);
		ad->getter_illuminance = NULL;
	}
}

static void _start_gathering(void *data)
{
	app_data *ad = data;

	ret_if(!ad);

	_stop_gathering(ad);

	ad->getter_illuminance = ecore_timer_add(SENSOR_GATHER_INTERVAL, __illuminance_to_servo_motor, ad);
	if (!ad->getter_illuminance)
		_E("Failed to add getter_illuminance");
}

static bool service_app_create(void *user_data)
{
	return true;
}

static void service_app_terminate(void *user_data)
{
	app_data *ad = user_data;
	if (!ad)
		return;

	_stop_gathering(ad);
	resource_close_servo_motor();
	resource_close_illuminance_sensor();

	free(ad);
}

static void service_app_control(app_control_h app_control, void *user_data)
{
	app_data *ad = user_data;
	_start_gathering(ad);
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
