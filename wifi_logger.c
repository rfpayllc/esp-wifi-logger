#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "utils.h"

// if true, local console spews a lot of debug output
#define DEBUG_VERBOSE_LOCAL_LOGGING 0

#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_UDP==1
#include "udp_handler.h"
#endif
#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_TCP==1
#include "tcp_handler.h"
#endif
#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_WEBSOCKET==1
#include "websocket_handler.h"
#endif

#include "wifi_logger.h"

static const char* TAG = "wifi_logger";

static bool s_wifi_logging_sending_enabled = true;
void udp_logging_set_sending_enabled(bool sending_enabled)
{
    // this call is safe to call before init, but enabling has no effect unless we previously started up logging.
    // it just enables/disabling network logging AFTER we already set everything else up.
    s_wifi_logging_sending_enabled = sending_enabled;
    ESP_LOGW(TAG, "udp logging state now set to: %d", s_wifi_logging_sending_enabled);
}


/**
 * @brief Initialises message queue
 * 
 * @return esp_err_t ESP_OK - if queue init sucessfully, ESP_FAIL - if queue init failed
 **/
static QueueHandle_t wifi_logger_queue;
esp_err_t init_queue(void)
{
	wifi_logger_queue = xQueueCreate(CONFIG_LOGGING_SERVER_MESSAGE_QUEUE_SIZE, sizeof(char*));

	if (wifi_logger_queue == NULL)
	{
		ESP_LOGE(TAG, "%s", "Queue creation failed");
		return ESP_FAIL;
	}
	else
	{
		ESP_LOGI(TAG, "%s", "Queue created");
		return ESP_OK;
	}
}

/**
 * @brief Sends log message to message queue
 * 
 * @param log_message log message to be sent to the queue
 * @return esp_err_t ESP_OK - if queue init sucessfully, ESP_FAIL - if queue init failed
 **/
esp_err_t send_to_queue(char* log_message)
{
    // use printf() for local logging (since ESP_LOGxxx may create a weird feedback loop since we potentially have it hooked)

    if (wifi_logger_queue == NULL) {
        printf("wifi logger: enqueue: queue not created / configured incorrectly. please fix.");
        return ESP_FAIL;
    }

	BaseType_t qerror = xQueueSendToBack(wifi_logger_queue, (void*)&log_message, (TickType_t) 0/portTICK_PERIOD_MS);

	if(qerror == pdPASS)
	{
        #if DEBUG_VERBOSE_LOCAL_LOGGING==1
		printf("log msg sent to Queue"); // spammy.
        #endif
		return ESP_OK;
	}
	else if(qerror == errQUEUE_FULL)
	{
		printf("wifi_logger: Data not sent to Queue, Queue full");
		return ESP_FAIL;
	}
	else
	{
        printf("wifi_logger: Unknown error");
		return ESP_FAIL;
	}
}

/**
 * @brief Receive data from queue. Timeout is set to portMAX_DELAY, which is around 50 days (confirm from esp32 specs)
 * 
 * @return char* - returns log message received from the queue, returns NULL if error
 **/
char* receive_from_queue(void)
{
    // use printf() for local logging (since ESP_LOGxxx may create a weird feedback loop since we potentially have it hooked)

	char* data;
	// ************************* IMPORTANT *******************************************************************
	// Timeout period is set to portMAX_DELAY, so if it doesnot receive a log message for ~50 days, config assert will fail and program will crash
	//
	BaseType_t qerror = xQueueReceive(wifi_logger_queue, &data, (TickType_t) portMAX_DELAY);
	configASSERT(qerror);
	//
	// *******************************************************************************************************

	if(qerror == pdPASS)
	{
        #if DEBUG_VERBOSE_LOCAL_LOGGING==1
		printf("Data received from Queue"); // spammy
        #endif
	}
	else if(qerror == pdFALSE)
	{
        printf("wifi_logger: dequeue: Data not received from Queue");
		data = NULL;
	}
	else
	{
		free((void*)data);
        printf("wifi_logger: dequeue: Unknown error");
		data = NULL;
	}

	return data;
}

