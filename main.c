// /*
//  * =============================================================================
//  * RTOS Network Simulation — Single-File Version for Linux Terminal
//  * =============================================================================
//  * Runs all 16 experiment combinations (4 P_drop × 4 Tout) in sequence.
//  * After each experiment the user is asked:
//  *   [Enter] → continue to next experiment
//  *   [q]     → quit
//  *
//  * Compile:
//  *   gcc main.c \
//  *     -I./FreeRTOS/include \
//  *     -I./FreeRTOS/portable/ThirdParty/GCC/Posix \
//  *     -I./src/config \
//  *     -I. \
//  *     FreeRTOS/tasks.c FreeRTOS/queue.c FreeRTOS/timers.c \
//  *     FreeRTOS/list.c FreeRTOS/event_groups.c FreeRTOS/stream_buffer.c \
//  *     FreeRTOS/portable/ThirdParty/GCC/Posix/port.c \
//  *     FreeRTOS/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c \
//  *     FreeRTOS/portable/MemMang/heap_3.c \
//  *     -o simulation -lpthread -lrt
//  *
//  *   Run:  ./simulation
//  * =============================================================================
//  */

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  STANDARD INCLUDES
//  * ═══════════════════════════════════════════════════════════════════════════ */
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdint.h>
// #include <iostream>

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  FreeRTOS INCLUDES
//  * ═══════════════════════════════════════════════════════════════════════════ */
// #include "FreeRTOS.h"
// #include "task.h"
// #include "queue.h"
// #include "timers.h"

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 1 — NETWORK PARAMETERS
//  * ═══════════════════════════════════════════════════════════════════════════ */

// #define L1                  500
// #define L2                  1500
// #define T1_MS               100
// #define T2_MS               200
// #define K_BYTES             40
// #define C_BPS               100000
// #define D_MS                5
// #define P_ACK               0.01f
// #define TARGET_PACKETS      2000
// #define MAX_RETRANSMISSIONS 4
// #define HEADER_SIZE_BYTES   8
// #define NODE_1_ID           1
// #define NODE_2_ID           2
// #define PACKET_QUEUE_SIZE   20
// #define LINK_QUEUE_SIZE     10
// #define ACK_QUEUE_SIZE      10
// #define PRIORITY_LINK       3
// #define PRIORITY_SENDER     2
// #define PRIORITY_RECEIVER   2
// #define PRIORITY_GENERATOR  1
// #define MAX_SEQ_TRACK       10000

// typedef struct {
//     uint8_t  sender_id;
//     uint8_t  dest_id;
//     uint16_t length;
//     uint32_t seq_num;
//     uint8_t  payload[];
// } Packet_t;

// typedef struct {
//     uint32_t ack_seq_num;
//     uint8_t  sender_id;
//     uint8_t  dest_id;
//     uint8_t  padding[34];
// } ACK_t;

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 2 — RUNTIME EXPERIMENT CONFIG
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static float    g_active_p_drop  = 0.01f;
// static uint32_t g_active_tout_ms = 150;

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 3 — GLOBAL QUEUE HANDLES
//  * ═══════════════════════════════════════════════════════════════════════════ */

// QueueHandle_t packet_queue   = NULL;
// QueueHandle_t tx_link_queue  = NULL;
// QueueHandle_t rx_queue       = NULL;
// QueueHandle_t ack_tx_queue   = NULL;
// QueueHandle_t ack_rx_queue   = NULL;

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 4 — TASK HANDLES (needed to delete tasks between experiments)
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static TaskHandle_t h_generator = NULL;
// static TaskHandle_t h_sender    = NULL;
// static TaskHandle_t h_link_fwd  = NULL;
// static TaskHandle_t h_link_ack  = NULL;
// static TaskHandle_t h_receiver  = NULL;

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 5 — UTILITY FUNCTIONS
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static int rand_uniform_int(int min, int max)
// {
//     if (max <= min) return min;
//     return min + rand() % (max - min + 1);
// }

// static float rand_uniform_float(void)
// {
//     return (float)rand() / (float)RAND_MAX;
// }

// static int should_drop(float prob)
// {
//     return rand_uniform_float() < prob;
// }

