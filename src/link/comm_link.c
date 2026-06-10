#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "comm_link.h"
#include "types.h"
#include "utils.h"
#include "statistics.h"

void vCommLinkForwardTask(void *pvParameters)
{
    (void) pvParameters;
    printf("[LINK_FWD] Task started\n");

    Packet_t *pkt = NULL;

    for(;;)
    {
        if(xQueueReceive(tx_link_queue, &pkt, portMAX_DELAY) != pdPASS)
            continue;

        printf("[LINK_FWD] Received packet seq=%lu | len=%u bytes\n",
               (unsigned long) pkt->seq_num, pkt->length);

        // Compute delay
        uint32_t tx_delay    = calc_tx_delay(pkt->length);
        uint32_t total_delay = tx_delay + D_MS;

        printf("[LINK_FWD] Delay = %lu ms (tx=%lu + prop=%d)\n",
               (unsigned long) total_delay,
               (unsigned long) tx_delay, D_MS);

        vTaskDelay(pdMS_TO_TICKS(total_delay));

        // Drop?
        if(should_drop(ACTIVE_P_DROP))
        {
            printf("[LINK_FWD] Packet seq=%lu DROPPED ❌\n",
                   (unsigned long) pkt->seq_num);
            stats_packet_dropped_link();
            free(pkt);   // link owns the copy — free it on drop ✅
            pkt = NULL;
            continue;
        }

        // Forward copy to receiver
        printf("[LINK_FWD] Packet seq=%lu forwarded ✅\n",
               (unsigned long) pkt->seq_num);

        if(xQueueSend(rx_queue, &pkt, portMAX_DELAY) != pdPASS)
        {
            printf("[LINK_FWD] ERROR: rx_queue full\n");
            free(pkt);   // can't forward — free it ✅
            pkt = NULL;
        }
    }
}

void vCommLinkACKTask(void *pvParameters)
{
    (void) pvParameters;
    printf("[LINK_ACK] Task started\n");

    ACK_t *ack = NULL;

    for(;;)
    {
        if(xQueueReceive(ack_tx_queue, &ack, portMAX_DELAY) != pdPASS)
            continue;

        printf("[LINK_ACK] Received ACK for seq=%lu\n",
               (unsigned long) ack->ack_seq_num);

        uint32_t tx_delay    = calc_tx_delay(K_BYTES);
        uint32_t total_delay = tx_delay + D_MS;

        printf("[LINK_ACK] Delay = %lu ms\n", (unsigned long) total_delay);

        vTaskDelay(pdMS_TO_TICKS(total_delay));

        if(should_drop(P_ACK))
        {
            printf("[LINK_ACK] ACK seq=%lu DROPPED ❌\n",
                   (unsigned long) ack->ack_seq_num);
            free(ack);   // link frees dropped ACK ✅
            ack = NULL;
            continue;
        }

        printf("[LINK_ACK] ACK seq=%lu forwarded ✅\n",
               (unsigned long) ack->ack_seq_num);

        if(xQueueSend(ack_rx_queue, &ack, portMAX_DELAY) != pdPASS)
        {
            printf("[LINK_ACK] ERROR: ack_rx_queue full\n");
            free(ack);
            ack = NULL;
        }
    }
}