/**
 * @brief generates log message, of the format generated by ESP_LOG function
 * 
 * @param level set ESP LOG level {E, W, I, D, V}
 * @param log_tag Tag for the log message
 * @param line line
 * @param func func
 * @param fmt fmt
 */
void generate_log_message(esp_log_level_t level, const char *log_tag, int line, const char *func, const char *fmt, ...)
{
    // this function is NOT used during hooking of ESP_LOGxx() functions.
    // it is only used as a manual call for sending msgs that should ONLY go through network logging.

    if (!s_wifi_logging_sending_enabled)
        return;

    const uint32_t buffer_size = CONFIG_LOGGING_SERVER_BUFFER_SIZE;
    char log_print_buffer[buffer_size];

	memset(log_print_buffer, '\0', buffer_size);
	snprintf(log_print_buffer, buffer_size, "%s (%s:%d) ", log_tag, func, line);
	va_list args;
	va_start(args, fmt);

	int len = strlen(log_print_buffer);

	if (buffer_size - len > 1)
	{
		vsnprintf(&log_print_buffer[len], (buffer_size - len), fmt, args);
	}
	else
	{
		memset(log_print_buffer, '\0', buffer_size);
		sprintf(log_print_buffer, "%s", "Buffer overflowed, increase buffer size");
	}
	va_end(args);

	uint8_t log_level_opt = 2;

	switch (level)
	{
	case ESP_LOG_ERROR:
		log_level_opt = 0;
		break;
	case ESP_LOG_WARN:
		log_level_opt = 1;
		break;
	case ESP_LOG_INFO:
		log_level_opt = 2;
		break;
	case ESP_LOG_DEBUG:
		log_level_opt = 3;
		break;
	case ESP_LOG_VERBOSE:
		log_level_opt = 4;
		break;
	default:
		log_level_opt = 2;
		break;
	}

	// ************************* IMPORTANT *******************************************************************
	// I am mallocing a char* inside generate_log_timestamp() function situated inside util.cpp, log_print_buffer is not being pushed to queue
	// The function returns the malloc'd char* and is passed to the queue
	//
	send_to_queue(generate_log_message_timestamp_and_device_id(true, log_level_opt, esp_log_timestamp(), log_print_buffer));
	//
	//********************************************************************************************************
}

/**
 * @brief route log messages generated by ESP_LOGX to the wifi logger
 * 
 * @param fmt logger string format
 * @param tag arguments
 * @return int return value of vprintf
 */
int system_log_message_route(const char* fmt, va_list tag)
{
    // TODO: optimize allocations, this is casually using/freeing a lot of memory.

    // ESP_LOGx() statements call this.
    // we perform 2 decisions:
    // 1. do we want to send this message out over the network?
    // 2. do we want to echo this locally

    // ---------------------------
    // STEP 1 - NETWORK SENDING
    // ---------------------------

    // if we're in an interrupt, just... dont even try, forget it.
    // not sure we should even try this though.
    bool skip_network_logging = false;
    skip_network_logging |= xPortInIsrContext();

    // optional...ish? skip some tasks that might cause re-entrat issues if they call for logs while we're logging (like LWIP etc)
    // for instance: "tiT" is LWIP stack. we don't want logging stuff from the TCP/IP stack caused by message from inside our sendto()
    char *cur_task = pcTaskGetName(xTaskGetCurrentTaskHandle());
    skip_network_logging |= strcmp(cur_task, "tiT") == 0;

    // finally, we'll skip if network sending is explicitly disabled
    skip_network_logging |= !s_wifi_logging_sending_enabled;

    // note: even if we're skipping sending to UDP, we need to call vprintf() below to display the log msg locally.
    if (!skip_network_logging)
    {
        // we do want to send to UDP! let's prep.

        char *log_print_buffer = (char *) malloc(sizeof(char) * CONFIG_LOGGING_SERVER_BUFFER_SIZE);
        vsprintf(log_print_buffer, fmt, tag);

        // send_to_queue(log_print_buffer); // original. (MUST FREE log_print_buffer here)

        // or... this version prepends the mac address.
        // note that this does an additional malloc that the queue consumer must free.
        // note that log_print_buffer is copied into this new malloc()'d data, so, WE need to free it right after.
        send_to_queue(generate_log_message_timestamp_and_device_id(false, 0, 0, log_print_buffer));

        // generate_log_message_timestamp_and_device_id() does an additional copy of this data, so, we should free this ourselves.
        free(log_print_buffer);
    }

    // ---------------------------
    // STEP 2 - LOCAL ECHO
    // ---------------------------

    // pass along to original normal logging system now. (this actually just prints it to the console, normally)
	return vprintf(fmt, tag);
}

