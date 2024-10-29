#pragma once

#include <sdkconfig.h>

#ifndef CONFIG_MQTT_HANDLER_LOG_TAG
#define CONFIG_MQTT_HANDLER_LOG_TAG                 "MQTTHANDLER"
#endif /* CONFIG_MQTT_HANDLER_LOG_TAG */
#define MQTTHANDLERTAG CONFIG_MQTT_HANDLER_LOG_TAG

#ifndef CONFIG_MQTT_HANDLER_TASK_PRIO
#define CONFIG_MQTT_HANDLER_TASK_PRIO               1                       // default task Priority 1
#endif /* CONFIG_MQTT_HANDLER_TASK_PRIO */

#ifndef CONFIG_MQTT_HANDLER_TASK_NAME
#define CONFIG_MQTT_HANDLER_TASK_NAME               "MQTTHANDLERTASK"       // default task tag name
#endif /* CONFIG_MQTT_HANDLER_TASK_NAME */

#ifndef CONFIG_MQTT_HANDLER_TASK_STACK
#define CONFIG_MQTT_HANDLER_TASK_STACK               (4096)                 // 4KB task memory alloc
#endif /* CONFIG_MQTT_HANDLER_TASK_STACK */