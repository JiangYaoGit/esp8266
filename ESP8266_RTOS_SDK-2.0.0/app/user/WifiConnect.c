#include "user_config.h"
#include <stdlib.h>
#include <string.h>

/***********************************************************************/

#define WIFI_CONNTECT_THREAD_NAME         "wifi_connect_thread"
#define WIFI_CONNTECT_THREAD_STACK_WORDS  1024
#define WIFI_CONNTECT_THREAD_PRIO         4
LOCAL xTaskHandle wifi_connect_handle;

/***********************************************************************/
char wifi_name[20] = {0};
char wifi_pwd[20] = {0};

/***********************************************************************/
void wifi_event_handler_cb(System_Event_t *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->event_id) {
        case EVENT_STAMODE_GOT_IP:
            os_printf("sta got ip ,create task and free heap size is %d\n", system_get_free_heap_size());
            os_printf("save net connect infomation to flash\n");
            http_clint_init();
            //user_conn_init();

            break;

        case EVENT_STAMODE_CONNECTED:
            os_printf("sta connected\n");
            break;

        case EVENT_STAMODE_DISCONNECTED:
        	os_printf("sta disconnected:username or password err\n");

        	user_wifi_set_opmode(SOFTAP_MODE);

    	    os_timer_disarm(&user_init_timer);
    	    os_timer_setfn(&user_init_timer, user_init_timer_handle, NULL);
    	    os_timer_arm(&user_init_timer,1000,0);
            break;

        default:
            break;
    }
}



static void wifi_connect_thread(void* pvParameters)
{
	os_printf("wifi connect thread start\n");
	//wifi_set_opmode(STATION_MODE);

	struct station_config config;
	bzero(&config, sizeof(struct station_config));

	os_printf("%s\nlen = %d\n",wifi_name, strlen(wifi_name));
	os_printf("%s\nlen = %d\n",wifi_pwd, strlen(wifi_pwd));
	if(strlen(wifi_name) == 0 || strlen(wifi_pwd) == 0)
	{
		wifi_set_opmode_current(SOFTAP_MODE);

	    os_timer_disarm(&user_init_timer);
	    os_timer_setfn(&user_init_timer, user_init_timer_handle, NULL);
	    os_timer_arm(&user_init_timer,500,0);
	}
	else
	{
		wifi_set_opmode_current(STATION_MODE);

		wifi_station_disconnect();
		sprintf(config.ssid, wifi_name);
		sprintf(config.password, wifi_pwd);
		wifi_station_set_config(&config);
		wifi_set_event_handler_cb(wifi_event_handler_cb);
		wifi_station_connect();
	}

	os_printf("wifi connect thread going to be deleted\n");
	vTaskDelete(NULL);
	return;
}


/******************************************************************/

os_timer_t wifi_connect_timer;
void wifi_connect_timer_handle(void *timer_arg)
{
	const char * buf = (char *)timer_arg;

	struct station_config config;
	bzero(&config, sizeof(struct station_config));
	if(strcmp(buf , CONF_NET_FAIL) == 0)
	{
		os_printf("configure net failed\n");

		memset(wifi_name, 0, sizeof(wifi_name));
		memset(wifi_pwd, 0, sizeof(wifi_pwd));
		wifi_station_get_config_default(&config);
		sprintf(wifi_name , config.ssid);
		sprintf(wifi_pwd, config.password);
	}
	else if(strcmp(buf , CONF_NET_SUCC) == 0)
	{
		os_printf("configure net succeed\n");
	}

	xTaskCreate(wifi_connect_thread,
			WIFI_CONNTECT_THREAD_NAME,
			WIFI_CONNTECT_THREAD_STACK_WORDS,
			NULL,
			WIFI_CONNTECT_THREAD_PRIO,
			&wifi_connect_handle);
}

/******************************************************************/

