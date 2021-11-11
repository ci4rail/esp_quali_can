/*
Copyright © 2021 Ci4Rail GmbH
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef _QUALI_CAN_TEST_PRIVATE_H_
#define _QUALI_CAN_TEST_PRIVATE_H_

#include "quali_can_test.h"

#define STATUS_REPORT_THREAD_STACK_SIZE  4096

typedef struct {
    test_status_report_handle_t *reporter;
    const twai_timing_config_t *t_config;
    const twai_filter_config_t *f_config;
    const twai_general_config_t *g_config;
    uint32_t msg_id;
    uint32_t total_data;
    uint32_t msg_counter;
    bool stop_test;
    bool stop_restart;
}quali_can_test_data_t;


typedef struct {
    quali_can_test_handle_t handle_pub;
    quali_can_test_data_t handle_data;
}quali_can_test_handle_priv_t;

#endif //_QUALI_CAN_TEST_PRIVATE_H_