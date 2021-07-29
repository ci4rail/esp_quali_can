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
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "quali_can_test_private.h"

#define CAN_REPORT_THREAD_STACK_SIZE        4096
#define CAN_TRANSMIT_THREAD_STACK_SIZE      4096
#define CAN_ALERT_THREAD_STACK_SIZE         4096
#define CAN_TEST_CONTROL_THREAD_STACK_SIZE  4096

static const char *TAG = "quali-can";

static void can_alert_task(void *arg)
{
    quali_can_test_handle_priv_t* handle_priv = (quali_can_test_handle_priv_t*)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;
    uint32_t alerts_to_enable = TWAI_ALERT_ARB_LOST |
                                TWAI_ALERT_ABOVE_ERR_WARN |
                                TWAI_ALERT_BUS_ERROR |
                                TWAI_ALERT_TX_FAILED |
                                TWAI_ALERT_ERR_PASS |
                                TWAI_ALERT_BUS_OFF;
    uint32_t alerts_triggered;

    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
        ESP_LOGI(TAG, "Alerts reconfigured");
    } else {
        ESP_LOGI(TAG, "Failed to reconfigure alerts");
        vTaskDelete(NULL);
    }

    while(!handle_priv->handle_data.stop_test) {
        twai_read_alerts(&alerts_triggered, portMAX_DELAY);

        if(alerts_triggered & TWAI_ALERT_ARB_LOST)
        {
            ESP_LOGI(TAG, "CAN-Alert: TWAI_ALERT_ARB_LOST");
            ESP_ERROR_CHECK(reporter->report_status(reporter, "ERR: TWAI_ALERT_ARB_LOST\n"));
        }
        if(alerts_triggered & TWAI_ALERT_ABOVE_ERR_WARN)
        {
            ESP_LOGI(TAG, "CAN-Alert: TWAI_ALERT_ABOVE_ERR_WARN");
            ESP_ERROR_CHECK(reporter->report_status(reporter, "ERR: TWAI_ALERT_ABOVE_ERR_WARN\n"));
        }
        if(alerts_triggered & TWAI_ALERT_BUS_ERROR)
        {
            ESP_LOGI(TAG, "CAN-Alert: TWAI_ALERT_BUS_ERROR");
            ESP_ERROR_CHECK(reporter->report_status(reporter, "ERR: TWAI_ALERT_BUS_ERROR\n"));
        }
        if(alerts_triggered & TWAI_ALERT_TX_FAILED)
        {
            ESP_LOGI(TAG, "CAN-Alert: TWAI_ALERT_TX_FAILED");
            ESP_ERROR_CHECK(reporter->report_status(reporter, "ERR: TWAI_ALERT_TX_FAILED\n"));
        }
        if(alerts_triggered & TWAI_ALERT_ERR_PASS)
        {
            ESP_LOGI(TAG, "CAN-Alert: TWAI_ALERT_ERR_PASS");
            ESP_ERROR_CHECK(reporter->report_status(reporter, "ERR: TWAI_ALERT_ERR_PASS\n"));
        }
        if(alerts_triggered & TWAI_ALERT_BUS_OFF)
        {
            ESP_LOGI(TAG, "CAN-Alert: TWAI_ALERT_BUS_OFF");
            ESP_ERROR_CHECK(reporter->report_status(reporter, "ERR: TWAI_ALERT_BUS_OFF\n"));
        }
    }
}

static void can_report_task(void *arg)
{
    quali_can_test_handle_priv_t* handle_priv = (quali_can_test_handle_priv_t*)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;
    uint32_t last_len = 0;
    uint32_t last_msg_cnt = 0;

    while(!handle_priv->handle_data.stop_test)
    {
        char msg[MAX_MSG_SIZE];
        vTaskDelay(pdMS_TO_TICKS(1000));
        /* print amount of payload dent in the last second */
        ESP_LOGI(TAG, "Data rate (last second): %d bit/s (%d messages sent in the last second)\n",
        (handle_priv->handle_data.total_data - last_len)*8, handle_priv->handle_data.msg_counter - last_msg_cnt);
        /* report data rate */
        snprintf(&(msg[0]), MAX_MSG_SIZE, "INF: Data rate: %d bit/s (%d messages/sec)\n",
        (handle_priv->handle_data.total_data - last_len)*8, handle_priv->handle_data.msg_counter - last_msg_cnt);
        ESP_ERROR_CHECK(reporter->report_status(reporter, msg));
        last_len = handle_priv->handle_data.total_data;
        last_msg_cnt = handle_priv->handle_data.msg_counter;
    }
    vTaskDelete(NULL);
}