// static uint32_t calc_tx_delay(int length)
// {
//     return (uint32_t)(((uint32_t)length * 8UL * 1000UL) / C_BPS);
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 6 — STATISTICS
//  * ═══════════════════════════════════════════════════════════════════════════ */

// volatile uint32_t stat_packets_received      = 0;
// volatile uint32_t stat_packets_dropped_link  = 0;
// volatile uint32_t stat_packets_dropped_4tx   = 0;
// volatile uint32_t stat_total_transmissions   = 0;
// volatile uint32_t stat_packets_generated     = 0;
// volatile uint32_t stat_total_bytes_received  = 0;
// volatile uint32_t stat_start_tick            = 0;
// volatile uint32_t stat_end_tick              = 0;

// static void stats_init(void)
// {
//     stat_packets_received     = 0;
//     stat_packets_dropped_link = 0;
//     stat_packets_dropped_4tx  = 0;
//     stat_total_transmissions  = 0;
//     stat_packets_generated    = 0;
//     stat_total_bytes_received = 0;
//     stat_start_tick           = 0;
//     stat_end_tick             = 0;
// }

// static void stats_packet_received(uint16_t payload_bytes)
// {
//     stat_packets_received++;
//     stat_total_bytes_received += payload_bytes;
// }

// static void stats_packet_dropped_link(void) { stat_packets_dropped_link++; }
// static void stats_packet_dropped_4tx(void)  { stat_packets_dropped_4tx++;  }
// static void stats_transmission_attempt(void){ stat_total_transmissions++;  }
// static void stats_packet_generated(void)    { stat_packets_generated++;    }

// static void stats_print_results(void)
// {
//     uint32_t duration_ticks = stat_end_tick - stat_start_tick;
//     float    duration_sec   = (float)duration_ticks / 1000.0f;

//     float throughput = (duration_sec > 0.0f)
//         ? (float)stat_total_bytes_received / duration_sec : 0.0f;

//     float    avg_tx = 0.0f;
//     uint32_t packets_attempted = stat_packets_received + stat_packets_dropped_4tx;
//     if (packets_attempted > 0)
//         avg_tx = (float)stat_total_transmissions / (float)packets_attempted;

//     printf("\n");
//     printf("================================================\n");
//     printf("         SIMULATION RESULTS                     \n");
//     printf("================================================\n");
//     printf("Config:\n");
//     printf("  P_drop          : %.2f\n",   g_active_p_drop);
//     printf("  Tout            : %lu ms\n", (unsigned long)g_active_tout_ms);
//     printf("  L range         : [%d, %d] bytes\n", L1, L2);
//     printf("  Link capacity   : %d bps\n", C_BPS);
//     printf("------------------------------------------------\n");
//     printf("Packet Counters:\n");
//     printf("  Generated       : %lu\n", (unsigned long)stat_packets_generated);
//     printf("  Received        : %lu\n", (unsigned long)stat_packets_received);
//     printf("  Dropped by link : %lu\n", (unsigned long)stat_packets_dropped_link);
//     printf("  Dropped (4 tx)  : %lu\n", (unsigned long)stat_packets_dropped_4tx);
//     printf("  Total tx tries  : %lu\n", (unsigned long)stat_total_transmissions);
//     printf("------------------------------------------------\n");
//     printf("Performance:\n");
//     printf("  Total bytes rx  : %lu bytes\n",     (unsigned long)stat_total_bytes_received);
//     printf("  Duration        : %.2f sec\n",       duration_sec);
//     printf("  Throughput      : %.2f bytes/sec\n", throughput);
//     printf("  Avg tx/packet   : %.2f\n",           avg_tx);
//     printf("================================================\n");
//     printf("RESULT: Pdrop=%.2f Tout=%lu Throughput=%.2f AvgTx=%.2f "
//            "DroppedAfter4=%lu Duration_ms=%lu\n",
//            g_active_p_drop,
//            (unsigned long)g_active_tout_ms,
//            throughput, avg_tx,
//            (unsigned long)stat_packets_dropped_4tx,
//            (unsigned long)duration_ticks);
//     printf("================================================\n");
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 7 — PACKET GENERATOR TASK
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static void vPacketGeneratorTask(void *pvParameters)
// {
//     (void)pvParameters;
//     uint32_t seq_num = 0;
//     printf("[GENERATOR] Task started\n");

