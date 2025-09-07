#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
#include <string>
#include <cstring>
extern "C" {
#endif

char* generate_log_message_timestamp_and_device_id(bool print_device_id, bool print_timestamp, uint8_t log_level, uint32_t timestamp, const char* log_message);

const char* udp_logging_get_device_id();

#ifdef __cplusplus
}
#endif

#endif