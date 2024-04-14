// TODO: this file is pretty busted up. fix it.
//  look at UDP handler for template

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"

#include "include/websocket_handler.h"

struct websocket_network_manager {
    esp_websocket_client_handle_t network_handle;
};

/*
 * 0x00: this frame continues the payload from the last.
 * 0x01: this frame includes utf-8 text data.
 * 0x02: this frame includes binary data.
 * 0x08: this frame terminates the connection.
 * 0x09: this frame is a ping.
 * 0x0a: this frame is a pong.
 *
 * A ping or pong is just a regular frame, but it's a control frame.
 * Pings have an opcode of 0x9, and pongs have an opcode of 0xA.
 * When you get a ping, send back a pong with the exact same Payload Data
 * as the ping (for pings and pongs, the max payload length is 125).
 * You might also get a pong without ever sending a ping; ignore this if it happens.
 *
 * https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
 */

typedef enum websocket_op {
    websocket_op_continue_payload = 0,
    websocket_op_utf_8_text = 1,
    websocket_op_binary_data = 2,
    websocket_op_termination_frame = 8,
    websocket_op_ping_frame = 9,
    websocket_op_pong_frame = 10,
}websocket_op_t;

static const char* TAG = "websocket_handler";

/**
 * @brief Websocket event handler
 * 
 * @param handler_args args to be passed to handler
 * @param base event base 
 * @param event_id event id
 * @param event_data event data
 */
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");


            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            break;

        case WEBSOCKET_EVENT_DATA:
        	  //Avoid printing logs if it is just a ping/pong frame.
        	  if(websocket_op_ping_frame != data->op_code && websocket_op_pong_frame != data->op_code)
        	  {
        		  ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        		  ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        		  ESP_LOGW(TAG, "Received=%.*s\r\n", data->data_len, (char*)data->data_ptr);
        	  }
            break;
        
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
    }
}

struct websocket_network_manager* create_websocket_network_manager_handle()
{
    const size_t size = sizeof(struct websocket_network_manager);
    struct websocket_network_manager* handle = malloc(size);
    memset(handle, 0, size);
    return handle;
}

/**
 * @brief start the websocket client and connect to the server
 * 
 * @return esp_websocket_client_handle_t return a websocket client handle, used to communicate with server
 */
struct websocket_network_manager* init_websocket_network_manager()
{
	ESP_LOGI(TAG, "Connecting to %s...", WEBSOCKET_HOST_URI);

    struct websocket_network_manager* nm =

	esp_websocket_client_handle_t network_handle;

	const esp_websocket_client_config_t websocket_cfg = {
			.uri = WEBSOCKET_HOST_URI,
	};

	network_handle = esp_websocket_client_init(&websocket_cfg);
	esp_websocket_register_events(network_handle, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)network_handle);

	esp_websocket_client_start(network_handle);

	return network_handle;
}

ret = esp_websocket_client_is_connected((esp_websocket_client_handle_t) handle_t)

/**
 * @brief Sends data to the websocket server
 * 
 * @param network_handle network handle returned by network manager
 * @param payload data to be sent to the server
 * @return int returns number of bytes sent, -1 if any error occurs in sending
 */
int websocket_send_data(esp_websocket_client_handle_t network_handle, char* payload)
{

	if (esp_websocket_client_is_connected(network_handle))
	{
		int err = esp_websocket_client_send(network_handle, payload, strlen(payload), portMAX_DELAY);

		if (err < 0)
		{
			ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
		}
		else
		{
			ESP_LOGD(TAG, "%s", "Message sent");
		}

		return err;
	}

	return -1;
}

/**
 * @brief stop and destroy websocket client
 * 
 * @param network_handle client handle that needs to be stopped and destroyed
 */
void websocket_close_network_manager(esp_websocket_client_handle_t network_handle)
{
	esp_websocket_client_stop(network_handle);
	esp_websocket_client_destroy(network_handle);
	ESP_LOGI(TAG, "Websocket Stopped");
}
