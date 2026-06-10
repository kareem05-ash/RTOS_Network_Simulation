#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "sender.h"
#include "types.h"
#include "utils.h"
#include "statistics.h"

void vSenderTask(void *pvParameters)
{
    (void) pvParameters;
    printf("[SENDER] task started\n");

    Packet_t *tx_buffer = NULL;

    for(;;)
    {
        // 1. Get next packet from generator — sender OWNS this
        xQueueReceive(packet_queue, &tx_buffer, portMAX_DELAY);

        printf("[SENDER] Got packet seq=%lu | len=%u from queue\n",
               (unsigned long) tx_buffer->seq_num, tx_buffer->length);

        uint8_t attempts = 0;
        uint8_t done     = 0;

        while(!done)
        {
            attempts++;
            printf("[SENDER] Transmitting seq=%lu | attempt=%u\n",
                   (unsigned long) tx_buffer->seq_num, attempts);

            stats_transmission_attempt();

            // 2. Make a COPY of the packet for this transmission
            // Link and receiver will own and free this copy
            Packet_t *tx_copy = (Packet_t *) malloc(tx_buffer->length);
            if(tx_copy == NULL)
            {
                printf("[SENDER] ERROR: malloc failed for copy\n");
                continue;
            }
            memcpy(tx_copy, tx_buffer, tx_buffer->length);

            // 3. Send the COPY to link — not the original
            xQueueSend(tx_link_queue, &tx_copy, portMAX_DELAY);

            // 4. Wait for ACK with Tout timeout
            ACK_t *ack = NULL;
            BaseType_t got_ack = xQueueReceive(
                ack_rx_queue,
                &ack,
                pdMS_TO_TICKS(ACTIVE_TOUT_MS)
            );

            if(got_ack == pdPASS)
            {
                if(ack->ack_seq_num == tx_buffer->seq_num)
                {
                    printf("[SENDER] ACK received for seq=%lu ✅\n",
                           (unsigned long) tx_buffer->seq_num);
                    free(ack);
                    free(tx_buffer);  // sender frees original ✅
                    tx_buffer = NULL;
                    done = 1;
                }
                else
                {
                    printf("[SENDER] Old ACK seq=%lu ignoring\n",
                           (unsigned long) ack->ack_seq_num);
                    free(ack);
                    attempts--;  // don't count this as a real attempt
                }
            }
            else
            {
                // Timeout
                printf("[SENDER] Timeout for seq=%lu | attempt=%u\n",
                       (unsigned long) tx_buffer->seq_num, attempts);

                if(attempts >= MAX_RETRANSMISSIONS)
                {
                    printf("[SENDER] Max attempts for seq=%lu — DISCARDING ❌\n",
                           (unsigned long) tx_buffer->seq_num);
                    stats_packet_dropped_4tx();
                    free(tx_buffer);  // sender frees original ✅
                    tx_buffer = NULL;
                    done = 1;
                }
                // else loop → retransmit with fresh copy
            }
        }
    }
}