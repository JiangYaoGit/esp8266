#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt/MQTTClient.h"

#include "esp_common.h"
#include "user_config.h"

#include "espconn.h"
#include "lwip/sockets.h"
#include "user_config.h"

/***********************************************************************/
#define HTTP_CLIENT_THREAD_NAME         "http_client_thread"
#define HTTP_CLIENT_THREAD_STACK_WORDS  2048
#define HTTP_CLIENT_THREAD_PRIO         4

/***********************************************************************/
//URL  http://testdemo.wulian.cc:32040/api/wifi/connect/600194220129
#define HTTP_DOMAIN "testdemo.wulian.cc"
#define HTTP_PORT 32040
#define HTTP_Addr "/api/wifi/connect/"
#define BODY "{\"softVer\":\"3.5.12\"}"

/***********************************************************************/
#define PWD 					"pwd"
#define USER 					"user"
#define MASTERDOMAIN 			"masterDomain"
#define MASTERPORT 				"masterPort"
#define SLAVEDOMAIN 			"slaveDomain"
#define SLAVEPORT 				"slavePort"
#define PROTOCOL 				"protocol"
#define CLIENTID  				"clientId"
#define FORMAT					"\":\""
/***********************************************************************/
#define HTTPBUFSIZE 1024
HttpMsg Http_msg;

LOCAL xTaskHandle http_client_handle;