//     for (;;)
//     {
//         uint16_t payload_size = (uint16_t)rand_uniform_int(L1, L2);
//         uint16_t total_length = payload_size + HEADER_SIZE_BYTES;

//         Packet_t *pkt = (Packet_t *)malloc(total_length);
//         if (pkt == NULL)
//         {
//             printf("[GENERATOR] ERROR: malloc failed\n");
//             vTaskDelay(pdMS_TO_TICKS(10));
//             continue;
//         }

//         pkt->sender_id = NODE_1_ID;
//         pkt->dest_id   = NODE_2_ID;
//         pkt->length    = total_length;
//         pkt->seq_num   = seq_num;
//         seq_num++;
//         stats_packet_generated();
//         memset(pkt->payload, 0xAB, payload_size);

//         if (xQueueSend(packet_queue, &pkt, portMAX_DELAY) != pdPASS)
//         {
//             printf("[GENERATOR] ERROR: queue full\n");
//             free(pkt);
//         }
//         else
//         {
//             printf("[GENERATOR] Created packet seq=%lu | len=%u bytes\n",
//                    (unsigned long)seq_num, total_length);
//         }

//         TickType_t delay_ms = (TickType_t)rand_uniform_int(T1_MS, T2_MS);
//         vTaskDelay(pdMS_TO_TICKS(delay_ms));
//     }
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 8 — SENDER TASK
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static void vSenderTask(void *pvParameters)
// {
//     (void)pvParameters;
//     printf("[SENDER] Task started\n");

//     Packet_t *tx_buffer = NULL;

//     for (;;)
//     {
//         xQueueReceive(packet_queue, &tx_buffer, portMAX_DELAY);

//         printf("[SENDER] Got packet seq=%lu | len=%u\n",
//                (unsigned long)tx_buffer->seq_num, tx_buffer->length);

//         uint8_t attempts = 0;
//         uint8_t done     = 0;

//         while (!done)
//         {
//             attempts++;
//             printf("[SENDER] Transmitting seq=%lu | attempt=%u\n",
//                    (unsigned long)tx_buffer->seq_num, attempts);

//             stats_transmission_attempt();

//             Packet_t *tx_copy = (Packet_t *)malloc(tx_buffer->length);
//             if (tx_copy == NULL)
//             {
//                 printf("[SENDER] ERROR: malloc failed for copy\n");
//                 vTaskDelay(pdMS_TO_TICKS(10));
//                 continue;
//             }
//             memcpy(tx_copy, tx_buffer, tx_buffer->length);
//             xQueueSend(tx_link_queue, &tx_copy, portMAX_DELAY);

//             ACK_t     *ack     = NULL;
//             BaseType_t got_ack = xQueueReceive(
//                 ack_rx_queue, &ack,
//                 pdMS_TO_TICKS(g_active_tout_ms)
//             );

//             if (got_ack == pdPASS)
//             {
//                 if (ack->ack_seq_num == tx_buffer->seq_num)
//                 {
//                     printf("[SENDER] ACK received for seq=%lu\n",
//                            (unsigned long)tx_buffer->seq_num);
//                     free(ack);
//                     free(tx_buffer);
//                     tx_buffer = NULL;
//                     done = 1;
//                 }
//                 else
//                 {
//                     printf("[SENDER] Old ACK seq=%lu ignoring\n",
//                            (unsigned long)ack->ack_seq_num);
//                     free(ack);
//                     attempts--;
//                 }
//             }
//             else
//             {
//                 printf("[SENDER] Timeout for seq=%lu | attempt=%u\n",
//                        (unsigned long)tx_buffer->seq_num, attempts);

//                 if (attempts >= MAX_RETRANSMISSIONS)
//                 {
//                     printf("[SENDER] Max attempts for seq=%lu — DISCARDING\n",
//                            (unsigned long)tx_buffer->seq_num);
//                     stats_packet_dropped_4tx();
//                     free(tx_buffer);
//                     tx_buffer = NULL;
//                     done = 1;
//                 }
//             }
//         }
//     }
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 9 — COMMUNICATION LINK TASKS
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static void vCommLinkForwardTask(void *pvParameters)
// {
//     (void)pvParameters;
//     printf("[LINK_FWD] Task started\n");

//     Packet_t *pkt = NULL;

