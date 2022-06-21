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
#include "lwip/sockets.h"
#include "quali_can_test.h"
#include "test_status_report.h"
#include "tinyusb.h"
#include "tusb_cdc_ecm.h"

static const char *TAG = "can-test-example";

#define TX_GPIO_NUM 21
#define RX_GPIO_NUM 33

/* configure cdc-ecm driver ip */
static const ip_addr_t ipaddr = IPADDR4_INIT_BYTES(192, 168, 7, 1);
static const ip_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);
static const ip_addr_t gateway = IPADDR4_INIT_BYTES(0, 0, 0, 0);

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);

static void initialize_cdc_ecm_interface(void)
{
    /* setup CDC-ECM interface*/
    tinyusb_config_t tusb_cfg = {.descriptor = NULL, .string_descriptor = NULL, .external_phy = false};

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_ethernet_over_usb_t tusb_ethernet_over_usb_cfg = {
        .ipaddr = ipaddr, .netmask = netmask, .gateway = gateway, .mac_address = {}  // MAC address is defined later
    };

    /* get pre-programmed ethernet MAC address */
    esp_read_mac(tusb_ethernet_over_usb_cfg.mac_address, ESP_MAC_ETH);

    ESP_ERROR_CHECK(tusb_ethernet_over_usb_init(tusb_ethernet_over_usb_cfg));
}

void app_main(void)
{
    test_status_report_handle_t *reporter;
    quali_can_test_handle_t *can_test;
    int port = 10000;

    /* setup cdc ecm interface */
    initialize_cdc_ecm_interface();

    /* enable ENDC_DC */
    gpio_set_direction(GPIO_NUM_34, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_34, 1);
    /* disable CAN_SILENT */
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_18, 0);

    ESP_ERROR_CHECK(new_test_status_report_instance(&reporter, port));
    ESP_ERROR_CHECK(new_quali_can_instance(&can_test, reporter, &t_config, &f_config, &g_config, 0xA3));
}
