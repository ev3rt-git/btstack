#pragma once

/**
 * Provided interface
 */

void bluetooth_task(intptr_t);
void btstack_db_cache_put(const char *key, const char *value); // Put a data into database cache
void btstack_db_cache_flush(); // Flush the database cache

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

void btstack_db_lock();   // Lock database
void btstack_db_unlock(); // Unload database
void btstack_db_modify(const char *key, const char *value); // Modify data, (key && !value) means delete an entry, (!key) means clean entire database
