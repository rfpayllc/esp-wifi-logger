#include <esp_log.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "udp_handler.h"

static const char *TAG = "udp_logger";

struct logger_udp_network_data
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    struct sockaddr_in dest_addr;
    int sock;
};

struct logger_udp_network_data* create_udp_network_manager_handle()
{
    const size_t size = sizeof(struct logger_udp_network_data);
    struct logger_udp_network_data* handle = malloc(size);
    memset(handle, 0, size);
    handle->sock = -1;
    return handle;
}


/**
 * @brief Manages UDP connection to the server
 * 
 * @param nm logger_udp_network_data struct which contains necessary data for a UDP connection
 * @return void
 **/
bool init_udp_network_manager(struct logger_udp_network_data* nm, const char* host, int port)
{
    // use printf() for local logging to avoid anything weird with feedback loops, since we're hooked into ESP_LOG()

    assert(nm);
    if (!nm)
        return false;

    memset(nm, 0, sizeof(struct logger_udp_network_data));
    nm->sock = -1;

    if (!host || strlen(host) <= 0 || port <= 0) {
        return false;
    }

    struct hostent *server;
    server = gethostbyname(host);
    if (server == NULL) {
        printf("%s: No such host known", TAG);
        return false;
    }

	memcpy((void*)&nm->dest_addr.sin_addr, server->h_addr_list[0], server->h_length);
	nm->dest_addr.sin_family = AF_INET;
	nm->dest_addr.sin_port = htons(port);
	nm->addr_family = AF_INET;
	nm->ip_protocol = IPPROTO_IP;
	inet_ntoa_r(nm->dest_addr.sin_addr, nm->addr_str, sizeof(nm->addr_str) - 1);

	nm->sock = socket(nm->addr_family, SOCK_DGRAM, nm->ip_protocol);
	if (nm->sock < 0)
	{
		printf("%s: Unable to create socket: errno %d", TAG, errno);
        return false;
	}

    printf("%s: Socket created, connected to %s:%d", TAG, host, port);
    return true;
}

bool is_logging_udp_connected(struct logger_udp_network_data* nm) {
    assert(nm);
    return nm && nm->sock > 0;
}

/**
 * @brief Sends data to the server through a UDP socket
 * 
 * @param nm A pointer to logger_udp_network_data struct
 * @param payload char array which contains data to be sent
 * @return int - returns -1 if sending failed, number of bytes sent if successfully sent the data
 **/
int send_udp_data(struct logger_udp_network_data* nm, char* payload)
{
	int err = sendto(nm->sock, payload, strlen(payload), 0, (struct sockaddr *)&(nm->dest_addr), sizeof(nm->dest_addr));
	if (err < 0)
	{
        if (errno != 118) { // 118 = no network is available. we'll silently ignore it.
            printf("%s: Error occurred during sending: errno %d", TAG, errno);
        }
	}
	else
	{
        // printf("%s: Log msg sent via UDP", TAG); // very spammy
	}

	return err;
}

/**
 * @brief Receives data from UDP server
 * 
 * @param nm logger_udp_network_data struct which contains connection info
 * @return char array which contains data received
 **/
char* receive_udp_data(struct logger_udp_network_data* nm)
{
	struct sockaddr_in source_addr;
	socklen_t socklen = sizeof(source_addr);
	int len = recvfrom(nm->sock, nm->rx_buffer, sizeof(nm->rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

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
 * @param nm logger_udp_network_data struct which contains connection info
 * @return void
 **/
void close_udp_network_manager(struct logger_udp_network_data* nm)
{
    assert(nm);
    if (!nm) {
        return;
    }

	ESP_LOGI(TAG, "%s", "Shutting down socket");
	shutdown(nm->sock, 0);
	close(nm->sock);
	free(nm);
}
