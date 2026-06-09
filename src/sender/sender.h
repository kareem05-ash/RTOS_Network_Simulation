#ifndef SENDER_H
#define SENDER_H

#include "FreeRTOS.h"
#include "task.h"
#include "types.h"

// Task function
void vSenderTask(void *pvParameters);

#endif // SENDER_H