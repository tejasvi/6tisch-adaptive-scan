#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Enable Adaptive scan time */
#define ADAPTIVE_SCAN 1

/* Broadcast channel bias extent (integer). 0 to disable */
#define BROADCAST_BIAS 3

/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0x81a5

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

#define TSCH_HOPPING_SEQUENCE_16_16 (uint8_t[]){ 16, 17, 23, 18, 26, 15, 25, 22, 19, 11, 12, 13, 24, 14, 20, 21 }
#define TSCH_HOPPING_SEQUENCE_4_4 (uint8_t[]){ 15, 25, 26, 20 }
#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE TSCH_HOPPING_SEQUENCE_16_16

/* Enable debug logs */
#define ADAPTIVE_SCAN_DEBUG  0

/* Disable adaptive timesync enabled by default in Contiki */
#define TSCH_CONF_ADAPTIVE_TIMESYNC 0

/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 101

#define SLOTFRAME_DURATION_CS ((clock_time_t)(TSCH_SCHEDULE_DEFAULT_LENGTH * 0.01 * CLOCK_SECOND))

/* Reduce the EB period in order to update the network nodes with more agility */
#define TSCH_CONF_EB_PERIOD (4 * SLOTFRAME_DURATION_CS)
#define TSCH_CONF_MAX_EB_PERIOD TSCH_CONF_EB_PERIOD

/* Logging */
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_WARN
#define TSCH_LOG_CONF_PER_SLOT                     1

#endif /* PROJECT_CONF_H_ */