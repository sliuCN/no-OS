#include <mqtt_client.h>
#include "tcp_socket.h"
#include "error.h"
#include "string.h"
#include "uart_extra.h"
#include "irq_extra.h"
#include "delay.h"
#include <stdio.h>
#include "debug.h"

char		*mqtt_server = "192.168.1.5";
uint16_t	mqtt_port = 1883;
char		*topic_to_suscribe = "my_subscribe";
char		*topic_to_publish = "my_publish";

int32_t init_socket(struct tcp_socket_desc **sock);
int32_t clean_socket(struct tcp_socket_desc *sock);

#define BUFF_SIZE 100
uint8_t send_buff[BUFF_SIZE];
uint8_t read_buff[BUFF_SIZE];

void message_handler(struct mqtt_message_data *msg)
{
	msg->message.payload[msg->message.len] = 0;
	printf("Topic:%s -- Payload: %s\n", msg->topic, msg->message.payload);
}

int32_t example_mqtt_main()
{
	int32_t			ret;
	struct tcp_socket_desc	*sock;
	struct mqtt_desc	*mqtt;
	struct mqtt_init_param	mqtt_init_param = {
		.extra_timer_init_param = NULL,
		.command_timeout_ms = 20000,
		.send_buff = send_buff,
		.read_buff = read_buff,
		.send_buff_size = BUFF_SIZE,
		.read_buff_size = BUFF_SIZE,
		.message_handler = message_handler
	};
	struct socket_address		server;

	ret = init_socket(&sock);
	ASSERT_AND_RET(ret, "Fail to init socket\n");

	server.addr = mqtt_server;
	server.port = mqtt_port;
	if (!IS_ERR_VALUE(ret)) //For debugging purposes
		ret = socket_connect(sock, &server);
	ASSERT_AND_RET(ret, "Socket connect failed\n");

	mqtt_init_param.sock = sock;
	ret = mqtt_init(&mqtt, &mqtt_init_param);
	ASSERT_AND_RET(ret, "Mqqt init failed\n");

	struct mqtt_connect_config conn_config = {
		.version = MQTT_VERSION_3_1,
		.keep_alive_ms = 7200,
		.client_name = (int8_t *)"noos-client",
		.username = NULL,
		.password = NULL
	};
	ret = mqtt_connect(mqtt, &conn_config, NULL);
	ASSERT_AND_RET(ret, "Mqqt connect failed\n");

	struct mqtt_message msg = {
		.qos = MQTT_QOS0,
		.retained = true,
		.payload = (uint8_t *)"Salut\n"
	};
	msg.len = strlen((char *)msg.payload);
	mqtt_publish(mqtt, (int8_t *)topic_to_publish, &msg);

	mqtt_subscribe(mqtt, (int8_t *)topic_to_suscribe, MQTT_QOS0, NULL);

	while (true)
		mqtt_yield(mqtt, 1000);

	mqtt_remove(mqtt);
	clean_socket(sock);
}
