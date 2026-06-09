#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "receiver.h"
#include "types.h"
#include "utils.h"
#include"statistics.h"

// ─────────────────────────────────────────
// Receiver Task
// Receives packets from link (rx_queue)
// Sends ACK back through link (ack_tx_queue)
// Counts packets until TARGET_PACKETS reached
// ─────────────────────────────────────────
void vReceiverTask(void *pvParameters)
{
    (void) pvParameters;

    printf("[RECEIVER] Task started\n");

    // ── Counter ──
    stat_start_tick = (uint32_t) xTaskGetTickCount();

    Packet_t *pkt = NULL;

    for(;;)
    {
        // Wait for a packet from the link ──
        if(xQueueReceive(rx_queue, &pkt, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        printf("[RECEIVER] Got packet seq=%lu | len=%u bytes\n",
               (unsigned long) pkt->seq_num, pkt->length);

        uint16_t payload_bytes = pkt->length - HEADER_SIZE_BYTES;
        stats_packet_received(payload_bytes);   

    printf("[RECEIVER] Total received = %lu / %d packets\n",
       (unsigned long) stat_packets_received, TARGET_PACKETS);

        // Build and send ACK back ──
        ACK_t *ack = (ACK_t *) malloc(sizeof(ACK_t));

        if(ack == NULL)
        {
            printf("[RECEIVER] ERROR: malloc failed for ACK seq=%lu\n",
                   (unsigned long) pkt->seq_num);

            // Free packet and continue — can't send ACK
            free(pkt);
            pkt = NULL;
            continue;
        }

        // Fill ACK fields
        ack->ack_seq_num = pkt->seq_num;  // ACKing this sequence number
        ack->sender_id   = NODE_2_ID;     // ACK sent by Node 2
        ack->dest_id     = NODE_1_ID;     // ACK going to Node 1

        printf("[RECEIVER] Sending ACK for seq=%lu\n",
               (unsigned long) ack->ack_seq_num);

        // Send ACK to link ACK path
        if(xQueueSend(ack_tx_queue, &ack, portMAX_DELAY) != pdPASS)
        {
            printf("[RECEIVER] ERROR: ack_tx_queue full — ACK seq=%lu lost\n",
                   (unsigned long) ack->ack_seq_num);
            free(ack);
            ack = NULL;
        }

        // Free the received packet ──
        pkt = NULL;

        // Check if simulation is done ──
       if(stat_packets_received >= TARGET_PACKETS)
{
    stat_end_tick = (uint32_t) xTaskGetTickCount();
    stats_print_results();
    vTaskEndScheduler();
}
    }
}