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
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "quali_can_test_private.h"

#define CAN_REPORT_THREAD_STACK_SIZE 4096
#define CAN_TRANSMIT_THREAD_STACK_SIZE 4096
#define CAN_RECEIVE_THREAD_STACK_SIZE 4096
#define CAN_ALERT_THREAD_STACK_SIZE 4096
#define CAN_TEST_CONTROL_THREAD_STACK_SIZE 4096

#define CAN_RECEIVE_DATA_LENGTH 8

static const char *TAG = "quali-can";

static void can_tx_report_task(void *arg)
{
    quali_can_test_handle_priv_t *handle_priv = (quali_can_test_handle_priv_t *)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;
    uint32_t last_len = 0;
    uint32_t last_msg_cnt = 0;

    while (!handle_priv->handle_data.stop_test) {
        char msg[MAX_MSG_SIZE];
        vTaskDelay(pdMS_TO_TICKS(1000));
        /* print amount of payload dent in the last second */
        ESP_LOGI(TAG, "Data rate (last second): %d bit/s (%d messages sent in the last second)",
            (handle_priv->handle_data.total_tx_data - last_len) * 8,
            handle_priv->handle_data.tx_msg_counter - last_msg_cnt);
        /* report data rate */
        snprintf(&(msg[0]), MAX_MSG_SIZE, "INF: Transmit Data rate: %d bit/s (%d messages/sec)\n",
            (handle_priv->handle_data.total_tx_data - last_len) * 8,
            handle_priv->handle_data.tx_msg_counter - last_msg_cnt);
        reporter->report_status(reporter, msg);
        last_len = handle_priv->handle_data.total_tx_data;
        last_msg_cnt = handle_priv->handle_data.tx_msg_counter;
    }

    vTaskDelete(NULL);
}

static void can_rx_report_task(void *arg)
{
    quali_can_test_handle_priv_t *handle_priv = (quali_can_test_handle_priv_t *)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;
    uint32_t last_len = 0;
    uint32_t last_msg_cnt = 0;

    while (!handle_priv->handle_data.stop_test) {
        char msg[MAX_MSG_SIZE];
        vTaskDelay(pdMS_TO_TICKS(1000));
        /* print amount of payload dent in the last second */
        ESP_LOGI(TAG, "Data rate (last second): %d bit/s (%d messages received in the last second)",
            (handle_priv->handle_data.total_rx_data - last_len) * 8,
            handle_priv->handle_data.rx_msg_counter - last_msg_cnt);
        /* report data rate */
        snprintf(&(msg[0]), MAX_MSG_SIZE, "INF: Receive Data rate: %d bit/s (%d messages/sec)\n",
            (handle_priv->handle_data.total_rx_data - last_len) * 8,
            handle_priv->handle_data.rx_msg_counter - last_msg_cnt);
        reporter->report_status(reporter, msg);
        last_len = handle_priv->handle_data.total_rx_data;
        last_msg_cnt = handle_priv->handle_data.rx_msg_counter;
    }

    vTaskDelete(NULL);
}

static void can_transmit_task(void *arg)
{
    quali_can_test_handle_priv_t *handle_priv = (quali_can_test_handle_priv_t *)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;
    const twai_message_t message = {.identifier = handle_priv->handle_data.msg_id,
        .data_length_code = 8,
        .ss = 1,
        .data = {1, 2, 3, 4, 5, 6, 7, 8}};

    xSemaphoreTake(handle_priv->handle_data.driver_start, portMAX_DELAY);

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
        char msg[MAX_MSG_SIZE];
        ret = twai_transmit(&message, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGI(TAG, "Transmit Error: ret = 0x%x", ret);
            snprintf(msg, MAX_MSG_SIZE, "ERR: Transmit Error: ret = 0x%x\n", ret);
            reporter->report_status(reporter, msg);
            vTaskDelay(pdMS_TO_TICKS(500));  // don't flood host with errors
        } else {
            handle_priv->handle_data.tx_msg_counter++;
            handle_priv->handle_data.total_tx_data += 8;
        }
    }

    // notify receive task that driver is ready to stop
    xSemaphoreGive(handle_priv->handle_data.driver_stop);

    vTaskDelete(NULL);
}