static void can_transmit_task(void *arg)
{
    quali_can_test_handle_priv_t* handle_priv = (quali_can_test_handle_priv_t*)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;
    const twai_message_t message = {.identifier = handle_priv->handle_data.msg_id, .data_length_code = 8,
                                      .ss = 1, .data = {1, 2, 3, 4, 5, 6, 7, 8}};

    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "Driver started");

    ESP_LOGI(TAG, "Message Details:");
    if (message.extd) {
        ESP_LOGI(TAG, "Message is in Extended Format");
    } else {
        ESP_LOGI(TAG, "Message is in Standard Format");
    }
    ESP_LOGI(TAG, "ID is 0x%x", message.identifier);
    if (!(message.rtr)) {
        for (int i = 0; i < message.data_length_code; i++) {
            ESP_LOGI(TAG, "Data byte %d = %d", i, message.data[i]);
        }
    }

    while (!handle_priv->handle_data.stop_test) {
        esp_err_t ret;
        ret = twai_transmit(&message, portMAX_DELAY);
        if(ret != ESP_OK)
        {
            ESP_LOGI(TAG, "Transmit Error: ret = 0x%x", ret);
            ESP_ERROR_CHECK(reporter->report_status(reporter, "ERR: can transmit error"));
        }
        else
        {
            handle_priv->handle_data.msg_counter++;
            handle_priv->handle_data.total_data += 8;
        }
    }

    ESP_ERROR_CHECK(twai_stop());
    ESP_LOGI(TAG, "Driver stopped");
    vTaskDelete(NULL);
}

static void can_test_control_task(void *arg)
{
    quali_can_test_handle_priv_t* handle_priv = (quali_can_test_handle_priv_t*)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;

    /* install TWAI driver */
    ESP_ERROR_CHECK(twai_driver_install(handle_priv->handle_data.g_config,
                                        handle_priv->handle_data.t_config,
                                        handle_priv->handle_data.f_config));
    ESP_LOGI(TAG, "Driver installed");
    xTaskCreate(&can_alert_task, "can_alert", CAN_ALERT_THREAD_STACK_SIZE, (void*)handle_priv, 5, NULL);

    while(!handle_priv->handle_data.stop_restart) {
        ESP_LOGI(TAG, "Wait for start...");
        ESP_ERROR_CHECK(reporter->wait_for_start(reporter));
        xTaskCreate(&can_report_task, "can_report", CAN_REPORT_THREAD_STACK_SIZE, (void*)handle_priv, 5, NULL);
        xTaskCreate(&can_transmit_task, "can_transmit", CAN_TRANSMIT_THREAD_STACK_SIZE, (void*)handle_priv, 5, NULL);
        ESP_LOGI(TAG, "Wait for stop...");
        ESP_ERROR_CHECK(reporter->wait_for_stop(reporter));
        handle_priv->handle_data.stop_test = true;
    }

    vTaskDelete(NULL);
}

esp_err_t can_stop_restart(quali_can_test_handle_t *handle)
{
    quali_can_test_handle_priv_t* handle_priv = (quali_can_test_handle_priv_t*)handle;
    handle_priv->handle_data.stop_restart = true;
    return ESP_OK;
}

esp_err_t new_quali_can_instance(quali_can_test_handle_t **return_handle,
                                 test_status_report_handle_t *reporter,
                                 const twai_timing_config_t *t_config,
                                 const twai_filter_config_t *f_config,
                                 const twai_general_config_t *g_config,
                                 uint32_t msg_id)
{
    /* create handle */
    quali_can_test_handle_priv_t* handle = malloc(sizeof(quali_can_test_handle_priv_t));
    if(handle == NULL) {
        ESP_LOGE(TAG, "Could not allocate memory for status report handle");
        return ESP_FAIL;
    }
    /* set methods */
    handle->handle_pub.stop_restart = &can_stop_restart;
    /* set parameter */
    handle->handle_data.reporter = reporter;
    handle->handle_data.t_config = t_config;
    handle->handle_data.f_config = f_config;
    handle->handle_data.g_config = g_config;
    handle->handle_data.msg_id = msg_id;
    handle->handle_data.total_data = 0;
    handle->handle_data.msg_counter = 0;
    handle->handle_data.stop_test = false;
    handle->handle_data.stop_restart = false;

    /* disable CAN_SILENT */
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_18, 0);

    xTaskCreate(&can_test_control_task, "can_test_control", CAN_TEST_CONTROL_THREAD_STACK_SIZE, (void*)handle, 5, NULL);
    *return_handle = (quali_can_test_handle_t*) handle;
    return ESP_OK;
}
