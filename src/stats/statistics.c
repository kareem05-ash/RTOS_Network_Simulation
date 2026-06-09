#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "statistics.h"
#include "types.h"

// ─────────────────────────────────────────
// Define all global counters
// since multiple tasks write to them
// ─────────────────────────────────────────
volatile uint32_t stat_packets_received      = 0;
volatile uint32_t stat_packets_dropped_link  = 0;
volatile uint32_t stat_packets_dropped_4tx   = 0;
volatile uint32_t stat_total_transmissions   = 0;
volatile uint32_t stat_packets_generated     = 0;
volatile uint32_t stat_total_bytes_received  = 0;
volatile uint32_t stat_start_tick            = 0;
volatile uint32_t stat_end_tick              = 0;

// ─────────────────────────────────────────
// Initialize — reset all counters to zero
// Call this once in main() before scheduler
// ─────────────────────────────────────────
void stats_init(void)
{
    stat_packets_received     = 0;
    stat_packets_dropped_link = 0;
    stat_packets_dropped_4tx  = 0;
    stat_total_transmissions  = 0;
    stat_packets_generated    = 0;
    stat_total_bytes_received = 0;
    stat_start_tick           = 0;
    stat_end_tick             = 0;

    printf("[STATS] Initialized all counters to zero\n");
}

// ─────────────────────────────────────────
// Helper functions
// Each task calls these instead of touching
// the counters directly
// ─────────────────────────────────────────

// Called by receiver when packet arrives successfully
void stats_packet_received(uint16_t payload_bytes)
{
    stat_packets_received++;
    stat_total_bytes_received += payload_bytes;
}

// Called by link when it drops a packet
void stats_packet_dropped_link(void)
{
    stat_packets_dropped_link++;
}

// Called by sender when 4 attempts exhausted
void stats_packet_dropped_4tx(void)
{
    stat_packets_dropped_4tx++;
}

// Called by sender every time it transmits
void stats_transmission_attempt(void)
{
    stat_total_transmissions++;
}

// Called by generator every time it creates a packet
void stats_packet_generated(void)
{
    stat_packets_generated++;
}

// ─────────────────────────────────────────
// Print final results
// Called by receiver when 2000 packets done
// ─────────────────────────────────────────
void stats_print_results(void)
{
    // ── Calculate duration ──
    uint32_t duration_ticks = stat_end_tick - stat_start_tick;
    float    duration_sec   = (float) duration_ticks / 1000.0f; // 1 tick = 1ms

    // ── Calculate throughput ──
    // throughput = total bytes received / total time in seconds
    float throughput = (float) stat_total_bytes_received / duration_sec;

    // ── Calculate average transmissions per packet ──
    // total transmissions / total packets generated
    float avg_tx = 0.0f;
    if(stat_packets_generated > 0)
    {
        avg_tx = (float) stat_total_transmissions / (float) stat_packets_generated;
    }

    printf("\n");
    printf("================================================\n");
    printf("         SIMULATION RESULTS                     \n");
    printf("================================================\n");
    printf("Config:\n");
    printf("  P_drop          : %.2f\n",   ACTIVE_P_DROP);
    printf("  Tout            : %d ms\n",  ACTIVE_TOUT_MS);
    printf("  L range         : [%d, %d] bytes\n", L1, L2);
    printf("  Link capacity   : %d bps\n", C_BPS);
    printf("------------------------------------------------\n");
    printf("Packet Counters:\n");
    printf("  Generated       : %lu\n", (unsigned long) stat_packets_generated);
    printf("  Received        : %lu\n", (unsigned long) stat_packets_received);
    printf("  Dropped by link : %lu\n", (unsigned long) stat_packets_dropped_link);
    printf("  Dropped (4 tx)  : %lu\n", (unsigned long) stat_packets_dropped_4tx);
    printf("  Total tx tries  : %lu\n", (unsigned long) stat_total_transmissions);
    printf("------------------------------------------------\n");
    printf("Performance:\n");
    printf("  Total bytes rx  : %lu bytes\n",    (unsigned long) stat_total_bytes_received);
    printf("  Duration        : %.2f sec\n",      duration_sec);
    printf("  Throughput      : %.2f bytes/sec\n", throughput);
    printf("  Avg tx/packet   : %.2f\n",           avg_tx);
    printf("================================================\n");
    printf("RESULT: Pdrop=%.2f Tout=%d Throughput=%.2f AvgTx=%.2f DroppedAfter4=%lu Duration_ms=%lu\n",
           ACTIVE_P_DROP,
           ACTIVE_TOUT_MS,
           throughput,
           avg_tx,
           (unsigned long) stat_packets_dropped_4tx,
           (unsigned long) duration_ticks);
    printf("================================================\n");
}

