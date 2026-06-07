#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "src/common/types.h"
#include "src/utils/utils.h"
#include "src/generator/Packet_generator.h"

//---------initalize all Queues to NULL -------
QueueHandle_t packet_queue      = NULL;
QueueHandle_t tx_link_queue     = NULL;
QueueHandle_t rx_queue          = NULL;
QueueHandle_t ack_tx_queue      = NULL;
QueueHandle_t ack_rx_queue      = NULL;

int main(void){

    printf("=== RTOS Network Simulation ===\n");
    printf("Active Config: P_drop=%.2f | Tout=%d ms\n",
       ACTIVE_P_DROP, ACTIVE_TOUT_MS);

// -------- create all Queues --------------------------
packet_queue    = xQueueCreate(PACKET_QUEUE_SIZE, sizeof(Packet_t *));
tx_link_queue   = xQueueCreate(LINK_QUEUE_SIZE,   sizeof(Packet_t *));
rx_queue        = xQueueCreate(LINK_QUEUE_SIZE,   sizeof(Packet_t *));
ack_tx_queue    = xQueueCreate(ACK_QUEUE_SIZE,    sizeof(ACK_t *));
ack_rx_queue    = xQueueCreate(ACK_QUEUE_SIZE,    sizeof(ACK_t *));

// -------- verify all Queues created successfully -----
configASSERT(packet_queue   !=NULL);
configASSERT(tx_link_queue  !=NULL);
configASSERT(rx_queue       !=NULL);
configASSERT(ack_tx_queue   !=NULL);
configASSERT(ack_rx_queue   !=NULL);

printf("All queues created OK\n");
printf("sizeof(Packet_t header) = %lu bytes\n", sizeof(Packet_t));
printf("sizeof(ACK_t)           = %lu bytes\n", sizeof(ACK_t));

//------- Spawn Packet Generator Task--------------------
    xTaskCreate(
        vPacketGeneratorTask,   /* function        */
        "Generator",            /* name for debug  */
        1024,                   /* stack size      */
        NULL,                   /* parameters      */
        PRIORITY_GENERATOR,     /* priority        */
        NULL                    /* handle          */
    );

    printf("Tasks spawned — starting scheduler...\n");

vTaskStartScheduler();
        return 0;
}