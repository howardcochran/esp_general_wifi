#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include "esp_log.h"
#include "esp_timer.h"

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a <= _b ? _a : _b; })


static const char *TAG = "OTA_Server";

esp_err_t ota_post_handler(httpd_req_t *req) {
    esp_ota_handle_t ota_handle;
    const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        ESP_LOGE(TAG, "No OTA partition found");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Beginning OTA update to partition %s at addr 0x%08lx. Size: %zu bytes.",
	     ota_partition->label, ota_partition->address, req->content_len);
    esp_err_t err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA Begin failed: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    char buffer[1024];
    int remaining = req->content_len;

    int64_t start_stamp = esp_timer_get_time();
    int64_t last_log_stamp = start_stamp;
    while (remaining > 0) {
        int received = httpd_req_recv(req, buffer, MIN(sizeof(buffer), remaining));
        if (received <= 0) {
	    ESP_LOGE(TAG, "OTA Receive Error.");
            esp_ota_end(ota_handle);
            return ESP_FAIL;
        }
        esp_ota_write(ota_handle, buffer, received);
        remaining -= received;
	int64_t cur_stamp = esp_timer_get_time();  // microsecond counter
	if (cur_stamp - last_log_stamp > 500 * 1000) {
            last_log_stamp = cur_stamp;
            ESP_LOGI(TAG, "Writing %zu%% (%zu / %zu)",
                     100 * (req->content_len - remaining) / req->content_len,
                     (req->content_len - remaining), req->content_len);
	}
    }
    ESP_LOGI(TAG, "Elapsed: %0.2fs", (float)(esp_timer_get_time() - start_stamp) / 1e6);

    esp_ota_end(ota_handle);
    esp_ota_set_boot_partition(ota_partition);
    ESP_LOGI(TAG, "OTA Update Complete. Rebooting...");
    esp_restart();

    return ESP_OK;
}

httpd_uri_t ota_uri = {
    .uri = "/ota",
    .method = HTTP_POST,
    .handler = ota_post_handler,
    .user_ctx = NULL
};

void start_ota_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Default 4096 is not enough for OTA and network logging
    config.stack_size = 8192;

    httpd_handle_t server;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &ota_uri);
        ESP_LOGI(TAG, "OTA Server started. Post firmware to /ota");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}
