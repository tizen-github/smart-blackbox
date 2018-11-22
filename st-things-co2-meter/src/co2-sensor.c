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

#include <peripheral_io.h>
#include <math.h>
#include "adc-mcp3008.h"
#include "log.h"

#define CO2_SENSOR_VOLTAGE_GAIN (8.5)

#define CO2_SENSOR_REF_VOLTAGE (5)
#define CO2_SENSOR_VALUE_MAX (1024)

//#define USE_EXAMPLE_CODE

#ifdef USE_EXAMPLE_CODE
/* CAUTION !!  These data are EXAMPLE Data, NOT REAL data */
#define CO2_SENSOR_DEFAULT_VALUE_ZP (690) // 400ppm
#define CO2_SENSOR_DEFAULT_VALUE_1000 (600)
#endif /* USE_EXAMPLE_CODE */

static const double log400 = 2.602;

static bool initialized = false;
static double co2_zp_volt = -1;
static double slope_value = 10000; // A real slope value should be a negative value.

void co2_sensor_close(void)
{
	adc_mcp3008_fini();
	initialized = false;
}

int co2_sensor_read(int ch_num, unsigned int *out_value)
{
	unsigned int read_value = 0;
	int ret = 0;

	if (!initialized) {
		ret = adc_mcp3008_init();
		retv_if(ret != 0, -1);
		initialized = true;
	}

	ret = adc_mcp3008_read(ch_num, &read_value);
	retv_if(ret != 0, -1);

	*out_value = read_value;

	return 0;
}

static inline double __value_to_volt(unsigned int value)
{
	return (double)value * CO2_SENSOR_REF_VOLTAGE / CO2_SENSOR_VALUE_MAX / CO2_SENSOR_VOLTAGE_GAIN;
}

static double __calc_slope(double zp_volt, double sec_volt, double x_axis_diff)
{
	return (zp_volt - sec_volt) / (x_axis_diff);
}

/* To use to calibrate ppm value*/
int co2_sensor_set_calibration_values(unsigned int zero_point_v,
		unsigned int second_point_ppm, unsigned int second_point_v)
{
	double sec_volt = -1;
	double x_axis_diff = -1;

	retvm_if(zero_point_v <= second_point_v, -1, "%u - %u", zero_point_v, second_point_v);
	retv_if(second_point_ppm <= 400, -1);

	co2_zp_volt = __value_to_volt(zero_point_v);
	sec_volt = __value_to_volt(second_point_v);
	x_axis_diff = log400 - log10(second_point_ppm);
	slope_value = __calc_slope(co2_zp_volt, sec_volt, x_axis_diff);

	return 0;
}

double co2_sensor_value_to_voltage(unsigned int value)
{
	retv_if(value > 1023, -100000000.0); // out of value range

	return __value_to_volt(value);
}

/* Not implemented !!! */
unsigned int co2_sensor_voltage_to_ppm(double voltage)
{
	double ppm = 0;
	/* Make this function yourself */
	/* Please ref. 'Theory' section in https://sandboxelectronics.com/?p=147#Theory */

	/* Example */
#ifdef USE_EXAMPLE_CODE
	if (slope_value > 0)
		co2_sensor_set_calibration_values(CO2_SENSOR_DEFAULT_VALUE_ZP,
				1000, CO2_SENSOR_DEFAULT_VALUE_1000);

	if (voltage >= co2_zp_volt)
		return 400;

	ppm = pow(10, (voltage - co2_zp_volt)/slope_value + log400);
	if (ppm > 10000) {
		_W("ppm value[%lf] is too large - limiting to 10000ppm", ppm);
		ppm = 10000;
	}
#endif /* USE_EXAMPLE_CODE */

	return (unsigned int)ppm;
}

unsigned int co2_sensor_value_to_ppm(unsigned int value)
{
	/* You can use this function after implementing co2_sensor_voltage_to_ppm() function */
	return co2_sensor_voltage_to_ppm(co2_sensor_value_to_voltage(value));
}

