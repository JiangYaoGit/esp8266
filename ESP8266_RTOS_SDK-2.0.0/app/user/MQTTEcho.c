/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include <stddef.h>

#include "mqtt/MQTTClient.h"
#include "user_config.h"

#include "openssl/ssl.h"

//#include "ssl_client_crt.h"
//#include "cert.h"
//#include "private_key.h"

#include "WlinkWifi.h"

ssl_ca_crt_key_t ssl_cck;
#define SSL_CA_CERT_KEY_INIT(s,a,b,c,d,e,f)  ((ssl_ca_crt_key_t *)s)->cacrt = a;\
                                             ((ssl_ca_crt_key_t *)s)->cacrt_len = b;\
                                             ((ssl_ca_crt_key_t *)s)->cert = c;\
                                             ((ssl_ca_crt_key_t *)s)->cert_len = d;\
                                             ((ssl_ca_crt_key_t *)s)->key = e;\
                                             ((ssl_ca_crt_key_t *)s)->key_len = f;

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  2048
#define MQTT_CLIENT_THREAD_PRIO         8

LOCAL xTaskHandle mqttc_client_handle;

/*****wifi topic*****************************/
#define SUB_RECV			"wk/wifi/recv/"
#define PUB_REQ				"wk/wifi/req/"
#define PUB_STATE			"wk/wifi/state/"
#define PUB_DATA			"wk/wifi/data/"
#define PUB_ALARM			"wk/wifi/alarm/"

extern int mqtt_sent_len;

static os_timer_t dev_query_time;
void  dev_query_handle()
{
	uint8 var[7] = {0x48, 0x07, 0x02, 0x01, 0x00, 0x00, 0x52 };
	Uart0_Sent(var, 7);
}

static void messageArrived(MessageData* data)
{
	int i = 0;
	uint8 check_sum = 0;
	uint8 * revBuf = data->message->payload;
	int len = data->message->payloadlen;

//	WlinkInfo info;
//	Deserialize(&info,revBuf,len);
//	uint8 buf[256];

//	for(i = 0;i < len;i++)
//	{
//		buf[i] = *var++;
//		if(i ==  len - 1)
//			break;
//		else
//			check_sum += buf[i];
//	}
//	os_printf("check_sum=%d\n",check_sum);
//	//可以接受到APP的数据并发送给设备
//	//Uart0_Sent(buf,len);
//
//	if( buf[len-1] == check_sum)
//	{
//		//APP主动控制指令
//		//0x48,0x07,0x02,0x01,0x00,0x00,0x52
//		if(buf[2] == 0x02)
//		{
//			os_printf("APP-Control Instruction\n");
//
//		    os_timer_disarm(&uart0_tx_timer);
//		    os_timer_setfn(&uart0_tx_timer,uart0_tx_handle,NULL);
//		    os_timer_arm(&uart0_tx_timer,50,0);
//		}
//		//APP返回应答帧或错误帧
//		else if(buf[2] == 0x01)
//		{
//			os_printf("APP-Return Response Frame OR Error Frame:\"ESP8266-Pub\"\n");
//			Uart0_Sent(buf,len);
//		}
//	}
}

