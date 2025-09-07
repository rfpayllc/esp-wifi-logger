#include <esp_mac.h>
#include "utils.h"

static const char log_level_char[5] = { 'E', 'W', 'I', 'D', 'V'};
static const char log_level_color[5][7] = {"\e[31m", "\e[33m", "\e[32m", "\e[39m", "\e[39m"};

static bool s_print_device_id = true;
static char* s_device_id = nullptr;

void utils_get_mac_address(char *formatted_mac_address)  // provide at least 18 byte buffer (17 chars + null) i.e. "12:45:78:90:23:56"
{
    uint8_t mac_address[6];
    esp_efuse_mac_get_default(mac_address);
    sprintf(formatted_mac_address, "%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
}

/**
 * @brief adds timestamp to the log message
 * 
 * @param log_level log level of the log message
 * @param timestamp timestamp provided by ESP in milliseconds
 * @param log_message log message to be sent through wifi
 * @return char* final log message with timestamp (caller is responsible for [eventually] freeing this)
 */
char* generate_log_message_timestamp_and_device_id(bool print_timestamp, uint8_t log_level, uint32_t timestamp, char* log_message)
{
    if (s_print_device_id && s_device_id == nullptr) {
        // one-time generate device_id via static var.
        // for now, use mac address. in future, can be made configurable.
        s_device_id = (char*)malloc(18); // TODO: free somewhere if we care
        assert(s_device_id);
        utils_get_mac_address(s_device_id);
    }

    log_level = log_level%5;

    // outputs text like: "12:34:56:78:9A| I (15517) your_tag: log line text goes here"
    std::string log_string;

    if (s_print_device_id && s_device_id)
        log_string += std::string(s_device_id) +std::string("| ");

    if (print_timestamp) {
        log_string +=
                std::string(log_level_color[log_level]) + log_level_char[log_level] +
                std::string(" (") + std::to_string(timestamp) + std::string(") ");
    }

    log_string += std::string(log_message);

    if (print_timestamp) {
        log_string += std::string("\e[39m") + std::string("\n");
    }

    char *c_log_string = (char*) malloc(sizeof(char)*(log_string.size()+1));
    memcpy(c_log_string, log_string.c_str(), log_string.size()+1);
    
    return c_log_string;
}