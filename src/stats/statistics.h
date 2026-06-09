#ifndef STATISTICS_H
#define STATISTICS_H
#include <stdint.h>

// ─────────────────────────────────────────
// Global statistics counters
// All tasks can read/write these
// ─────────────────────────────────────────

// Packet counters
extern volatile uint32_t stat_packets_received;     // successfully received at Node 2
extern volatile uint32_t stat_packets_dropped_link; // dropped by the link
extern volatile uint32_t stat_packets_dropped_4tx;  // dropped after 4 failed attempts
extern volatile uint32_t stat_total_transmissions;  // every single tx attempt made
extern volatile uint32_t stat_packets_generated;    // total packets created by generator

// Byte counter
extern volatile uint32_t stat_total_bytes_received; // payload bytes at receiver

// Time
extern volatile uint32_t stat_start_tick;           // tick when first packet was sent
extern volatile uint32_t stat_end_tick;             // tick when 2000th packet received

// ─────────────────────────────────────────
// Functions
// ─────────────────────────────────────────

// Call once at start of simulation
void stats_init(void);

// Call at end to print everything
void stats_print_results(void);

// Helpers — call from any task
void stats_packet_received    (uint16_t payload_bytes);
void stats_packet_dropped_link(void);
void stats_packet_dropped_4tx (void);
void stats_transmission_attempt(void);
void stats_packet_generated   (void);

#endif // STATISTICS_H