void connect_default()
{
	Http_msg.protocol = "tcp";
	//Http_msg.clientId = "esp--wifi";
	//Http_msg.masterDomain = "222.190.121.158"; 41883
	Http_msg.masterPort = "41883";
	//Http_msg.user = "admin";
	//Http_msg.pwd = "123nb";
}
static void mqtt_client_thread(void* pvParameters)
{
	os_printf("Mqtt client thread start\n");
	MQTTClient client;
    Network network;
    char connect_state = 0;
    unsigned char sendbuf[1024], readbuf[1024] = {0};
    char topicRev[30];
    pvParameters = 0;
    //connect_default();

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    connectData.MQTTVersion = 3;
    connectData.clientID.cstring = Http_msg.clientId;
    connectData.username.cstring = Http_msg.user;
    connectData.password.cstring = Http_msg.pwd;
    connectData.keepAliveInterval = 30;
    connectData.cleansession = true;
	connectData.willFlag = true;
	connectData.will.qos = QOS2;
	connectData.will.retained = true;

	memset(topicRev, 0, sizeof(topicRev));
	sprintf(topicRev,"%s%s",PUB_STATE, Http_msg.clientId);
	connectData.will.topicName.cstring = topicRev;

	char willBuf[1024] = { 0 };
	int willLen = 0;
	DevStatus(willBuf, &willLen, DEV_LEAVE_LINE);
	connectData.will.message.cstring = willBuf;

	if (strcmp(Http_msg.protocol, "tcp") == 0){
		NetworkInit(&network);
	}
	else if (strcmp(Http_msg.protocol, "ssl") == 0) {
		NetworkInitSSL(&network);
	}  
    MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

    for(;;){
		/***连接Broker和TCP/SSL***/
		int port = atoi(Http_msg.masterPort);
		//os_printf("port = %d\n",port);
		if (strcmp(Http_msg.protocol, "tcp") == 0) {
			if ((connect_state = NetworkConnect(&network, Http_msg.masterDomain, port)) != 0) {
				os_printf("Connect Broker and TCP failed\n");
				os_printf("Will be starting\n");
				system_restart();
			}
			else
			{
				os_printf("Connect Broker and TCP succeed\n");
			}
		}
		else if (strcmp(Http_msg.protocol, "ssl") == 0) {
			SSL_CA_CERT_KEY_INIT(&ssl_cck, NULL, 0, NULL, 0, NULL, 0);
			//SSL_CA_CERT_KEY_INIT(&ssl_cck,  default_certificate, default_certificate_len, NULL, 0,NULL, 0);
			if ((connect_state = NetworkConnectSSL(&network, Http_msg.masterDomain, port, &ssl_cck, TLSv1_1_client_method(), SSL_VERIFY_NONE, 2048)) != 1) {
				os_printf("Connect Broker and TLS failed\n");
				os_printf("Will be starting\n");
				system_restart();
			}
			else
			{
				os_printf("Connect Broker and TLS succeed\n");
			}
		}


		#if defined(MQTT_TASK)

			if ((connect_state = MQTTStartTask(&client)) != pdPASS) {
				os_printf("Create Mqtt_User task failed\n");
			} else {
				os_printf("Create Mqtt_User task succeed\n");
			}

		#endif

		/***建立MQTT连接***/
		if ((connect_state = MQTTConnect(&client, &connectData)) != 0) {
			os_printf("Create Mqtt Connect failed\n");
		} else {
			os_printf("Create Mqtt Connect succeed\n");
		}

		memset(topicRev, 0, sizeof(topicRev));
		sprintf(topicRev, "%s%s",SUB_RECV, Http_msg.clientId);
		if ((connect_state = MQTTSubscribe(&client, topicRev, 2, messageArrived)) != 0) {
			os_printf("MQTT Subscribe %s failed\n", topicRev);
		}
		else {
			os_printf("MQTT Subscribe %s succeed\n", topicRev);
		}

		char sendBuf[1024] = { 0 };
		int len = 0;
		DevStatus(sendBuf, &len, DEV_ON_LINE);

		MQTTMessage message;
		message.qos = QOS2;
		message.retained = 0;
		message.payload = sendBuf;
		message.payloadlen = len;
		memset(topicRev, 0, sizeof(topicRev));
		sprintf(topicRev, "%s%s", PUB_STATE, Http_msg.clientId);
		if ((connect_state = MQTTPublish(&client, topicRev, &message)) != 0) {
			os_printf("MQTT Publish %s failed\n",topicRev);
		} else {
			os_printf("MQTT Publish %s succeed\n",topicRev);

			os_timer_disarm(&dev_query_time);
			os_timer_setfn(&dev_query_time,dev_query_handle,NULL);
			os_timer_arm(&dev_query_time,60000,1);
		}


		while (1) {
			if (mqtt_sent_len != 0)
			{
				DevDataSend(sendBuf, &len);

				message.qos = QOS2;
				message.retained = 0;
				message.payload = sendBuf;
				message.payloadlen = len;
				memset(topicRev, 0, sizeof(topicRev));
				sprintf(topicRev, "%s%s", PUB_DATA, Http_msg.clientId);
				if ((connect_state = MQTTPublish(&client, topicRev, &message)) != 0) {
					os_printf("MQTT Publish %s failed\n",topicRev);
				}
				else {
					os_printf("MQTT Publish %s succeed\n",topicRev);
					mqtt_sent_len = 0;

					os_timer_disarm(&test_timer);
					os_timer_setfn(&test_timer, test_timer_handle, NULL);
					os_timer_arm(&test_timer,2000,0);
				}
			}
			if (connect_state != 0) {
				//system_restart();
				break;
			}
				//vTaskDelay(1000 / portTICK_RATE_MS);  //send every 1 seconds
		 }
    }

    os_printf("mqtt_client_thread going to be deleted\n");
    vTaskDelete(NULL);
    return;
}
static os_timer_t handshake_time;
void handshake_handle()
{
    uint8 var[11] = {0x48,0x0B,0xFE,0x01,0x01,0x01,0x01,0x01,0x05,0x00,0x5B };
    Uart0_Sent(var, 11);
}

