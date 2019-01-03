/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#include "user_config.h"
#include "WlinkWifi.h"

#define TEST_THREAD_NAME         "test_thread"
#define TEST_THREAD_STACK_WORDS  2048
#define TEST_THREAD_PRIO         4
xTaskHandle test_handle;

static void test_thread(void* pvParameters);
os_timer_t test_timer;
void test_timer_handle(void *timer_arg){
	os_printf("size = %d\n", system_get_free_heap_size());

}

static void test_thread(void* pvParameters)
{
	os_printf("test thread start\n");


	os_printf("going to delete test thread\n");

//    os_timer_disarm(&test_timer);
//    os_timer_setfn(&test_timer, test_timer_handle, NULL);
//    os_timer_arm(&test_timer,2000,0);

    vTaskDelay(1000 / portTICK_RATE_MS);
    vTaskDelete(NULL);
    return;
}

/**********************************************************/
#define HTTP_SERVER_THREAD_NAME         "http_server_thread"
#define HTTP_SERVER_THREAD_STACK_WORDS  1024
#define HTTP_SERVER_THREAD_PRIO         4
xTaskHandle http_server_handle;

/**********************************************************/
#define WIFI_MODE_THREAD_NAME         "wifi_thread"
#define WIFI_MODE_THREAD_STACK_WORDS  512
#define WIFI_MODE_THREAD_PRIO         2
xTaskHandle wifi_mode_handle;

WIFI_MODE user_wifi_set_opmode(WIFI_MODE para)
{
	static WIFI_MODE mode = NULL_MODE;

	if(GET_MODE != para)
	{
		mode = para;
		return mode;
	}
	else
		return mode;
}

static void wifi_mode_thread(void* pvParameters)
{
	os_printf("wifi mode configuration thread\n");

	WIFI_MODE mode = NULL_MODE;

	mode = user_wifi_set_opmode(GET_MODE);
	if(mode == SOFTAP_MODE)
	{
		os_printf("---wifi mode ap---\n");
		wifi_set_opmode_current(SOFTAP_MODE);
	}

	os_printf("going to delete wifi mode configuration thread\n");
    vTaskDelete(NULL);
    return;
}


os_timer_t user_init_timer;
void user_init_timer_handle(void *timer_arg)
{
	os_printf("---configuration net---\n");
	vTaskResume(http_server_handle);

    xTaskCreate(wifi_mode_thread,
    		WIFI_MODE_THREAD_NAME,
			WIFI_MODE_THREAD_STACK_WORDS,
			NULL,
			WIFI_MODE_THREAD_PRIO,
			&wifi_mode_handle);

    os_timer_disarm(&config_net_status_timer);
    os_timer_setfn(&config_net_status_timer, config_net_status_timer_handle, NULL);
    os_timer_arm(&config_net_status_timer,10000,1);
}
/**********************************************************/

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
xQueueHandle xQueue1;
void user_init(void)
{
    os_printf("SDK version:%s %d\n", system_get_sdk_version(), system_get_free_heap_size());

//	xTaskCreate(test_thread,
//			TEST_THREAD_NAME,
//			TEST_THREAD_STACK_WORDS,
//			NULL,
//			TEST_THREAD_PRIO,
//			&test_handle);

	wifi_station_set_auto_connect(false);
	wifi_station_set_reconnect_policy(false);
	wifi_set_opmode_current(SOFTAP_MODE);
    user_set_softap_ip();
    user_set_softap_config();

    xTaskCreate(http_server_thread,
    		HTTP_SERVER_THREAD_NAME,
			HTTP_SERVER_THREAD_STACK_WORDS,
			NULL,
			HTTP_SERVER_THREAD_PRIO,
			&http_server_handle);

    os_timer_disarm(&user_init_timer);
    os_timer_setfn(&user_init_timer, user_init_timer_handle, NULL);
    os_timer_arm(&user_init_timer,500,0);

    xQueue1 = xQueueCreate(QUEUE_LENGTH, sizeof(char) * QUEUE_SIZE);
    if(xQueue1 != NULL)
    {
    	os_printf("create queue succeed\n");
    }


}



/**********set_up net*************************************/

void user_get_macaddr(char *macStr){
	//recommended length:char macStr[20];
	unsigned char macRev[10] = {0};

	char ret = wifi_get_macaddr(SOFTAP_IF, macRev);
	if(ret == false)
	{
		printf("get mac failed\n");
		macStr = NULL;
	}
	else
	{
		char i = 0;
		printf("get mac succed\n");
		for(i; i < strlen(macRev); i++)
		{
			sprintf(macStr + i * 2, "%02X", macRev[i]);
		}
		//printf("mac=%s\n", macStr);
	}
}

void user_set_softap_ip(void) {
	struct ip_info info;
	memset(&info, 0, sizeof(info));
	wifi_softap_dhcps_stop();  //设置之前需要关闭

	char mac[20] = {0};
	user_get_macaddr(mac);

	struct dhcps_lease lease;
	lease.start_ip.addr = ((mac[0] & 0x0f) << 24) | ((mac[1] & 0x0f) << 16) | ((mac[2] & 0x0f) << 8);
	lease.end_ip.addr = (mac[0] & 0x0f) << 24 | (mac[1] & 0x0f) << 16 | (mac[2] & 0x0f) << 8 | 0x0f;

	IP4_ADDR(&info.ip, mac[0] & 0x0f, mac[1] & 0x0f, mac[2] & 0x0f, mac[3] & 0x0f);
	IP4_ADDR(&info.gw, mac[0] & 0x0f, mac[1] & 0x0f, mac[2] & 0x0f, mac[3] & 0x0f);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);

//	struct dhcps_lease lease;
//	lease.start_ip.addr = 0xc0a80200;  //192.168.2.0
//	lease.end_ip.addr = 0xc0a80a00;    //192.168.10.0
//	wifi_softap_set_dhcps_lease(&lease);
//
//	IP4_ADDR(&info.ip, 192, 168, 3, 1);
//	IP4_ADDR(&info.gw, 192, 168, 3, 1);
//	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	wifi_set_ip_info(SOFTAP_IF, &info);

	wifi_softap_dhcps_start();
}

void user_set_softap_config(void){
	char macAddr[20] = {0};
	user_get_macaddr(macAddr);
	macAddr[4] = '\0';

	struct softap_config config;
	//wifi_softap_get_config(&config); // Get config first.
	memset(&config, 0, sizeof(config));

	sprintf(config.ssid, "%s%s", AP_USERNAME, macAddr);
	sprintf(config.password, "%s%s",AP_PASSWORD, macAddr);
	config.authmode = AUTH_WPA_WPA2_PSK;
	config.ssid_len = 0;	// or its actual length
	config.ssid_hidden = 0; //broadcast ssid
	config.beacon_interval = 100;
	config.max_connection = 2; // how many stations can connect to ESP8266 softAP at most.

	wifi_softap_set_config(&config);// Set ESP8266 softap config

//    struct station_config config;
//    bzero(&config, sizeof(struct station_config));
//    sprintf(config.ssid, SSID);
//    sprintf(config.password, PASSWORD);
//    wifi_station_set_config(&config);
//    wifi_set_event_handler_cb(wifi_event_handler_cb);
//    wifi_station_connect();
}

/**********set_up net*************************************/









/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}


