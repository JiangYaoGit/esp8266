/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
 /* This program compiles for Sparc Solaris 2.6.
  * To compile for Linux:
  *  1) Comment out the #include <pthread.h> line.
  *  2) Comment out the line that defines the variable newthread.
  *  3) Comment out the two lines that run pthread_create().
  *  4) Uncomment the line that runs accept_request().
  *  5) Remove -lsocket from the Makefile.
  */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_common.h"
#include "espconn.h"
#include "lwip/sockets.h"
#include "user_config.h"


/***********************************************************************/

int config_net_status_count = 0;
/***********************************************************************/

typedef enum {
	accept_empty,
	accept_succeed
}accept_parse;
/*********************************************************************/

#define USER_HTML	"<!DOCTYPE html>											\
					<html>														\
						<head><title>WuLian</title></head> 						\
					<body>														\
						<br><br><br><br><br><br>								\
						<form method='get' action='wulian'> 					\
						<h2 style='text-align:center'>							\
							username:<input type='text' name='user'><br>		\
							password:<input type='password' name='password'><br> \
							<input type='submit' value='Submit'>				\
							<input type='reset' value='Clear'></h2>				\
						</form>													\
					</body>														\
					</html>"															




#define USER_SEND_OK	"<!DOCTYPE html>											\
						<html>														\
							<head><title>WULIAN</title></head> 						\
							<body>													\
								<h1 style='text-align:center'>send-ok</h1>			\
							</body>													\
						</html>"
/*********************************************************************/

void user_html_interface(int client)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.1 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html; charset=utf-8\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Length: %d\r\n",strlen(USER_HTML));
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "%s\r\n", USER_HTML);
	send(client, buf, strlen(buf), 0);
}

void user_html_send_ok(int client)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.1 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html; charset=utf-8\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Length: %d\r\n",strlen(USER_SEND_OK));
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "%s\r\n", USER_SEND_OK);
	send(client, buf, strlen(buf), 0);
}

/************************************************************************/

accept_parse server_accept_parse(int client_sock, char *user, char *pwd)
{
	char buf[1024] = {0};
	int numchars;
	read(client_sock, buf, sizeof(buf));

	if (strstr(buf, "GET") != NULL)
	{
		char * user_t = strstr(buf,"user");
		char * pwd_t = strstr(buf,"password");
		if(user_t != NULL && pwd_t != NULL)
		{
			char userBuf[20] = {0};
			char pwdBuf[20] = {0};
			memccpy(userBuf, user_t+strlen("user="), '&', sizeof(userBuf));
			memccpy(pwdBuf, pwd_t+strlen("password="), ' ', sizeof(pwdBuf));
			if(userBuf[0] != '&' && pwdBuf[0] != ' ')
			{
				os_printf("accept user and password\n");

				snprintf(user, strlen(userBuf), "%s", userBuf);
				snprintf(pwd, strlen(pwdBuf), "%s", pwdBuf);
				user_html_send_ok(client_sock);
				return accept_succeed;
			}
			else
			{
				os_printf("input user or password is empty\n");
				user_html_interface(client_sock);
				return accept_empty;
			}
		}
		else if(user_t == NULL || pwd_t == NULL)
		{
			os_printf("request input interface\n");
			user_html_interface(client_sock);
			return accept_empty;
		}
	}
	else if(strstr(buf, "POST") != NULL)
	{
		os_printf("post request----Waiting for development\n");
		return accept_empty;
	}
	else
	{
		//os_printf("request mothod err\n");
		user_html_interface(client_sock);
		return accept_empty;
	}
}
/*********************************************************************/




/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
 /**********************************************************************/
void startup(int * sock)
{
	int httpd = 0;
	if( (httpd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		os_printf("create web server socket failed\n");
		return;
	}

	struct sockaddr_in name;
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(80);
	name.sin_addr.s_addr = INADDR_ANY;
	if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
	{
		os_printf("bind web server socket failed\n");
		return;
	}
	if (listen(httpd, 5) < 0)
	{
		os_printf("listen web server socket failed\n");
		return;
	}
	*sock = httpd;
}

/******************************************************/
void user_config_net_init(void)
{
	//创建套接字
	printf("---create server socket----\n");
	int server_sock = -1;
	int client_sock = -1;
	struct sockaddr_in client_name;
	socklen_t client_name_len = sizeof(client_name);

	startup(&server_sock);
	//os_printf("running on server_sock %d\n", server_sock);

	while (1)
	{
		for(;;)
		{
			client_sock = accept(server_sock,(struct sockaddr *)&client_name,&client_name_len);
			if(client_sock < 0)
			{
				os_printf("accept failed\n");
			}
			else
			{
				os_printf("accept succed\n");
				break;
			}
		}

		accept_parse sta;
		memset(wifi_name, 0, sizeof(wifi_name));
		memset(wifi_pwd, 0, sizeof(wifi_pwd));
		if( (sta = server_accept_parse(client_sock, wifi_name, wifi_pwd) ) == accept_succeed)
		{
		    config_net_status_count = 0;
		    os_timer_disarm(&config_net_status_timer);

		    os_timer_disarm(&wifi_connect_timer);
		    os_timer_setfn(&wifi_connect_timer, wifi_connect_timer_handle, CONF_NET_SUCC);
		    os_timer_arm(&wifi_connect_timer,500,0);

		    vTaskSuspend(http_server_handle);
		}
		close(client_sock);
	}

	vTaskDelay(100 / portTICK_RATE_MS);
	close(server_sock);
	return;
}

/******************************************************/

void http_server_thread(void* pvParameters)
{
	os_printf("http server thread start\n");

	//input wifi name and password
	user_config_net_init();

	os_printf("going to delete http server thread\n");
    vTaskDelete(NULL);
    return;
}


os_timer_t config_net_status_timer;
void config_net_status_timer_handle(void *timer_arg)
{
	uint8 num = 0;
	num = wifi_softap_get_station_num();
	if(num != 0)
	{
		os_printf("wating config net\n", num);
		if(++config_net_status_count > 4)    //>4 打印5次
		{
			config_net_status_count = 0;

			os_printf("config net input over time\n", num);
			vTaskSuspend(http_server_handle);
		    os_timer_disarm(&config_net_status_timer);

		    os_timer_disarm(&wifi_connect_timer);
		    os_timer_setfn(&wifi_connect_timer, wifi_connect_timer_handle, CONF_NET_FAIL);
		    os_timer_arm(&wifi_connect_timer,500,0);
		}
	}
	else
	{
		config_net_status_count = 0;

		os_printf("link ap over time\n", num);
		vTaskSuspend(http_server_handle);
	    os_timer_disarm(&config_net_status_timer);

	    os_timer_disarm(&wifi_connect_timer);
	    os_timer_setfn(&wifi_connect_timer, wifi_connect_timer_handle, CONF_NET_FAIL);
	    os_timer_arm(&wifi_connect_timer,500,0);
	}

}

/**********************************************************************************/


