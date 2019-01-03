/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "esp_common.h"

#define SSID         "espressif"        /* Wi-Fi SSID */
#define PASSWORD     "1234567890"     /* Wi-Fi Password */

#define MQTT_BROKER  "iot.eclipse.org"  /* MQTT Broker Address*/
#define MQTT_PORT    1883             /* MQTT Port*/



/****************************************************/
//set up ap mode : username and password
#define AP_USERNAME		"WULIAN"
#define AP_PASSWORD 	"wulian"

extern char wifi_name[20];
extern char wifi_pwd[20];
/****************************************************/


/****************************************************/
//configure net return status : fail\succedd\save\empty
#define CONF_NET_FAIL   "conf_net_fail"
#define CONF_NET_SUCC   "conf_net_succ"

#define DEV_ON_LINE		"1"
#define DEV_LEAVE_LINE  "0"
/****************************************************/
#define QUEUE_LENGTH    5
#define QUEUE_SIZE      100

/****************************************************/
//user extern declatation
extern os_timer_t wifi_connect_timer;
extern void wifi_connect_timer_handle(void *timer_arg);

extern os_timer_t config_net_status_timer;
extern void config_net_status_timer_handle(void *timer_arg);

extern os_timer_t user_init_timer;
extern void user_init_timer_handle(void *timer_arg);

extern os_timer_t test_timer;
extern void test_timer_handle(void *timer_arg);

extern xTaskHandle http_server_handle;
extern xTaskHandle wifi_mode_handle;

/****************************************************/
extern void user_set_softap_config(void);
extern void user_set_softap_ip(void);
extern void user_get_macaddr(char *macStr);


extern void wifi_event_handler_cb(System_Event_t *event);
extern void http_server_thread(void* pvParameters);

/*****************************************************/
typedef struct HttpMsg
{
    char *pwd;
    char *user;
    char *masterDomain;
    char *masterPort;
    char *slaveDomain;
    char *slavePort;
    char *protocol;
    char *clientId;
}HttpMsg;

extern HttpMsg Http_msg;

/***************************************************/
extern void uart0_rx_handle();

#endif

