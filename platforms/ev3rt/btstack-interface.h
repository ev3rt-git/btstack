#pragma once

/**
 * Provided interface
 */

void bluetooth_task(intptr_t);
void btstack_db_cache_put(const char *key, const char *value); // Put a data into database cache
void btstack_db_cache_flush(); // Flush the database cache

uint8_t/*FIXME: use bool*/ bnep_channel_send(uint8_t *packet, uint16_t size);

#define HCI_PACKET_BUFFER_SIZE (2048)

/**
 * Required interface
 */

//void btstack_initialize(const char *local_name, const char *pin_code, int rfcomm_mtu);
extern const char   *ev3rt_bluetooth_local_name;
extern const char   *ev3rt_bluetooth_pin_code;
extern const int     btstack_rfcomm_mtu;
//extern const char   *ev3rt_bluetooth_chip_rev;
// TODO: Add bt_init(local_name, pin_code, mtu)

uint32_t btstack_get_time(); // in milliseconds
void btstack_runloop_sleep(uint32_t time); // in milliseconds
void rfcomm_channel_open_callback();
void rfcomm_channel_close_callback();
void bnep_channel_receive_callback(uint8_t *packet, uint16_t size);

void btstack_db_lock();   // Lock database
void btstack_db_unlock(); // Unlock database
void btstack_db_append(const char *key, const char *value); // Append data, (!key) means clean entire database
