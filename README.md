# esp8266
开发环境：eclipse
环境的搭建以及工程的配置可参考安信可官网https://www.ai-thinker.com/home
SDK：ESP8266_RTOS-2.0.0     基于freertos操作系统

软件代码实现功能介绍：
1 给wifi配置网络
  先将wifi配置成ap模式，让该wifi形成局域网服务功能，通过手机或笔记本的web网页访问该wifi，
输入当前可用路由器的用户名和密码给该wifi。然后wifi设置成station模式，通过该用户名和密码连接上路由器。
2 获取公司的mqtt服务器的用户名和密码等信息
  创建socket套接字，通过URL解析出域名和端口，并填入sockaddr_in结构体中，之后connect和send，注意发送时，
使用C语言拼接时产生的格式问题，从recv返回的字符串中获取用户名和密码等信息。
3 连接mqtt服务器
  填入连接信息，也可添加mqtt的遗愿设置，然后发起请求。
连接成功后，要设置好订阅和发布的主题以及内容，为了加密信息传输，使用tls连接。
4 数据的传输格式
  采用messagepack，因为它比json更小。从官网下载源码，通过xtensa-lx106-elf-gcc工具链编译成库，加入到工
程中，通过头文件调用函数，拼接成协议中的数据格式，进行传输。

测试工具
1 messagepack的键值对的格式拼接，在vs下测试测试完成后，包括序列化与反序列化函数的封装，
再移植到ecilipse的工程下。
2 mqtt的测试使用MQTTBOX进行订阅与发布，MQTTBOX的ip和端口来源，需要自已搭建测试服务器，
当时是基于apache apollo来搭建的，此外还可以使用emqttd服务器搭建。
3 http格式验证，使用postman

