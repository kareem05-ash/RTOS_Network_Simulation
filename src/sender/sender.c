#include <stdio.h>
#include <stdlib.h>

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
        //  Get next packet from generator
        xQueueReceive(packet_queue, &tx_buffer, portMAX_DELAY);

        printf("[SENDER] Got packet seq=%lu | len=%u from queue\n",
               (unsigned long) tx_buffer->seq_num, tx_buffer->length);

        uint8_t attempts  = 0;
        uint8_t done      = 0;

        while(!done)
        {
            attempts++;
            printf("[SENDER] Transmitting seq=%lu | attempt=%u\n",
                   (unsigned long) tx_buffer->seq_num, attempts);

            stats_transmission_attempt();

            //  Send packet pointer to link
            Packet_t *send_ptr = tx_buffer;
            xQueueSend(tx_link_queue, &send_ptr, portMAX_DELAY);

            //  Wait for ACK with Tout timeout
            // xQueueReceive blocks until ACK arrives OR timeout expires
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
                    free(ack);       // sender frees ACK
                    free(tx_buffer); // sender frees packet ✅
                    tx_buffer = NULL;
                    done = 1;
                }
                else
                {
                    free(ack);       // wrong ACK — discard it
                    attempts--;
                }
            }
            else
            {
                if(attempts >= MAX_RETRANSMISSIONS)
                {
                    free(tx_buffer); // sender frees after giving up ✅
                    tx_buffer = NULL;
                    done = 1;
                }
            }
        }
    }
}