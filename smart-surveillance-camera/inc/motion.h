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

#ifndef __MOTION_H__
#define __MOTION_H__

typedef void(*motion_state_changed_cb) (int state, void* user_data);

int motion_initialize(void);
int motion_finalize(void);
int motion_state_set(int state, const char *pass_key);
int motion_state_get(int *state);
int motion_state_changed_cb_set(
	const char *callback_key, motion_state_changed_cb callback, void *cb_data);

#endif /* __MOTION_H__ */