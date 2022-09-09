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
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/twai_types.h"
#include "io4edge.h"
#include "io4edge_core.h"
#include "io4edge_machine.h"
#include "quali_can_test.h"
#include "test_status_report.h"

static char *TAG = "main";

#define TX_GPIO_NUM 21
#define RX_GPIO_NUM 33
#define MSG_ID 0x0A3

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);

static bool application_is_working(void)
{
    ESP_LOGI(TAG, "application_is_working called");
    return true;
}

void can_test_start(void)
{
    quali_can_test_handle_t *hdl;
    test_status_report_handle_t *sr_handle;

    test_status_report_config_t config = {.instance = "iou04-usb-ext-can", .instance_idx = 0, .port = 10002};

    /* disable CAN_SILENT */
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_18, 0);

    ESP_ERROR_CHECK(new_test_status_report_instance(&sr_handle, &config));
    ESP_ERROR_CHECK(new_quali_can_instance(&hdl, sr_handle, &t_config, &f_config, &g_config, MSG_ID));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Start");

    io4edge_config_t io4edge_config = {
        .log_config =
            {
                .min_buffered_lines = 30,
                .enable_deferred_console_logging = true,
            },
    };
    ESP_ERROR_CHECK(io4edge_init(&io4edge_config));

    static io4edge_core_config_t io4edge_core_config = {
        .core_server_priority = 5,
        .application_is_working = application_is_working,
    };
    ESP_ERROR_CHECK(io4edge_core_start(&io4edge_core_config));

    /* enable ENDC_DC */
    gpio_set_direction(GPIO_NUM_34, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_34, 1);

    can_test_start();

    for (;;) {
        ESP_LOGI(TAG, "Alive");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