/*
 * @brief A common wrapper function to check connection status for all interfaces
 *
 * @param handle_t Interface handle
 * @return bool True if connected
 */
bool is_connected(void* handle_t)
{
	bool ret = false;

#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_WEBSOCKET==1
    ret |= is_websocket_connected(handle_t);
#endif
#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_TCP==1
    ret |= is_tcp_connected(handle_t);
#endif
#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_UDP==1
	ret |= is_logging_udp_connected(handle_t);
#endif

	return ret;
}

/**
 * @brief function which handles sending of log messages to server by UDP
 * 
 */
#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_UDP==1
bool update_udp_logging(struct logger_udp_network_data *handle, const char *host, int port)
{
    // use printf() for local logging to avoid anything weird with feedback loops, since we're hooked into ESP_LOG()

    if (!is_logging_udp_connected(handle) && !init_udp_network_manager(handle, host, port)) {
        return false;
    }

    char *log_message = receive_from_queue();
    if (log_message == NULL) {
        printf("%s: receive_from_queue() got NULL, can't send anything. ignoring", TAG);
        return false;
    }

    int len_sent;
    send_udp_data(handle, log_message, &len_sent);
    #if DEBUG_VERBOSE_LOCAL_LOGGING==1
    printf("%s: %d %s", TAG, len_sent, "bytes of data sent"); // spammy
    #endif

    free((void *) log_message);

    return true;
}

_Noreturn void wifi_logger_task(void* param)
{
    assert(param);
    struct wifi_logger_config* config = (struct wifi_logger_config*)param;
    assert(config->host);

    struct logger_udp_network_data* handle = create_udp_network_manager_handle();

	while (true)
	{
        bool ok = update_udp_logging(handle, config->host, config->port);

        //Checkout following link to understand why we need this delay if want watchdog running.
        //https://github.com/espressif/esp-idf/issues/1646#issuecomment-367507724
        // 10 = shortest possible delay
        vTaskDelay((ok ? 10 : 2000) / portTICK_PERIOD_MS);
    }

    close_udp_network_manager(handle);
    handle = NULL;
}
#endif

// TODO: tcp and websockets need updating for new API/structs. see UDP implementation for reference. complete this
#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_TCP==1
bool send_next_tcp_queue_msg(struct logger_tcp_network_data *handle)
{
    if (!is_connected(handle))
        return false;

    char* log_message = receive_from_queue(); // wait forever for a msg to come in

    // is this a busted log msg?
    if (log_message == NULL) {
        int len = tcp_send_data(handle, "Unknown error - receiving log message");
        ESP_LOGE(TAG, "%d %s", len, "Unknown error");
        return false;
    }

    int len = tcp_send_data(handle, log_message);
    if (len < 0) {
        /* Trying to push it back to queue if sending fails, but might lose some logs if frequency is high
        * Might see garbage at first when reconnected.
        */
        xQueueSendToFront(wifi_logger_queue, (void *) log_message, (TickType_t) portMAX_DELAY);
        return false;
    }

    ESP_LOGD(TAG, "%d %s", len, "bytes of data sent");
    free(log_message);
    return true;
}

