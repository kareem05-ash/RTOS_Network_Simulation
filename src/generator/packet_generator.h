#ifndef PACKET_GENERATOR_H
#define PACKET_GENERATOR_H

#include "FreeRTOS.h"
#include "task.h"
#include "types.h"

/* Task function — pass to xTaskCreate */
void vPacketGeneratorTask(void *pvParameters);

#endif 