#ifndef WIFI_LOGGER_H
#define WIFI_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif


struct wifi_logger_config {
    char host[128];
    int port;
    bool route_esp_idf_api_logs_to_wifi;
};

#define wifi_log_e(TAG, fmt, ...) generate_log_message(ESP_LOG_ERROR, TAG, __LINE__, __func__, fmt, __VA_ARGS__)
#define wifi_log_w(TAG, fmt, ...) generate_log_message(ESP_LOG_WARN, TAG, __LINE__, __func__, fmt, __VA_ARGS__)
#define wifi_log_i(TAG, fmt, ...) generate_log_message(ESP_LOG_INFO, TAG, __LINE__, __func__, fmt, __VA_ARGS__)
#define wifi_log_d(TAG, fmt, ...) generate_log_message(ESP_LOG_DEBUG, TAG, __LINE__, __func__, fmt, __VA_ARGS__)
#define wifi_log_v(TAG, fmt, ...) generate_log_message(ESP_LOG_VERBOSE, TAG, __LINE__, __func__, fmt, __VA_ARGS__)

// if using websockets, port is ignored and your host line should be a URI like: "ws://192.168.0.1:1234"
bool set_wifi_logger_config(struct wifi_logger_config* config, const char* host, int port, bool route_esp_idf_api_logs_to_wifi);
bool start_wifi_logger(const struct wifi_logger_config* config);

// after starting everything else up, you can use this to toggle whether logs are being sent out or not.
void udp_logging_set_sending_enabled(bool sending_enabled);

void generate_log_message(esp_log_level_t level, const char *TAG, int line, const char *func, const char *fmt, ...);
bool is_connected(void* handle_t); // TODO: fix definition

#ifdef __cplusplus
}
#endif

#endif // WIFI_LOGGER_H