//     for (;;)
//     {
//         if (xQueueReceive(tx_link_queue, &pkt, portMAX_DELAY) != pdPASS)
//             continue;

//         printf("[LINK_FWD] Received packet seq=%lu | len=%u bytes\n",
//                (unsigned long)pkt->seq_num, pkt->length);

//         uint32_t tx_delay    = calc_tx_delay(pkt->length);
//         uint32_t total_delay = tx_delay + D_MS;

//         printf("[LINK_FWD] Delay = %lu ms\n", (unsigned long)total_delay);
//         vTaskDelay(pdMS_TO_TICKS(total_delay));

//         if (should_drop(g_active_p_drop))
//         {
//             printf("[LINK_FWD] Packet seq=%lu DROPPED\n",
//                    (unsigned long)pkt->seq_num);
//             stats_packet_dropped_link();
//             free(pkt);
//             pkt = NULL;
//             continue;
//         }

//         printf("[LINK_FWD] Packet seq=%lu forwarded\n",
//                (unsigned long)pkt->seq_num);

//         if (xQueueSend(rx_queue, &pkt, portMAX_DELAY) != pdPASS)
//         {
//             printf("[LINK_FWD] ERROR: rx_queue full\n");
//             free(pkt);
//             pkt = NULL;
//         }
//     }
// }

// static void vCommLinkACKTask(void *pvParameters)
// {
//     (void)pvParameters;
//     printf("[LINK_ACK] Task started\n");

//     ACK_t *ack = NULL;

//     for (;;)
//     {
//         if (xQueueReceive(ack_tx_queue, &ack, portMAX_DELAY) != pdPASS)
//             continue;

//         printf("[LINK_ACK] Received ACK for seq=%lu\n",
//                (unsigned long)ack->ack_seq_num);

//         uint32_t tx_delay    = calc_tx_delay(K_BYTES);
//         uint32_t total_delay = tx_delay + D_MS;

//         printf("[LINK_ACK] Delay = %lu ms\n", (unsigned long)total_delay);
//         vTaskDelay(pdMS_TO_TICKS(total_delay));

//         if (should_drop(P_ACK))
//         {
//             printf("[LINK_ACK] ACK seq=%lu DROPPED\n",
//                    (unsigned long)ack->ack_seq_num);
//             free(ack);
//             ack = NULL;
//             continue;
//         }

//         printf("[LINK_ACK] ACK seq=%lu forwarded\n",
//                (unsigned long)ack->ack_seq_num);

//         if (xQueueSend(ack_rx_queue, &ack, portMAX_DELAY) != pdPASS)
//         {
//             printf("[LINK_ACK] ERROR: ack_rx_queue full\n");
//             free(ack);
//             ack = NULL;
//         }
//     }
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 10 — RECEIVER TASK
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static uint8_t seen_seq[MAX_SEQ_TRACK];

// static void vReceiverTask(void *pvParameters)
// {
//     (void)pvParameters;
//     printf("[RECEIVER] Task started\n");

//     memset(seen_seq, 0, sizeof(seen_seq));
//     stat_start_tick = (uint32_t)xTaskGetTickCount();

//     Packet_t *pkt = NULL;

//     for (;;)
//     {
//         if (xQueueReceive(rx_queue, &pkt, portMAX_DELAY) != pdPASS)
//             continue;

//         uint32_t seq = pkt->seq_num;

//         /* Duplicate check */
//         if (seq < MAX_SEQ_TRACK && seen_seq[seq] == 1)
//         {
//             printf("[RECEIVER] DUPLICATE seq=%lu — ignoring\n",
//                    (unsigned long)seq);

//             ACK_t *ack = (ACK_t *)malloc(sizeof(ACK_t));
//             if (ack != NULL)
//             {
//                 ack->ack_seq_num = seq;
//                 ack->sender_id   = NODE_2_ID;
//                 ack->dest_id     = NODE_1_ID;
//                 memset(ack->padding, 0, sizeof(ack->padding));
//                 xQueueSend(ack_tx_queue, &ack, portMAX_DELAY);
//             }
//             free(pkt);
//             pkt = NULL;
//             continue;
//         }

//         if (seq < MAX_SEQ_TRACK)
//             seen_seq[seq] = 1;

