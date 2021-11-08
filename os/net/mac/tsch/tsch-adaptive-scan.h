#ifndef SCANIMPL_H
#define SCANIMPL_H

#include "contiki-conf.h"
#include "net/mac/tsch/tsch-conf.h"

#ifndef ADAPTIVE_SCAN
#define ADAPTIVE_SCAN 0
#endif
#define OPTIMAL_CHANNEL_SCAN_DURATION (sizeof(TSCH_JOIN_HOPPING_SEQUENCE) * SLOTFRAME_DURATION_CS)

/* Broadcast channel bias extent */
#ifndef BROADCAST_BIAS
#define BROADCAST_BIAS 3
#endif

#if TSCH_CONF_ADAPTIVE_TIMESYNC
#error "Adaptive timesync must be disabled for adaptive scan to work"
#endif

// typedef unsigned __int128 uint128_t;

typedef uint8_t history_t;
#define HISTORY_SIZE sizeof(history_t) * 8

typedef struct
{
    history_t history;
    clock_time_t time[HISTORY_SIZE];
    int _time_last_idx;
    clock_time_t _time_cache;
    double _score_cache;
} channel_info;

void initial_setup();
void associated_scan_t(const clock_time_t  current_channel_since, const uint8_t current_channel);
void before_attempt_setup(clock_time_t * current_channel_since);
int not_associated_scan_t();
void associated_f(int *tsch_is_associated);
bool pre_time_spent(const uint8_t current_channel, const clock_time_t now_time, const clock_time_t current_channel_since);

uint8_t  tsch_calculate_channel_biased(uint8_t tsch_hopping_sequence[], uint16_t tsch_hopping_sequence_length_val);

#endif /* SCANIMPL_H */