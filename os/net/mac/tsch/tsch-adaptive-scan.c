#include "sys/log.h"
#include "lib/random.h"
#include <stdio.h>
#include "net/mac/tsch/tsch-adaptive-scan.h"
#include "net/mac/tsch/tsch.h"
#include "sys/log.h"

#define LOG_MODULE "MAC"
#define LOG_LEVEL LOG_LEVEL_MAC

/* LISTENER */

#include <stdlib.h>
#include <math.h>

static clock_time_t scan_duration;
static unsigned long i = 0, j = 0;
static int connected_idx = 0;
static unsigned long custom_sequence[sizeof(TSCH_JOIN_HOPPING_SEQUENCE)];
static unsigned long try_count = 0;
static clock_time_t total_time = 0;
static unsigned long num_connections = 0;
static clock_time_t channel_scan_time[sizeof(TSCH_JOIN_HOPPING_SEQUENCE)];
static clock_time_t before_channel_time;
static unsigned long attempt_count = 0;
static channel_info channel_average[sizeof(TSCH_JOIN_HOPPING_SEQUENCE)];

#if ADAPTIVE_SCAN_DEBUG
static char scan_data[999];
static char seq_data[999];
static unsigned long len = 0;
#endif


static unsigned long _try_count_history[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static int _last_try_idx = 0;
static double try_average = 0;

void add_try(unsigned long new_try_count){
  _try_count_history[_last_try_idx] = new_try_count;

  try_average += ((double)new_try_count - _try_count_history[_last_try_idx]) / 8;

  if (_last_try_idx >= ((sizeof(_try_count_history)/sizeof(_try_count_history[0])) - 1))
    _last_try_idx = 0;
  else
    _last_try_idx++;
}

/* HISTORY SETUP */
static double _total_average_time_cache = OPTIMAL_CHANNEL_SCAN_DURATION;

void update(channel_info* c, bool set, clock_time_t t)
{
  c->_score_cache = -1;

  c->history <<= 1;
  if (set)
    c->history++;

  long addition = t - c->time[c->_time_last_idx];
  c->_time_cache += addition;
  c->time[c->_time_last_idx] = t;

  if (c->_time_last_idx >= (HISTORY_SIZE-1))
    c->_time_last_idx = 0;
  else
    c->_time_last_idx++;

  // O(1) update accumulates error
  _total_average_time_cache = -1;
  // _total_average_time_cache += ((double)addition / (HISTORY_SIZE * sizeof(TSCH_JOIN_HOPPING_SEQUENCE)));
}

clock_time_t channel_time(channel_info* c) {
  if (c->_time_cache == -1){
    c->_time_cache = 0;
    for (i = 0; i < HISTORY_SIZE; i++)
      c->_time_cache += c->time[i];
    if (ADAPTIVE_SCAN_DEBUG)  LOG_INFO("DATA time mini update %lu c%lu a%ld s%d 11::%lu %lu\n", c->_time_cache, channel_average[8]._time_cache, channel_average[9]._time_cache, OPTIMAL_CHANNEL_SCAN_DURATION);
  }
  return c->_time_cache;
}

static channel_info channel_average[sizeof(TSCH_JOIN_HOPPING_SEQUENCE)];

double total_average_time(){
  // Average time per attempt
  if (_total_average_time_cache < -0.1)
  {
    _total_average_time_cache = 0;
    for (i = 0; i < sizeof(TSCH_JOIN_HOPPING_SEQUENCE); i++)
      _total_average_time_cache += channel_time(&channel_average[i]);
    _total_average_time_cache /= (HISTORY_SIZE * sizeof(TSCH_JOIN_HOPPING_SEQUENCE));
  }
  return _total_average_time_cache;
}

double score(channel_info* c){
  // Connections per second
  if (c->_score_cache < -0.1){
    history_t n = c->history;
    int res=0;
    while (n > 0)
    {
      n &= (n-1);
      res++;
    }
    c->_score_cache = ((channel_time(c) == 0) ? 1 : (double)res  * 1e3 / channel_time(c));
  }
  return c->_score_cache;
}

int true_comparison(const void *a, const void *b) {
    /* reverse sort */
  double counta = score(&channel_average[*(unsigned long *)a]);
  double countb = score(&channel_average[*(unsigned long *)b]);

  if (counta > countb)
    return -1;
  else if (counta < countb)
    return 1;
  else
    return random_rand() % 2 ? +1 : -1;
}

/* HISTORY SETUP end */

void before_attempt_setup(clock_time_t * current_channel_since){
  before_channel_time =  clock_time();
  *current_channel_since = before_channel_time;
  try_count = 0;

  for (i = 0; i < sizeof(TSCH_JOIN_HOPPING_SEQUENCE); ++i)
    channel_scan_time[i] = 0;
}

int not_associated_scan_t(){
  connected_idx = ADAPTIVE_SCAN ? custom_sequence[try_count % sizeof(TSCH_JOIN_HOPPING_SEQUENCE)] : random_rand() % sizeof(TSCH_JOIN_HOPPING_SEQUENCE);
  try_count += 1;
  if (ADAPTIVE_SCAN_DEBUG)  LOG_INFO("DATA channel choice %lu %u\n", TSCH_JOIN_HOPPING_SEQUENCE[connected_idx], connected_idx);
  return connected_idx;
}

bool pre_time_spent(const uint8_t current_channel, const clock_time_t now_time,const clock_time_t  current_channel_since){
      scan_duration = ADAPTIVE_SCAN ? total_average_time() * score(&channel_average[connected_idx]) * 3 : OPTIMAL_CHANNEL_SCAN_DURATION;
      clock_time_t time_spent = now_time - current_channel_since;
      if (current_channel != 0 && time_spent > scan_duration)
      {
        if (ADAPTIVE_SCAN_DEBUG)  LOG_INFO("DATA rtimer pre %lu %lu %lu %lu %d\n", current_channel, time_spent, channel_scan_time[connected_idx], scan_duration, connected_idx);
        channel_scan_time[connected_idx] += time_spent;
      }
      return time_spent >scan_duration;
}

void associated_scan_t(const clock_time_t  current_channel_since, const uint8_t current_channel){
  if (ADAPTIVE_SCAN_DEBUG) LOG_INFO("DATA rtimer no %lu %lu %lu %lu %d\n", current_channel, clock_time() - current_channel_since, channel_scan_time[connected_idx], scan_duration, connected_idx);
  channel_scan_time[connected_idx] += (clock_time() - current_channel_since);
}

void associated_f(int * tsch_is_associated) {
  if (*tsch_is_associated)
  {
    *tsch_is_associated = 0;

    if (attempt_count <= (ADAPTIVE_SCAN ? 256 : 128) && attempt_count % 32 == 0)
      LOG_INFO("Result status: %3lu out of%s 128 benchmark attempts left.\n", (ADAPTIVE_SCAN ? 256:128) - attempt_count, ADAPTIVE_SCAN ?  " 128 warmup +" : "");

    add_try(try_count);
    attempt_count++;
    if (ADAPTIVE_SCAN && attempt_count == 128)
      num_connections = total_time = 0;
    total_time += (clock_time() - before_channel_time);
    num_connections++;
    
    if (attempt_count == (ADAPTIVE_SCAN ? 256 : 128))
      LOG_INFO("[%s] Results: %f seconds per connection.\n", ADAPTIVE_SCAN ? "Adaptive Scan" : "Fixed Scan", (double)total_time / 1000 / num_connections);

#if ADAPTIVE_SCAN_DEBUG
    LOG_INFO("DATA statistics C%d %6.2fS/c %3luCon %8luMs:%4luT,%8luMs %8.1f %5.2fSc %5.1f %7lu %d\n",
            TSCH_JOIN_HOPPING_SEQUENCE[connected_idx],
            (double)total_time / 1000 / num_connections,
            attempt_count,
            (clock_time() - before_channel_time),
            try_count,
            scan_duration,
            total_average_time(),
            score(&channel_average[connected_idx]),
            try_average,
            channel_time(&channel_average[connected_idx]),
            ADAPTIVE_SCAN);

    for (len = i = 0; i < sizeof(TSCH_JOIN_HOPPING_SEQUENCE) && i < 15; ++i)
      len += sprintf(scan_data + len, ",%d:%lu", TSCH_JOIN_HOPPING_SEQUENCE[custom_sequence[i]], channel_scan_time[i]);
    LOG_INFO("DATA scan data %s \n", scan_data);
#endif

    for (i = 0; i < sizeof(TSCH_JOIN_HOPPING_SEQUENCE); ++i)
      if (channel_scan_time[i] > 0)
        update(&channel_average[i], i == connected_idx, channel_scan_time[i]);

    qsort(custom_sequence, sizeof(TSCH_JOIN_HOPPING_SEQUENCE), sizeof(unsigned long), true_comparison);

#if ADAPTIVE_SCAN_DEBUG
    for (len = i = 0; i < sizeof(TSCH_JOIN_HOPPING_SEQUENCE) && i < 15; ++i)
      len += sprintf(seq_data + len, ",%d:%f %lu %u", TSCH_JOIN_HOPPING_SEQUENCE[custom_sequence[i]], score(&channel_average[custom_sequence[i]]), channel_time(&channel_average[custom_sequence[i]]), channel_average[custom_sequence[i]].history);
    LOG_INFO("DATA seq %s \n", seq_data);

    for (len = i = 0; i < sizeof(TSCH_JOIN_HOPPING_SEQUENCE) && i < 15; ++i)
      len += sprintf(seq_data + len, ",%d:%lu", TSCH_JOIN_HOPPING_SEQUENCE[custom_sequence[i]], channel_time(&channel_average[custom_sequence[i]]));
    LOG_INFO("DATA tim channel time %s \n", seq_data);
#endif

  }
}

void initial_setup(){
  for (i = 0; i < sizeof(TSCH_JOIN_HOPPING_SEQUENCE);i++)
  {
    custom_sequence[i] = i;
    channel_average[i].history = -1;
    for (j = 0; j < HISTORY_SIZE;j++)
      channel_average[i].time[j] = OPTIMAL_CHANNEL_SCAN_DURATION;
    channel_average[i]._time_last_idx = 0;
    channel_average[i]._time_cache = OPTIMAL_CHANNEL_SCAN_DURATION * HISTORY_SIZE;
    channel_average[i]._score_cache = (double) 1e3 / OPTIMAL_CHANNEL_SCAN_DURATION;
  }
  /* Randomize sequence */
  qsort(custom_sequence, sizeof(TSCH_JOIN_HOPPING_SEQUENCE), sizeof(unsigned long), true_comparison);
}

/* LISTENER END */

/* COORDINATOR */

#include "lib/random.h"
uint8_t  tsch_calculate_channel_biased(uint8_t tsch_hopping_sequence[], uint16_t tsch_hopping_sequence_length_val){
  uint16_t index_of_offset = random_rand() % tsch_hopping_sequence_length_val;
  while ((random_rand() % (BROADCAST_BIAS + 1)) && (index_of_offset < (tsch_hopping_sequence_length_val - 1)))
    index_of_offset++;
 if (ADAPTIVE_SCAN_DEBUG) LOG_INFO("DATA channel %d %d\n", index_of_offset, tsch_hopping_sequence[index_of_offset]);

  return tsch_hopping_sequence[index_of_offset];
}

/* COORDINATOR END */