//         uint16_t payload_bytes = pkt->length - HEADER_SIZE_BYTES;
//         stats_packet_received(payload_bytes);

//         printf("[RECEIVER] Got packet seq=%lu | len=%u | unique=%lu / %d\n",
//                (unsigned long)seq, pkt->length,
//                (unsigned long)stat_packets_received, TARGET_PACKETS);

//         ACK_t *ack = (ACK_t *)malloc(sizeof(ACK_t));
//         if (ack == NULL)
//         {
//             printf("[RECEIVER] ERROR: malloc failed for ACK\n");
//             free(pkt);
//             pkt = NULL;
//             continue;
//         }

//         ack->ack_seq_num = seq;
//         ack->sender_id   = NODE_2_ID;
//         ack->dest_id     = NODE_1_ID;
//         memset(ack->padding, 0, sizeof(ack->padding));

//         printf("[RECEIVER] Sending ACK for seq=%lu\n", (unsigned long)seq);

//         if (xQueueSend(ack_tx_queue, &ack, portMAX_DELAY) != pdPASS)
//         {
//             printf("[RECEIVER] ERROR: ack_tx_queue full\n");
//             free(ack);
//         }

//         free(pkt);
//         pkt = NULL;

//         /* ── Simulation done: delete ALL tasks then stop scheduler ── */
//         if (stat_packets_received >= TARGET_PACKETS)
//         {
//             stat_end_tick = (uint32_t)xTaskGetTickCount();
//             stats_print_results();

//             /* Delete every other task first so they stop touching queues */
//             if (h_generator != NULL) { vTaskDelete(h_generator); h_generator = NULL; }
//             if (h_sender    != NULL) { vTaskDelete(h_sender);    h_sender    = NULL; }
//             if (h_link_fwd  != NULL) { vTaskDelete(h_link_fwd);  h_link_fwd  = NULL; }
//             if (h_link_ack  != NULL) { vTaskDelete(h_link_ack);  h_link_ack  = NULL; }

//             /* Stop the scheduler — returns to run_experiment() in main */
//             vTaskEndScheduler();

//             /* Delete self after scheduler stops */
//             vTaskDelete(NULL);
//         }
//     }
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 11 — QUEUE CLEANUP HELPER
//  *  Drains and frees any leftover pointers in a queue after an experiment.
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static void drain_packet_queue(QueueHandle_t q)
// {
//     Packet_t *p = NULL;
//     while (xQueueReceive(q, &p, 0) == pdPASS)
//     {
//         if (p) { free(p); p = NULL; }
//     }
// }

// static void drain_ack_queue(QueueHandle_t q)
// {
//     ACK_t *a = NULL;
//     while (xQueueReceive(q, &a, 0) == pdPASS)
//     {
//         if (a) { free(a); a = NULL; }
//     }
// }

// static void cleanup_queues(void)
// {
//     drain_packet_queue(packet_queue);
//     drain_packet_queue(tx_link_queue);
//     drain_packet_queue(rx_queue);
//     drain_ack_queue(ack_tx_queue);
//     drain_ack_queue(ack_rx_queue);

//     vQueueDelete(packet_queue);
//     vQueueDelete(tx_link_queue);
//     vQueueDelete(rx_queue);
//     vQueueDelete(ack_tx_queue);
//     vQueueDelete(ack_rx_queue);

//     packet_queue = tx_link_queue = rx_queue = ack_tx_queue = ack_rx_queue = NULL;
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 12 — RUN ONE EXPERIMENT
//  * ═══════════════════════════════════════════════════════════════════════════ */

// static void run_experiment(void)
// {
//     /* Reset stats and seen-sequence table */
//     stats_init();
//     memset(seen_seq, 0, sizeof(seen_seq));

//     /* Fresh queues */
//     packet_queue  = xQueueCreate(PACKET_QUEUE_SIZE, sizeof(Packet_t *));
//     tx_link_queue = xQueueCreate(LINK_QUEUE_SIZE,   sizeof(Packet_t *));
//     rx_queue      = xQueueCreate(LINK_QUEUE_SIZE,   sizeof(Packet_t *));
//     ack_tx_queue  = xQueueCreate(ACK_QUEUE_SIZE,    sizeof(ACK_t *));
//     ack_rx_queue  = xQueueCreate(ACK_QUEUE_SIZE,    sizeof(ACK_t *));

