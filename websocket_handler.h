#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

struct websocket_network_manager;

struct websocket_network_manager* init_websocket_network_manager();
int websocket_send_data(struct websocket_network_manager* nm, char* payload);
void websocket_close_network_manager(struct websocket_network_manager* nm);
bool is_websocket_connected(struct websocket_network_manager* nm);

#ifdef __cplusplus
}
#endif

#endif // WEBSOCKET_HANDLER_H
