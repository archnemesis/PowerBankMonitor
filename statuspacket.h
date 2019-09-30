#ifndef STATUSPACKET_H
#define STATUSPACKET_H

#include <stdint.h>

#define MODE_LOAD_TEST 0
#define MODE_DISCHARGING 1
#define MODE_CHARGING 2

#pragma pack(push, 1)
typedef struct status_packet {
  unsigned char a;
  uint8_t mode;
  int16_t current;
  uint16_t temperature;
  uint16_t charge_state;
  uint16_t pack_voltage;
  uint16_t cell_voltage[6];
  unsigned char b;
} status_packet_t;
#pragma pack(pop)

#endif // STATUSPACKET_H
