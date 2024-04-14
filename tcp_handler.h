#ifndef TCP_HANDLER_H
#define TCP_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

struct logger_tcp_network_data;

struct logger_tcp_network_data* create_tcp_network_manager_handle();
bool connect_tcp_network_manager(struct logger_tcp_network_data* nm, const char* host, int port);
int tcp_send_data(struct logger_tcp_network_data* nm, char* payload);
char* tcp_receive_data(struct logger_tcp_network_data* nm);
void tcp_close_network_manager(struct logger_tcp_network_data* nm);
bool is_tcp_connected(struct logger_tcp_network_data* nm);

#ifdef __cplusplus
}
#endif

#endif
