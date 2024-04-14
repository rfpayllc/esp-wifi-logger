set(srcs "wifi_logger.c" "utils.cpp")

set(priv_requires "")

if(CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_TCP)
    list(APPEND srcs "tcp_handler.c")
elseif(CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_UDP)
    list(APPEND srcs "udp_handler.c")
elseif(CONFIG_LOGGING_SERVER_TRANSPORT_PROTOCOL_WEBSOCKET)
    list(APPEND srcs "websocket_handler.c")
    list(APPEND priv_requires "esp_websocket_client")
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "${priv_requires}")