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
#ifndef _QUALI_CAN_TEST_H_
#define _QUALI_CAN_TEST_H_

#include "driver/twai.h"
#include "esp_err.h"
#include "test_status_report.h"

typedef struct quali_can_test_handle_t quali_can_test_handle_t;

struct quali_can_test_handle_t {
    esp_err_t (*stop_restart)(quali_can_test_handle_t *);
};

esp_err_t new_quali_can_instance(quali_can_test_handle_t **return_handle, test_status_report_handle_t *reporter,
    const twai_timing_config_t *t_config, const twai_filter_config_t *f_config, const twai_general_config_t *g_config,
    uint32_t msg_id);

#endif  //_QUALI_CAN_TEST_H_
