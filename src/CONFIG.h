#pragma once

#include <sdkconfig.h>

#ifndef CONFIG_MAIN_WIFI_DEFAULT_SSID
// #define CONFIG_MAIN_WIFI_DEFAULT_SSID "BMZimages"
#define CONFIG_MAIN_WIFI_DEFAULT_SSID "eFisheryFS"
#endif /*CONFIG_MAIN_WIFI_DEFAULT_SSID*/

#ifndef CONFIG_MAIN_WIFI_DEFAULT_PASS
// #define CONFIG_MAIN_WIFI_DEFAULT_PASS "bennamazarina"
#define CONFIG_MAIN_WIFI_DEFAULT_PASS "123123123"
#endif /*CONFIG_MAIN_WIFI_DEFAULT_PASS*/

#ifndef CONFIG_MAIN_SERVER
#define CONFIG_MAIN_SERVER "angkasatimelapse.com"
// #define CONFIG_MAIN_SERVER                              "test.mosquitto.org"
#endif /*CONFIG_MAIN_SERVER*/

// #ifndef MSG_BUFFER_SIZE
// #define MSG_BUFFER_SIZE	(50)
// #endif