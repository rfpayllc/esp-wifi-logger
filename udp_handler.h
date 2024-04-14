#ifndef LOGGING_UDP_HANDLER_H
#define LOGGING_UDP_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

struct logger_udp_network_data;

struct logger_udp_network_data* create_udp_network_manager_handle();
bool is_logging_udp_connected(struct logger_udp_network_data* nm);
bool init_udp_network_manager(struct logger_udp_network_data* nm, const char* host, int port);
int send_udp_data(struct logger_udp_network_data* nm, char* payload);
char* receive_udp_data(struct logger_udp_network_data* nm);
void close_udp_network_manager(struct logger_udp_network_data* nm);

#ifdef __cplusplus
}
#endif

#endif // LOGGING_UDP_HANDLER_H