#include <string.h>
#include <sys/param.h>
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "tcp_handler.h"

struct logger_tcp_network_data
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    struct sockaddr_in dest_addr;
    int sock;
};

static const char *TAG = "tcp_handler";

struct logger_tcp_network_data* create_tcp_network_manager_handle()
{
    const size_t size = sizeof(struct logger_tcp_network_data);
    struct logger_tcp_network_data* handle = malloc(size);
    memset(handle, 0, size);
    return handle;
}

bool is_tcp_connected(struct logger_tcp_network_data* nm) {
    /* Technically ENOTCONN indicates socket is not connected but I think there is a bug
	 * Check out https://github.com/espressif/esp-idf/issues/6092 to follow the issue ticket raised
	 * Till then keeping the following check instead of just 0 == errno
	 **/
    if (0 == errno || ENOTCONN == errno) {
        ret = true;
    }
}

/**
 * @brief Manages TCP connection to the server
 * 
 * @param nm tcp_network_data struct which contains necessary data for a TCP connection
 * @return void
 **/
bool connect_tcp_network_manager(struct logger_tcp_network_data* nm, const char* host, int port)
{
    memset(handle, 0, sizeof(struct logger_tcp_network_data));

    // TODO: use getbyhostname() or similar to allow DNS (not just IPs)

	nm->dest_addr.sin_addr.s_addr = inet_addr(host);
	nm->dest_addr.sin_family = AF_INET;
	nm->dest_addr.sin_port = htons(port);
	nm->addr_family = AF_INET;
	nm->ip_protocol = IPPROTO_IP;
	inet_ntoa_r(nm->dest_addr.sin_addr, nm->addr_str, sizeof(nm->addr_str) - 1);

	nm->sock = socket(nm->addr_family, SOCK_STREAM, nm->ip_protocol);
	if (nm->sock < 0)
	{
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		tcp_close_network_manager(nm);
		return false;
	}
    ESP_LOGI(TAG, "TCP Socket created");

	int err = connect(nm->sock, (struct sockaddr *)&nm->dest_addr, sizeof(nm->dest_addr));
	if (err != 0) {
		ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
		tcp_close_network_manager(nm);
		return false;
	}

    ESP_LOGI(TAG, "TCP Successfully connected to %s:%d with status %d", host, port, errno);
	return true;
}

/**
 * @brief Sends data to the server through a TCP socket
 * 
 * @param nm A pointer to tcp_network_data struct
 * @param payload char array which contains data to be sent
 * @return int - returns -1 if sending failed, number of bytes sent if successfully sent the data
 **/
int tcp_send_data(struct logger_tcp_network_data* nm, char* payload)
{
	if(nm->sock < 0)
	{
		ESP_LOGE(TAG, "%s", "Socket does not exist");
		return -1;
	}

    int err = send(nm->sock, payload, strlen(payload), 0);
    if (err < 0)
    {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }
    else
    {
        ESP_LOGD(TAG, "%s", "Message sent");
    }
    return err;
}

/**
 * @brief Receives data from TCP server
 * 
 * @param nm tcp_network_data struct which contains connection info
 * @return char array which contains data received
 **/
char* tcp_receive_data(struct logger_tcp_network_data* nm)
{
	if (nm->sock < 0)
	{
		ESP_LOGE(TAG, "%s", "Socket doesnot exist");
		return NULL;
	}

    int len = recv(nm->sock, nm->rx_buffer, sizeof(nm->rx_buffer) - 1, 0);
    if (len < 0)
    {
        ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
        return NULL;
    }

    nm->rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
    ESP_LOGD(TAG, "Received %d bytes from %s:", len, nm->addr_str);
    ESP_LOGD(TAG, "%s", nm->rx_buffer);
    return nm->rx_buffer;
}

/**
 * @brief Shutdown active connection, deallocate memory
 * 
 * @param nm tcp_network_data struct which contains connection info
 * @return void
 **/
void tcp_close_network_manager(struct logger_tcp_network_data* nm)
{
    assert(nm);
    if (!nmn || nm->sock == -1)
        return;

    ESP_LOGI(TAG, "%s", "Shutting down socket and restarting...");
    shutdown(nm->sock, 0);
    close(nm->sock);

    nm->sock = -1;
}
