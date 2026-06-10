#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "packet_generator.h"
#include "types.h"
#include "utils.h"
#include "statistics.h"

void vPacketGeneratorTask(void *pvParameters){
    (void)pvParameters;
    uint32_t seq_num = 0;                           //initialize sequence number 
    printf("GENERATOR task started\n");
    
    for(;;){
        // Pick a Random Packet Length
        uint16_t payload_size = (uint16_t) rand_uniform_int (L1,L2);
        uint16_t total_length = payload_size + HEADER_SIZE_BYTES;
        // Dynamic Allocate 
        Packet_t *pkt = (Packet_t *) malloc (total_length);
        
        if (pkt == NULL){
            printf("GENERATION ERROR : malloc failed for seq %lu\n",(unsigned long) seq_num );
            continue;
        }
        
        //fill the Header
        pkt->sender_id  = NODE_1_ID;
        pkt->dest_id    = NODE_2_ID;
        pkt->length     = total_length;
        pkt->seq_num    = seq_num;
        
        seq_num++;
        stats_packet_generated();
        // Fill payload with dummy data
        memset(pkt->payload, 0xAB, payload_size);

         // Send pointer to packet queue 
        if(xQueueSend(packet_queue, &pkt, portMAX_DELAY) != pdPASS)
        {
            printf("[GENERATOR] ERROR: queue full, dropping seq %lu\n",
                   (unsigned long) seq_num);
            free(pkt);
        }
        else
        {   
            printf("[GENERATOR] Created packet seq=%lu | len=%u bytes\n",
                   (unsigned long) seq_num, total_length);
        }

        // Wait random time between T1 and T2 ms    
        TickType_t delay_ms = (TickType_t) rand_uniform_int(T1_MS, T2_MS);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));

    }
}   