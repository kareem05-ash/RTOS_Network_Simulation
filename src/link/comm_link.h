#ifndef COMM_LINK_H
#define COMM_LINK_H

#include "FreeRTOS.h"
#include "task.h"
#include "types.h"

// Forward path task — Node1 -> Node2
void vCommLinkForwardTask(void *pvParameters);

// ACK return path task — Node2 -> Node1
void vCommLinkACKTask(void *pvParameters);

#endif