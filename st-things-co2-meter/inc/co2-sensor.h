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

#ifndef __CO2_SENSOR_H__
#define __CO2_SENSOR_H__

 /**
  * @brief Reads the value from co2 sensor through AD converter(MCP3008).
  * @param[in] ch_num The number of channel connected to the co2 detection sensor with AD converter
  * @param[out] out_value The vaule of a co2 level
  * @return 0 on success, otherwise a negative error value
  *
  */
int co2_sensor_read(int ch_num, unsigned int *out_value);

/* release resource */
void co2_sensor_close(void);

/* utility functions */
double co2_sensor_value_to_voltage(unsigned int value);


/* Fallowing functions art Not implemented, please see comments in c source code */
int co2_sensor_set_calibration_values(unsigned int zero_point_v,
		unsigned int second_point_ppm, unsigned int second_point_v);
unsigned int co2_sensor_voltage_to_ppm(double voltage);
unsigned int co2_sensor_value_to_ppm(unsigned int value);

#endif /* __CO2_SENSOR_H__ */