static void can_receive_task(void *arg)
{
    quali_can_test_handle_priv_t *handle_priv = (quali_can_test_handle_priv_t *)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;
    esp_err_t ret;
    char msg[MAX_MSG_SIZE];
    uint8_t i = 0;
    bool sync_data = true;
    twai_message_t message;

    ESP_LOGI(TAG, "started can receive task");

    twai_start();
    ESP_LOGI(TAG, "Driver started");

    // notify transmit task that driver is started
    xSemaphoreGive(handle_priv->handle_data.driver_start);

    while (!handle_priv->handle_data.stop_test) {
        uint8_t expected_data[CAN_RECEIVE_DATA_LENGTH] = {i, i, i, i, i, i, i, i};
        // Receive message and print message data
        ret = twai_receive(&message, pdMS_TO_TICKS(2500));
        if (ret != ESP_OK) {
            snprintf(msg, MAX_MSG_SIZE, "ERR: Receive failed: %s.\n", esp_err_to_name(ret));
            reporter->report_status(reporter, msg);
            sync_data = true;
        } else if (message.data_length_code != CAN_RECEIVE_DATA_LENGTH) {
            snprintf(msg, MAX_MSG_SIZE, "ERR: Got wrong data length! Expected %d, got %d.\n", CAN_RECEIVE_DATA_LENGTH,
                message.data_length_code);
            reporter->report_status(reporter, msg);
            sync_data = true;
        } else if (sync_data == true) {
            i = message.data[0];
            sync_data = false;
        } else if (memcmp(message.data, &expected_data, CAN_RECEIVE_DATA_LENGTH) != 0) {
            reporter->report_status(reporter, "ERR: Got wrong data!\n");
            sync_data = true;
        } else {
            handle_priv->handle_data.rx_msg_counter++;
            handle_priv->handle_data.total_rx_data += 8;
        }
        i++;
    }

    xSemaphoreTake(handle_priv->handle_data.driver_stop, portMAX_DELAY);

    twai_stop();
    ESP_LOGI(TAG, "Driver stopped");

    vTaskDelete(NULL);
}

static void can_test_control_task(void *arg)
{
    quali_can_test_handle_priv_t *handle_priv = (quali_can_test_handle_priv_t *)arg;
    test_status_report_handle_t *reporter = handle_priv->handle_data.reporter;

    /* install TWAI driver */
    ESP_ERROR_CHECK(twai_driver_install(
        handle_priv->handle_data.g_config, handle_priv->handle_data.t_config, handle_priv->handle_data.f_config));
    ESP_LOGI(TAG, "Driver installed");

    while (!handle_priv->handle_data.stop_restart) {
        ESP_LOGI(TAG, "Wait for start...");
        vTaskDelay(pdMS_TO_TICKS(100));
        reporter->wait_for_start(reporter);
        /* reset stop test flag */
        handle_priv->handle_data.stop_test = false;

        if (xTaskCreate(&can_rx_report_task, "can_rx_report", CAN_REPORT_THREAD_STACK_SIZE, (void *)handle_priv, 5,
                NULL) != pdPASS) {
            reporter->report_status(reporter, "ERR: could not start can rx report task!\n");
            handle_priv->handle_data.stop_test = true;
            continue;
        }
        if (xTaskCreate(&can_receive_task, "can_receive", CAN_RECEIVE_THREAD_STACK_SIZE, (void *)handle_priv, 5,
                NULL) != pdPASS) {
            reporter->report_status(reporter, "ERR: could not start can receive task!\n");
            handle_priv->handle_data.stop_test = true;
            continue;
        }
        if (handle_priv->handle_data.g_config->mode != TWAI_MODE_LISTEN_ONLY) {
            if (xTaskCreate(&can_tx_report_task, "can_tx_report", CAN_REPORT_THREAD_STACK_SIZE, (void *)handle_priv, 5,
                    NULL) != pdPASS) {
                reporter->report_status(reporter, "ERR: could not start can tx report task!\n");
                handle_priv->handle_data.stop_test = true;
                continue;
            }
            if (xTaskCreate(&can_transmit_task, "can_transmit", CAN_TRANSMIT_THREAD_STACK_SIZE, (void *)handle_priv, 5,
                    NULL) != pdPASS) {
                reporter->report_status(reporter, "ERR: could not start can transmit task!\n");
                handle_priv->handle_data.stop_test = true;
                continue;
            }
        } else {
            // notify receive task that driver is ready to stop
            xSemaphoreGive(handle_priv->handle_data.driver_stop);
        }

        ESP_LOGI(TAG, "Wait for stop...");
        reporter->wait_for_stop(reporter);
        /* stop test */
        handle_priv->handle_data.stop_test = true;
    }

    vTaskDelete(NULL);
}

esp_err_t can_stop_restart(quali_can_test_handle_t *handle)
{
    quali_can_test_handle_priv_t *handle_priv = (quali_can_test_handle_priv_t *)handle;
    handle_priv->handle_data.stop_restart = true;
    return ESP_OK;
}

esp_err_t new_quali_can_instance(quali_can_test_handle_t **return_handle, test_status_report_handle_t *reporter,
    const twai_timing_config_t *t_config, const twai_filter_config_t *f_config, const twai_general_config_t *g_config,
    uint32_t msg_id)
{
    /* create handle */
    quali_can_test_handle_priv_t *handle = malloc(sizeof(quali_can_test_handle_priv_t));
    if (handle == NULL) {
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
    handle->handle_data.total_tx_data = 0;
    handle->handle_data.tx_msg_counter = 0;
    handle->handle_data.total_rx_data = 0;
    handle->handle_data.rx_msg_counter = 0;
    handle->handle_data.stop_test = false;
    handle->handle_data.stop_restart = false;
    handle->handle_data.driver_start = xSemaphoreCreateBinary();
    handle->handle_data.driver_stop = xSemaphoreCreateBinary();

    xTaskCreate(
        &can_test_control_task, "can_test_control", CAN_TEST_CONTROL_THREAD_STACK_SIZE, (void *)handle, 5, NULL);
    *return_handle = (quali_can_test_handle_t *)handle;
    return ESP_OK;
}
