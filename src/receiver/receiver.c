#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "receiver.h"
#include "types.h"
#include "utils.h"
#include "statistics.h"

// ─────────────────────────────────────────
// Track seen sequence numbers to avoid
// counting duplicates from retransmissions
// Max unique packets we expect = TARGET_PACKETS + buffer
// ─────────────────────────────────────────
#define MAX_SEQ_TRACK   10000

static uint8_t seen_seq[MAX_SEQ_TRACK];  // 1 = already received this seq

void vReceiverTask(void *pvParameters)
{
    (void) pvParameters;

    printf("[RECEIVER] Task started\n");

    // Clear the seen table
    memset(seen_seq, 0, sizeof(seen_seq));

    // Record start time on first packet
    stat_start_tick = (uint32_t) xTaskGetTickCount();

    Packet_t *pkt = NULL;

    for(;;)
    {
        // 1. Wait for packet from link
        if(xQueueReceive(rx_queue, &pkt, portMAX_DELAY) != pdPASS)
            continue;

        uint32_t seq = pkt->seq_num;

        // 2. Check if this is a duplicate
        if(seq < MAX_SEQ_TRACK && seen_seq[seq] == 1)
        {
            printf("[RECEIVER] DUPLICATE seq=%lu — ignoring\n",
                   (unsigned long) seq);

            // Still send ACK so sender stops retransmitting
            ACK_t *ack = (ACK_t *) malloc(sizeof(ACK_t));
            if(ack != NULL)
            {
                ack->ack_seq_num = seq;
                ack->sender_id   = NODE_2_ID;
                ack->dest_id     = NODE_1_ID;
                xQueueSend(ack_tx_queue, &ack, portMAX_DELAY);
            }

            // Free the copy — not counted
            free(pkt);
            pkt = NULL;
            continue;
        }

        // 3. First time seeing this packet — mark as seen
        if(seq < MAX_SEQ_TRACK)
            seen_seq[seq] = 1;

        // 4. Count bytes (payload only)
        uint16_t payload_bytes = pkt->length - HEADER_SIZE_BYTES;
        stats_packet_received(payload_bytes);

        printf("[RECEIVER] Got packet seq=%lu | len=%u | unique=%lu / %d\n",
               (unsigned long) seq,
               pkt->length,
               (unsigned long) stat_packets_received,
               TARGET_PACKETS);

        // 5. Send ACK
        ACK_t *ack = (ACK_t *) malloc(sizeof(ACK_t));
        if(ack == NULL)
        {
            printf("[RECEIVER] ERROR: malloc failed for ACK\n");
            free(pkt);
            pkt = NULL;
            continue;
        }

        ack->ack_seq_num = seq;
        ack->sender_id   = NODE_2_ID;
        ack->dest_id     = NODE_1_ID;

        printf("[RECEIVER] Sending ACK for seq=%lu\n", (unsigned long) seq);

        if(xQueueSend(ack_tx_queue, &ack, portMAX_DELAY) != pdPASS)
        {
            printf("[RECEIVER] ERROR: ack_tx_queue full\n");
            free(ack);
        }

        // 6. Free the copy
        free(pkt);
        pkt = NULL;

        // 7. Check if simulation is done
        if(stat_packets_received >= TARGET_PACKETS)
        {
            stat_end_tick = (uint32_t) xTaskGetTickCount();
            stats_print_results();
            vTaskEndScheduler();
        }
    }
}