bool update_tcp_logging(struct logger_tcp_network_data *handle, const char *host, int port)
{
    // ensure we're connected
    if (!is_tcp_connected(handle) && !connect_tcp_network_manager(handle, host, port))
        return false;

    // after connecting, send the next log msg from the queue if available (warning: blocks)
    if (!send_next_tcp_queue_msg(handle)) {
        tcp_close_network_manager(handle);
        return false;
    }

    return true;
}


/**
 * @brief function which handles sending of log messages to server by TCP
 * 
 */
_Noreturn void wifi_logger_task()
{
    struct logger_tcp_network_data* handle = create_tcp_network_manager_handle();
    const char* host = "TODO";      // TODO: get from somewhere
    int port = 9999;                // TODO: get from somewhere

	while(1)
	{
        //Checkout following link to understand why we need this delay if want watchdog running.
        //https://github.com/espressif/esp-idf/issues/1646#issuecomment-367507724
        int delay_tick_ms = 10; // shortest possible delay
        if (!update_tcp_logging(handle, host, port))
        {
            delay_tick_ms = 2000; // wait a lot longer
        }

        vTaskDelay(delay_tick_ms / portTICK_PERIOD_MS);
    }

    free(handle);
}
#endif

// TODO: tcp and websockets need updating for new API/structs. see UDP implementation for reference. complete this
#if CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_WEBSOCKET==1
_Noreturn void wifi_logger_task()
{
    // TODO: update for refactored API, right now doesn't work.
	esp_websocket_client_handle_t handle = init_websocket_network_manager();

	while (true)
	{
		if(is_connected(handle))
		{
			char* log_message = receive_from_queue();

            if (log_message == NULL) {
                log_message = "Unknown error - log message corrupt";
                int len = websocket_send_data(handle, log_message);
                ESP_LOGE(TAG, "%d %s", len, "Unknown error");
            } else {
                int len = websocket_send_data(handle, log_message);

                if (len > 0) {
                    ESP_LOGD(TAG, "%d %s", len, "bytes of data sent");
                }

                free(log_message);
            }
        }
		else
		{
			//Checkout following link to understand why we need this delay if want watchdog running.
			//https://github.com/espressif/esp-idf/issues/1646#issuecomment-367507724
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	}

	websocket_close_network_manager(handle);
}
#endif

bool set_wifi_logger_config(struct wifi_logger_config* config, const char* host, int port, bool route_esp_idf_api_logs_to_wifi)
{
    assert(config);
    if (!config || !host || strlen(host) <= 0 || port <= 0) {
        ESP_LOGE(TAG, "config input params invalid, aborting.");
        return false;
    }

    if (strlen(host) >= sizeof(config->host)) {
        ESP_LOGE(TAG, "Host string is too long. Please provide a host string with less than 128 characters.\n");
        return false;
    }

    strcpy(config->host, host);
    config->port = port;
    config->route_esp_idf_api_logs_to_wifi = route_esp_idf_api_logs_to_wifi;

    return true;
}

/**
 * @brief wrapper function to start wifi logger
 * 
 */
bool start_wifi_logger(struct wifi_logger_config* config)
{
    if (init_queue() != ESP_OK)
        return false;

    // make a copy to pass in
    struct wifi_logger_config* config_copy = malloc(sizeof(struct wifi_logger_config));
    if(!config_copy) {
        ESP_LOGE(TAG, "out of memory copying config for init");
        return false;
    }
    memcpy(config_copy, config, sizeof(struct wifi_logger_config));

    if (config->route_esp_idf_api_logs_to_wifi) {
        esp_log_set_vprintf(system_log_message_route); // after queue init only. routes all ESP_LOGx() functions to our handler from now on.
    }

    xTaskCreatePinnedToCore(wifi_logger_task, "wifi_logger", 4096, config_copy, 2, NULL, 1);
    ESP_LOGI(TAG, "****** ============ !! UDP LOGGING HAS STARTED !! ============ ******");
    return true;
}