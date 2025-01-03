idf_component_register(
    SRCS
        "ota.cpp"
        "remote_logging.cpp"
        "wifi.cpp"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        app_update
        esp_http_server
        esp_timer
        esp_wifi
        nvs_flash
)

set(CMAKE_CXX_STANDARD 20)