//     configASSERT(packet_queue  != NULL);
//     configASSERT(tx_link_queue != NULL);
//     configASSERT(rx_queue      != NULL);
//     configASSERT(ack_tx_queue  != NULL);
//     configASSERT(ack_rx_queue  != NULL);

//     /* Spawn tasks — store handles so receiver can delete them */
//     xTaskCreate(vPacketGeneratorTask, "Generator", 1024,
//                 NULL, PRIORITY_GENERATOR, &h_generator);
//     xTaskCreate(vSenderTask,          "Sender",    2048,
//                 NULL, PRIORITY_SENDER,    &h_sender);
//     xTaskCreate(vCommLinkForwardTask, "LinkFwd",   2048,
//                 NULL, PRIORITY_LINK,      &h_link_fwd);
//     xTaskCreate(vCommLinkACKTask,     "LinkACK",   2048,
//                 NULL, PRIORITY_LINK,      &h_link_ack);
//     xTaskCreate(vReceiverTask,        "Receiver",  2048,
//                 NULL, PRIORITY_RECEIVER,  &h_receiver);

//     printf("All tasks spawned — starting scheduler...\n\n");

//     /* Blocks here until vTaskEndScheduler() is called by the receiver */
//     vTaskStartScheduler();

//     /* ── Back in main context: clean up queues before next experiment ── */
//     cleanup_queues();
// }

// /* ═══════════════════════════════════════════════════════════════════════════
//  *  SECTION 13 — MAIN
//  * ═══════════════════════════════════════════════════════════════════════════ */

// int main(void)
// {
//     const float    p_drops[4]  = { 0.01f, 0.02f, 0.04f, 0.08f };
//     const uint32_t touts_ms[4] = { 150,   175,   200,   225   };

//     int exp_num    = 1;
//     int total_exps = 16;

//     printf("══════════════════════════════════════════════\n");
//     printf("   RTOS Network Simulation — 16 Experiments   \n");
//     printf("══════════════════════════════════════════════\n");
//     printf("  4 drop probabilities x 4 timeouts = 16 runs\n");
//     printf("  Each run delivers %d unique packets.\n\n", TARGET_PACKETS);

//     for (int pi = 0; pi < 4; pi++)
//     {
//         for (int ti = 0; ti < 4; ti++)
//         {
//             g_active_p_drop  = p_drops[pi];
//             g_active_tout_ms = touts_ms[ti];

//             printf("\n");
//             printf("══════════════════════════════════════════════\n");
//             printf("   Experiment %2d / %2d                          \n",
//                    exp_num, total_exps);
//             printf("   P_drop = %.2f   |   Tout = %lu ms           \n",
//                    g_active_p_drop, (unsigned long)g_active_tout_ms);
//             printf("└──────────────────────────────────────────────┘\n\n");

//             run_experiment();

//             exp_num++;

//             /* All done */
//             if (exp_num > total_exps)
//             {
//                 printf("\n✅  All 16 experiments completed!\n");
//                 return 0;
//             }

//             /* Ask user to continue */
//             printf("\n──────────────────────────────────────────────\n");
//             printf("  Next → Experiment %d:  P_drop=%.2f | Tout=%lu ms\n",
//                    exp_num,
//                    p_drops[(exp_num - 1) / 4],
//                    (unsigned long)touts_ms[(exp_num - 1) % 4]);
//             printf("  Press [Enter] to continue, or [q] + Enter to quit: ");
//             char buf[8] = {0};
//             if (fgets(buf, sizeof(buf), stdin) != NULL)
//             {
//                 if (buf[0] == 'q' || buf[0] == 'Q')
//                 {
//                     printf("\nExiting after experiment %d.\n",
//                            exp_num - 1);
//                     return 0;
//                 }
//             }
//         }
//     }

//     printf("\nSimulation complete. Goodbye!\n");
//     return 0;
// }

#include <stdio.h>
int main() {
    char name[8];
    printf("Press Enter to continue. 'q' to continue: ");
    if (fgets(name, sizeof(name), stdin) == NULL) {
        printf("Goodbuy!\n");
        return 0;
    }
    printf("%s", name);
    printf("Continue...\n");
    return 0;
}