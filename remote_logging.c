#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "lwip/sockets.h"

static const char *TAG = "UDP_LOG";

static int udp_socket = -1;
static struct sockaddr_in dest_sock_addr;

static vprintf_like_t orig_log_handler;

int udp_log_handler(const char *format, va_list args) {
    if (orig_log_handler != NULL) {
        orig_log_handler(format, args);
    }
    if (udp_socket < 0) {
        return -1;
    }

    char log_buffer[256];
    log_buffer[0] = '\0';
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    int len = strlen(log_buffer);
    sendto(udp_socket, log_buffer, len, 0, (struct sockaddr *)&dest_sock_addr, sizeof(dest_sock_addr));
    return len;
}

void init_udp_log(const char* dest_addr, int port) {
    if (dest_addr == NULL || strlen(dest_addr) < 1) {
        ESP_LOGE(TAG, "Missing parameter: dest_addr");
        return;
    }
    if (port < 1) {
        port = 12345;
        ESP_LOGI(TAG, "Using default port: %d", port);
    }

    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_socket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    memset(&dest_sock_addr, 0, sizeof(dest_sock_addr));
    dest_sock_addr.sin_family = AF_INET;
    dest_sock_addr.sin_port = htons(port);
    inet_pton(AF_INET, dest_addr, &dest_sock_addr.sin_addr.s_addr);

    // Set custom log handler
    orig_log_handler = esp_log_set_vprintf(udp_log_handler);

    ESP_LOGI(TAG, "UDP log initialized to %s:%d", dest_addr, port);
}
