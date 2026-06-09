#ifndef RECEIVER_H
#define RECEIVER_H

#include "FreeRTOS.h"
#include "task.h"
#include "types.h"

// Task function — pass to xTaskCreate
void vReceiverTask(void *pvParameters);

#endif 