void uart0_rx_handle()
{
	//printf("rev_count: %d \n",rev_count);
	//rev_length = rev_count - 2; //后两个字节\t\n
	int i = 0;
	uint8 check_sum = 0;
	int len = uart0_rev_length;
    uart0_rev_length = 0;
    uint8 buf[256];
    os_printf("rev_len=%d\n",len);

    if (uart0_rev_buf[2] == 0xFE && uart0_rev_buf[24] == 0xFE)
    {
    	len = 22;
    	for (i = 0; i < len; i++)
		{
			buf[i] = uart0_rev_buf[i];
			//os_printf("%d\n", buf[i]);
		}
        Uart0_Sent(buf, len);

        os_timer_disarm(&handshake_time);
        os_timer_setfn(&handshake_time,handshake_handle,NULL);
        os_timer_arm(&handshake_time,500,0);
    }
    else if(uart0_rev_buf[2] == 0x01)
    {
        for (i = 0; i < len; i++)
        {
            buf[i] = uart0_rev_buf[i];
            //os_printf("%d\n", buf[i]);
            if (i == len - 1)
                break;
            else
                check_sum += buf[i];
        }
        os_printf("check_sum=%02x\n", check_sum);

        if (buf[len - 1] == check_sum)
        {
            //设备主动上报，校验无误，原帧返回
            if (buf[2] == 0x01)
            {
                Publish(buf, len);
                //Uart0_Sent(buf, len);
            }
            //设备返回应答帧或错误帧
            else if (buf[2] == 0x02 || buf[2] == 0xFF)
            {
                Publish(buf, len);
            }
        }
    }
}


static os_timer_t open_uart0_timer;
void open_uart0_handle()
{
	printf("open uart0 recieve succeed\n");
	uart_init_new();
}

void mqtt_init(void)
{
    int ret = 0;

	os_timer_disarm(&open_uart0_timer);
	os_timer_setfn(&open_uart0_timer,open_uart0_handle,NULL);
	os_timer_arm(&open_uart0_timer,20000,0);

    ret = xTaskCreate(mqtt_client_thread,
                      MQTT_CLIENT_THREAD_NAME,
                      MQTT_CLIENT_THREAD_STACK_WORDS,
                      NULL,
                      MQTT_CLIENT_THREAD_PRIO,
                      &mqttc_client_handle);

    if (ret != pdPASS)  {
    	os_printf("Create Mqtt client thread failed\n");
    }
}
