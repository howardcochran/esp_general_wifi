#ifndef _GENERAL_WIFI_H_
#define _GENERAL_WIFI_H_

void wifi_init_sta(const char *ssid, const char *password);
void start_ota_server();
void init_udp_log(const char *dest_addr, int port);

#endif
