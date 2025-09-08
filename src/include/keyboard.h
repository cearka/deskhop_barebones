/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */

#pragma once

#include "structs.h"
#include "hid_parser.h"

/*==============================================================================
 *  Data Extraction
 *==============================================================================*/

int32_t    extract_bit_variable(report_val_t *, uint8_t *, int, uint8_t *);
int32_t    extract_kbd_data(uint8_t *, int, uint8_t, hid_interface_t *, hid_keyboard_report_t *);
keyboard_t *get_keyboard(hid_interface_t *iface, uint8_t report_id);

/*==============================================================================
 *  Hotkey Handling
 *==============================================================================*/

bool key_in_report(uint8_t, const hid_keyboard_report_t *);
bool check_f17_hotkey(const hid_keyboard_report_t *);

/*==============================================================================
 *  Keyboard State Management
 *==============================================================================*/
void     update_kbd_state(device_t *, hid_keyboard_report_t *, uint8_t);
void     update_remote_kbd_state(device_t *, hid_keyboard_report_t *);
void     combine_kbd_states(device_t *, hid_keyboard_report_t *);

/*==============================================================================
 *  Keyboard Report Processing
 *==============================================================================*/
bool     key_in_report(uint8_t, const hid_keyboard_report_t *);
void     process_consumer_report(uint8_t *, int, uint8_t, hid_interface_t *);
void     process_keyboard_report(uint8_t *, int, uint8_t, hid_interface_t *);
void     process_system_report(uint8_t *, int, uint8_t, hid_interface_t *);
void     queue_cc_packet(uint8_t *, device_t *);
void     queue_kbd_report(hid_keyboard_report_t *, device_t *);
void     queue_system_packet(uint8_t *, device_t *);
void     release_all_keys(device_t *);
void     send_consumer_control(uint8_t *, device_t *);
void     send_key(hid_keyboard_report_t *, device_t *);

/* ==================================================== *
 * Map hotkeys to alternative layouts
 * ==================================================== */

