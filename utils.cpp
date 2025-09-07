#include <esp_mac.h>
#include "utils.h"

static constexpr char log_level_char[5] = { 'E', 'W', 'I', 'D', 'V'};
static constexpr char log_level_color[5][7] = {"\e[31m", "\e[33m", "\e[32m", "\e[39m", "\e[39m"};

/**
 * @brief adds timestamp to the log message
 * 
 * @param print_device_id if true, prepend the device's ID to the msg (mac address)
 * @param print_timestamp if true, add log_level and timestamp to the msg. (logs received via ESP_LOGxx() already have this baked in, so pass in false here)
 * @param log_level (only used if print_timestamp=true) log level of the log message
 * @param timestamp (only used if print_timestamp=true)timestamp provided by ESP in milliseconds
 * @param log_message log message to be sent through wifi
 * @return char* final log message with timestamp (caller is responsible for [eventually] freeing this)
 */
char* generate_log_message_timestamp_and_device_id(
    const bool print_device_id,
    const bool print_timestamp,
    const uint8_t log_level,
    const uint32_t timestamp,
    const char* log_message)
{
    // WARNING: this does a lot of memory allocation in the C++ string.
    // this could be better replaced with a simpler C version that uses buffer pools/etc if needed.

    const uint8_t final_log_level = log_level%5;

    // outputs text like: "12:34:56:78:9A| I (15517) your_tag: log line text goes here"
    std::string log_string;

    // ReSharper disable once CppDFAConstantConditions
    if (print_device_id)
    {
        // mac address
        const char* device_id = udp_logging_get_device_id();
        if (device_id && strlen(device_id) > 0)
        {
            log_string += std::string(device_id) +std::string("| ");
        }
    }

    if (print_timestamp) {
        log_string +=
                std::string(log_level_color[final_log_level]) + log_level_char[final_log_level] +
                std::string(" (") + std::to_string(timestamp) + std::string(") ");
    }

    log_string += std::string(log_message);

    if (print_timestamp) {
        log_string += std::string("\e[39m") + std::string("\n");
    }

    char *c_log_string = (char*) malloc(sizeof(char)*(log_string.size()+1));
    if (!c_log_string)
        return nullptr;

    memcpy(c_log_string, log_string.c_str(), log_string.size()+1);
    return c_log_string;
}