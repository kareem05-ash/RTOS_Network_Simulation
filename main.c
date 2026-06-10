#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "types.h"
#include "utils.h"
#include "src/stats/statistics.h"
#include "packet_generator.h"
#include "sender.h"
#include "comm_link.h"
#include "receiver.h"

// ── Global Queue Handles ──
QueueHandle_t packet_queue   = NULL;
QueueHandle_t tx_link_queue  = NULL;
QueueHandle_t rx_queue       = NULL;
QueueHandle_t ack_tx_queue   = NULL;
QueueHandle_t ack_rx_queue   = NULL;

int main(void)
{
    printf("=== RTOS Network Simulation ===\n");
    printf("Active Config: P_drop=%.2f | Tout=%d ms\n",
           ACTIVE_P_DROP, ACTIVE_TOUT_MS);

    // ── Initialize statistics ──
    stats_init();

    // ── Create all queues ──
    packet_queue   = xQueueCreate(PACKET_QUEUE_SIZE, sizeof(Packet_t *));
    tx_link_queue  = xQueueCreate(LINK_QUEUE_SIZE,   sizeof(Packet_t *));
    rx_queue       = xQueueCreate(LINK_QUEUE_SIZE,   sizeof(Packet_t *));
    ack_tx_queue   = xQueueCreate(ACK_QUEUE_SIZE,    sizeof(ACK_t *));
    ack_rx_queue   = xQueueCreate(ACK_QUEUE_SIZE,    sizeof(ACK_t *));

    configASSERT(packet_queue  != NULL);
    configASSERT(tx_link_queue != NULL);
    configASSERT(rx_queue      != NULL);
    configASSERT(ack_tx_queue  != NULL);
    configASSERT(ack_rx_queue  != NULL);

    printf("All queues created OK\n");

    // ── Spawn all tasks ──
    xTaskCreate(vPacketGeneratorTask, "Generator",   1024,
                NULL, PRIORITY_GENERATOR,  NULL);

    xTaskCreate(vSenderTask,          "Sender",      2048,
                NULL, PRIORITY_SENDER,     NULL);

    xTaskCreate(vCommLinkForwardTask, "LinkFwd",     2048,
                NULL, PRIORITY_LINK,       NULL);

    xTaskCreate(vCommLinkACKTask,     "LinkACK",     2048,
                NULL, PRIORITY_LINK,       NULL);

    xTaskCreate(vReceiverTask,        "Receiver",    2048,
                NULL, PRIORITY_RECEIVER,   NULL);

    printf("All tasks spawned — starting scheduler...\n");

    // ── Start FreeRTOS Scheduler ──
    vTaskStartScheduler();

    return 0;
}