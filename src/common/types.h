#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

/* ─────── NETWORK PARAMETERS ──────────── */

// Packet length range (bytes)
#define L1                  500
#define L2                  1500
#define L_AVG               ((L1 + L2) / 2)     // 1000 bytes

// Inter-packet generation time range (ms)
#define T1_MS               100
#define T2_MS               200

// ACK packet fixed size (bytes)
#define K_BYTES             40

// Link capacity (bits/sec)
#define C_BPS               100000

// Propagation delay (ms)
#define D_MS                5

// ACK drop probability (fixed)
#define P_ACK               0.01f

// Packet drop probabilities 
#define P_DROP_1            0.01f
#define P_DROP_2            0.02f
#define P_DROP_3            0.04f
#define P_DROP_4            0.08f

// Timeout values (ms)
#define TOUT_1_MS           150
#define TOUT_2_MS           175
#define TOUT_3_MS           200
#define TOUT_4_MS           225

// ── ACTIVE CONFIG (change these per run) ──
#define ACTIVE_P_DROP       P_DROP_1
#define ACTIVE_TOUT_MS      TOUT_3_MS

// Simulation target
#define TARGET_PACKETS      2000
#define MAX_RETRANSMISSIONS 4

/* ─────────────────────────────────────────
   HEADER STRUCTURE
   sender_id : 1 byte
   dest_id   : 1 byte
   length    : 2 bytes
   seq_num   : 4 bytes
   TOTAL H   : 8 bytes
───────────────────────────────────────── */
#define HEADER_SIZE_BYTES   8

/* ──────── NODE ID──────────────── */
#define NODE_1_ID           1
#define NODE_2_ID           2

/* ──────── PACKET STRUCTURE───────── */
typedef struct {
    uint8_t  sender_id;        
    uint8_t  dest_id;        
    uint16_t length;           
    uint32_t seq_num;       
    uint8_t  payload[];   

} Packet_t;

/* ─────────────────────────────────────────
   ACK STRUCTURE ( K=40 bytes)
───────────────────────────────────────── */
typedef struct {
    uint32_t ack_seq_num;
    uint8_t  sender_id; 
    uint8_t  dest_id;             
    uint8_t  padding[34];      
} ACK_t;

/* ──────── QUEUE HANDLES ────────────── */
extern QueueHandle_t packet_queue;    // Generator  → Sender
extern QueueHandle_t tx_link_queue;   // Sender     → Link (forward)
extern QueueHandle_t rx_queue;        // Link       → Receiver
extern QueueHandle_t ack_tx_queue;    // Receiver   → Link (ACK return)
extern QueueHandle_t ack_rx_queue;    // Link       → Sender (ACK delivered)

/* ─────── TASK PRIORITIES ───────────── */
#define PRIORITY_LINK       3
#define PRIORITY_SENDER     2
#define PRIORITY_RECEIVER   2
#define PRIORITY_GENERATOR  1

/* ──────── QUEUE SIZES ──────────────── */
#define PACKET_QUEUE_SIZE   20
#define LINK_QUEUE_SIZE     10
#define ACK_QUEUE_SIZE      10

#endif 