u32_t host_ip = 0;

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;
    if (ipaddr != NULL)
    {
    	os_printf("user_esp_platform_dns_found:%d.%d.%d.%d\n", *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
    			*((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
        host_ip = ipaddr->addr;
    }
}
void dns_parsing(ip_addr_t* server_ip) {
	struct espconn *pespconn;
	espconn_gethostbyname(pespconn, HTTP_DOMAIN,server_ip,user_esp_platform_dns_found);
}

void Get_http_msg(char * input,int *len)
{
    char i = 0;
    while (*(input + i) != '\"')
    {
        i++;
    }
    //os_printf("%d\n", i);
    *len = i;
}

static os_timer_t http_time;
void http_handle()
{
	mqtt_init();
}

static void http_client_thread(void* pvParameters)
{
	//创建套接字
	int sock;
	if( (sock= socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		os_printf("create socket failed\n");
		goto exit;
	}
	else
		os_printf("create socket succeed\n");
	//向服务器（特定的IP和端口）发起请求
    ip_addr_t esp_server_ip;
    dns_parsing(&esp_server_ip);

    //等待解析出主机IP地址
    while(host_ip == 0)
    {
        os_printf("wait host_dns_parsing ...\n");
        vTaskDelay(1000 / portTICK_RATE_MS);  //send every 2 seconds
    }

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));  	        	 //每个字节都用0填充
	serv_addr.sin_family = AF_INET;  						 //使用IPv4地址
	serv_addr.sin_addr.s_addr = host_ip;
	//serv_addr.sin_addr.s_addr = inet_addr("222.190.121.158"); 		 //具体的IP地址 "222.190.121.158"
	serv_addr.sin_port = htons(HTTP_PORT); 						//端口
	if( connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		os_printf("link http_server failed\n");
		goto exit;
	}
	else
		os_printf("link http_server succeed\n");

    char sndBuf[HTTPBUFSIZE];
    memset(sndBuf, 0, sizeof(sndBuf));
    char macAddr[20] = { 0 };
    user_get_macaddr(macAddr);

    sprintf(sndBuf, "POST %s%s HTTP/1.1\n", HTTP_Addr, macAddr);
    sprintf(sndBuf + strlen(sndBuf), "Host: %s:%d\n", HTTP_DOMAIN, HTTP_PORT);
	//strcat(sndBuf, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\n");// body数据类型：    |Content-Type|冒号|xxxxxxxx|分号|xxxxxx|换行符号|
    strcat(sndBuf, "Content-Type: text/plain; charset=UTF-8\n");
    sprintf(sndBuf + strlen(sndBuf), "Content-Length: %d\n",strlen(BODY));
	strcat(sndBuf, "\n");
    strcat(sndBuf, BODY);

    //sendmsg recvmsg read
	if( send(sock,sndBuf,strlen(sndBuf), 0) < 0)
	{
		os_printf("send http_msg error...\n");
		goto exit;
	}
	else
		os_printf("send http_msg succeed...\n");

	while(1){
		//读取服务器传回的数据
        char buf[1024] = { 0 };
		int rec_len = 0;
		int msg_len = 0;
		char * ptr;

		if((rec_len = recv(sock, buf, sizeof(buf),0)) == -1) {
			os_printf("Receiver http_msg erro...\n");
		}
		else{
			static char Http_revBuf[HTTPBUFSIZE];
			memset(Http_revBuf, 0, sizeof(Http_revBuf));
            buf[rec_len]  = '\0';
			//os_printf("Received:%s\n", buf);
			os_printf("Rec_len=%d\n",rec_len);
			os_printf("Receiver http_msg succedd...\n");

#define masterDomain_name "masterDomain\":\""
			ptr = strstr(buf, "masterDomain");
            Http_msg.masterDomain = ptr + strlen(masterDomain_name);
            Get_http_msg(Http_msg.masterDomain,&msg_len);
            memccpy(Http_revBuf, Http_msg.masterDomain, '\"',msg_len);
            Http_msg.masterDomain = Http_revBuf;
            os_printf("%s\n", Http_msg.masterDomain);
            rec_len += msg_len + 5;

#define masterPort_name "masterPort\":\""
			ptr = strstr(buf, "masterPort");
            Http_msg.masterPort = ptr + strlen(masterPort_name);
            Get_http_msg(Http_msg.masterPort,&msg_len);
            memccpy(Http_revBuf + rec_len, Http_msg.masterPort, '\"',msg_len);
            Http_msg.masterPort = &Http_revBuf[rec_len];
            os_printf("%s\n", Http_msg.masterPort);
            rec_len += msg_len + 5;

#define protocol_name "protocol\":\""
			ptr = strstr(buf, "protocol");
            Http_msg.protocol = ptr + strlen(protocol_name);
            Get_http_msg(Http_msg.protocol,&msg_len);
            memccpy(Http_revBuf + rec_len, Http_msg.protocol, '\"',msg_len);
            Http_msg.protocol = &Http_revBuf[rec_len];
            os_printf("%s\n", Http_msg.protocol);
            rec_len += msg_len + 5;

#define clientId_name "clientId\":\""
            ptr = strstr(buf, "clientId");
            Http_msg.clientId = ptr + strlen(clientId_name);
            Get_http_msg(Http_msg.clientId,&msg_len);
            memccpy(Http_revBuf + rec_len, Http_msg.clientId, '\"',msg_len);
            Http_msg.clientId = &Http_revBuf[rec_len];
            os_printf("%s\n", Http_msg.clientId);
            rec_len += msg_len + 5;

#define user_name "user\":\""
			ptr = strstr(buf, "user");
            Http_msg.user = ptr + strlen(user_name);
            Get_http_msg(Http_msg.user,&msg_len);
            memccpy(Http_revBuf + rec_len, Http_msg.user, '\"',msg_len);
            Http_msg.user = &Http_revBuf[rec_len];
            os_printf("%s\n", Http_msg.user);
            rec_len += msg_len + 5;

#define pwd_name "pwd\":\""
            ptr = strstr(buf, "pwd");
            Http_msg.pwd = ptr + strlen(pwd_name);
            Get_http_msg(Http_msg.pwd,&msg_len);
            memccpy(Http_revBuf + rec_len, Http_msg.pwd, '\"',msg_len);
            Http_msg.pwd = &Http_revBuf[rec_len];
            os_printf("%s\n", Http_msg.pwd);
            rec_len += msg_len + 5;

            os_timer_disarm(&http_time);
            os_timer_setfn(&http_time, http_handle,NULL);
            os_timer_arm(&http_time,500,0);
			break;
		}
		vTaskDelay(1000 / portTICK_RATE_MS);  //send every 2 seconds
	}
	//关闭套接字
	close(sock);

	exit:
    os_printf("http_client_thread going to be deleted\n");
    vTaskDelete(NULL);
    return;
}

void http_clint_init(void)
{
    int ret = 0;
    ret = xTaskCreate(http_client_thread,
        HTTP_CLIENT_THREAD_NAME,
        HTTP_CLIENT_THREAD_STACK_WORDS,
        NULL,
        HTTP_CLIENT_THREAD_PRIO,
        &http_client_handle);

    if (ret != pdPASS)  {
    	os_printf("Create http client thread failed\n");
    }
}
