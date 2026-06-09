#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "comm_link.h"
#include "types.h"
#include "utils.h"
#include "statistics.h"

// ─────────────────────────────────────────
// Forward Path Task
// Receives packets from sender (tx_link_queue)
// Applies delay + random drop
// Forwards surviving packets to receiver (rx_queue)
// ─────────────────────────────────────────
void vCommLinkForwardTask(void *pvParameters)
{
    (void) pvParameters;

    printf("[LINK_FWD] Task started\n");

    Packet_t *pkt = NULL;

    for(;;)
    {
        // Wait for a packet from the sender ──
        if(xQueueReceive(tx_link_queue, &pkt, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        printf("[LINK_FWD] Received packet seq=%lu | len=%u bytes\n",
               (unsigned long) pkt->seq_num, pkt->length);

        // Compute total delay ──
        // Transmission delay = (L * 8) / C  in ms
        // Propagation delay  = D ms (constant)
        uint32_t tx_delay   = calc_tx_delay(pkt->length);
        uint32_t total_delay = tx_delay + D_MS;

        printf("[LINK_FWD] Delay = %lu ms (tx=%lu + prop=%d)\n",
               (unsigned long) total_delay,
               (unsigned long) tx_delay,
               D_MS);

        //  Simulate the delay ──
        vTaskDelay(pdMS_TO_TICKS(total_delay));

        //  Randomly drop the packet ──
        if(should_drop(ACTIVE_P_DROP))
        {
            printf("[LINK_FWD] Packet seq=%lu DROPPED ❌ (P_drop=%.2f)\n",
                   (unsigned long) pkt->seq_num, ACTIVE_P_DROP);
            stats_packet_dropped_link();
            // Free memory — packet is gone
            free(pkt);
            pkt = NULL;
            continue;
        }

        // Packet survived — forward to receiver ──
        printf("[LINK_FWD] Packet seq=%lu forwarded to receiver ✅\n",
               (unsigned long) pkt->seq_num);

        if(xQueueSend(rx_queue, &pkt, portMAX_DELAY) != pdPASS)
        {
            printf("[LINK_FWD] ERROR: rx_queue full — dropping seq=%lu\n",
                   (unsigned long) pkt->seq_num);
            free(pkt);
            pkt = NULL;
        }
    }
}

// ─────────────────────────────────────────
// ACK Return Path Task
// Receives ACKs from receiver (ack_tx_queue)
// Applies delay + random drop (P_ack)
// Forwards surviving ACKs to sender (ack_rx_queue)
// ─────────────────────────────────────────
void vCommLinkACKTask(void *pvParameters)
{
    (void) pvParameters;

    printf("[LINK_ACK] Task started\n");

    ACK_t *ack = NULL;

    for(;;)
    {
        //  Wait for an ACK from the receiver ──
        if(xQueueReceive(ack_tx_queue, &ack, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        printf("[LINK_ACK] Received ACK for seq=%lu\n",
               (unsigned long) ack->ack_seq_num);

        // Compute ACK delay ──
        // ACK is fixed size K bytes
        uint32_t tx_delay    = calc_tx_delay(K_BYTES);
        uint32_t total_delay = tx_delay + D_MS;

        printf("[LINK_ACK] Delay = %lu ms\n", (unsigned long) total_delay);

        // Simulate the delay ──
        vTaskDelay(pdMS_TO_TICKS(total_delay));

        // Randomly drop the ACK ──
        if(should_drop(P_ACK))
        {
           printf("[LINK_FWD] Packet seq=%lu DROPPED ❌ (P_drop=%.2f)\n",
                (unsigned long) ack->ack_seq_num, ACTIVE_P_DROP);
            stats_packet_dropped_link();
            ack = NULL;    // just clear local pointer, don't free
            continue;
        }

        // ACK survived — forward to sender ──
        printf("[LINK_ACK] ACK seq=%lu forwarded to sender ✅\n",
               (unsigned long) ack->ack_seq_num);

        if(xQueueSend(ack_rx_queue, &ack, portMAX_DELAY) != pdPASS)
        {
            printf("[LINK_ACK] ERROR: ack_rx_queue full — dropping ACK seq=%lu\n",
                   (unsigned long) ack->ack_seq_num);
            free(ack);
            ack = NULL;
        }
    }
}