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

#include "main.h"

/* ==================================================== *
 * Hotkeys to trigger actions via the keyboard.
 * ==================================================== */

/* ============================================================ *
 * Key detection utilities
 * ============================================================ */

/* Tries to find if the keyboard report contains key, returns true/false */
bool key_in_report(uint8_t key, const hid_keyboard_report_t *report) {
    for (int j = 0; j < KEYS_IN_USB_REPORT; j++) {
        if (key == report->keycode[j]) {
            return true;
        }
    }
    return false;
}

/* Check if F17 key is pressed for output switching */
bool check_f17_hotkey(const hid_keyboard_report_t *report) {
    return key_in_report(HOTKEY_TOGGLE, report);
}

/* Check if HELP key is pressed for NULL MODE toggle */
bool check_null_mode_hotkey(const hid_keyboard_report_t *report) {
    return key_in_report(HOTKEY_NULL_MODE, report);
}

/* ==================================================== *
 * Keyboard State Management
 * ==================================================== */

/* Update the keyboard state for a specific device */
void update_kbd_state(device_t *state, hid_keyboard_report_t *report, uint8_t device_idx) {
    /* Ensure device_idx is within bounds */
    if (device_idx >= MAX_DEVICES)
        return;

    /* Update the keyboard state for this device */
    memcpy(&state->local_kbd_states[device_idx], report, sizeof(hid_keyboard_report_t));

    /* Track the largest keyboard index we have */
    if (state->max_kbd_idx < device_idx)
        state->max_kbd_idx = device_idx;
}

/* Update the struct storing the state of the keyboard(s) connected to the other board */
void update_remote_kbd_state(device_t *state, hid_keyboard_report_t *report) {
    memcpy(&state->remote_kbd_state, report, sizeof(hid_keyboard_report_t));
}

/* Add keys from source to destination, avoiding duplicates */
static void add_keys(hid_keyboard_report_t *dest, const hid_keyboard_report_t *src) {
    for (uint8_t i = 0; i < KEYS_IN_USB_REPORT; i++) {
        uint8_t key = src->keycode[i];
        
        if (key == 0 || key_in_report(key, dest))
            continue;
            
        uint8_t *empty_slot = memchr(dest->keycode, 0, KEYS_IN_USB_REPORT);
        if (empty_slot)
            *empty_slot = key;
    }
}

/* Release all keys */
void release_all_keys(device_t *state) {
    memset(state->local_kbd_states, 0, sizeof(state->local_kbd_states));
    memset(&state->remote_kbd_state, 0, sizeof(hid_keyboard_report_t));
    
    /* Don't send empty report if NULL MODE is active */
    if (state->null_mode)
        return;
    
    static hid_keyboard_report_t empty_report = {0};
    queue_kbd_report(&empty_report, state);
}


/* Combine all keyboard states into a single report */
void combine_kbd_states(device_t *state, hid_keyboard_report_t *combined_report) {
    memset(combined_report, 0, sizeof(hid_keyboard_report_t));

    /* Combine all local keyboards up to max_kbd_idx */
    for (uint8_t i = 0; i <= state->max_kbd_idx; i++) {
        combined_report->modifier |= state->local_kbd_states[i].modifier;
        add_keys(combined_report, &state->local_kbd_states[i]);
    }
    
    /* Add remote keyboard */
    combined_report->modifier |= state->remote_kbd_state.modifier;
    add_keys(combined_report, &state->remote_kbd_state);
}

/* ==================================================== *
 * Keyboard Queue Section
 * ==================================================== */

void process_kbd_queue_task(device_t *state) {
    hid_keyboard_report_t report;

    /* If NULL MODE is active, don't send any keyboard reports */
    if (state->null_mode)
        return;

    /* If we're not connected, we have nowhere to send reports to. */
    if (!state->tud_connected)
        return;

    /* Peek first, if there is anything there... */
    if (!queue_try_peek(&state->kbd_queue, &report))
        return;

    /* If we are suspended, let's wake the host up */
    if (tud_suspended())
        tud_remote_wakeup();

    /* If it's not ok to send yet, we'll try on the next pass */
    if (!tud_hid_n_ready(ITF_NUM_HID))
        return;

    /* ... try sending it to the host, if it's successful */
    bool succeeded = tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report.modifier, report.keycode);

    /* ... then we can remove it from the queue. Race conditions shouldn't happen [tm] */
    if (succeeded)
        queue_try_remove(&state->kbd_queue, &report);
}

void queue_kbd_report(hid_keyboard_report_t *report, device_t *state) {
    /* It wouldn't be fun to queue up a bunch of messages and then dump them all on host */
    if (!state->tud_connected)
        return;

    queue_try_add(&state->kbd_queue, report);
}

/* If keys need to go locally, queue packet to kbd queue, else send them through UART */
void send_key(hid_keyboard_report_t *report, device_t *state) {
    /* Create a combined report from all device states */
    hid_keyboard_report_t combined_report;
    combine_kbd_states(state, &combined_report);

    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        /* Queue the combined report */
        queue_kbd_report(&combined_report, state);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        /* Send the combined report to ensure all keys are included */
        queue_packet((uint8_t *)&combined_report, KEYBOARD_REPORT_MSG, KBD_REPORT_LENGTH);
    }
}

/* Decide if consumer control reports go local or to the other board */
void send_consumer_control(uint8_t *raw_report, device_t *state) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        queue_cc_packet(raw_report, state);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        queue_packet((uint8_t *)raw_report, CONSUMER_CONTROL_MSG, CONSUMER_CONTROL_LENGTH);
    }
}

/* Decide if consumer control reports go local or to the other board */
void send_system_control(uint8_t *raw_report, device_t *state) {
    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        queue_system_packet(raw_report, state);
        state->last_activity[BOARD_ROLE] = time_us_64();
    } else {
        queue_packet((uint8_t *)raw_report, SYSTEM_CONTROL_MSG, SYSTEM_CONTROL_LENGTH);
    }
}

/* ==================================================== *
 * Parse and interpret the keys pressed on the keyboard
 * ==================================================== */

void process_keyboard_report(uint8_t *raw_report, int length, uint8_t itf, hid_interface_t *iface) {
    hid_keyboard_report_t new_report = {0};
    device_t *state                  = &global_state;

    if (length < KBD_REPORT_LENGTH)
        return;

    /* No more keys accepted if we're about to reboot */
    if (global_state.reboot_requested)
        return;

    extract_kbd_data(raw_report, length, itf, iface, &new_report);

    /* Update the keyboard state for this device */
    update_kbd_state(state, &new_report, itf);

    /* Check if F17 hotkey was pressed for output switching */
    if (check_f17_hotkey(&new_report)) {
        /* Execute the output toggle handler and don't pass key to OS */
        output_toggle_hotkey_handler(state, &new_report);
        return;
    }

    /* Check if HELP hotkey was pressed for NULL MODE toggle */
    if (check_null_mode_hotkey(&new_report)) {
        /* Execute the NULL MODE toggle handler and don't pass key to OS */
        null_mode_toggle_hotkey_handler(state, &new_report);
        return;
    }

    /* If NULL MODE is active, drop all keyboard input */
    if (state->null_mode) {
        return;
    }

    /* This method will decide if the key gets queued locally or sent through UART */
    send_key(&new_report, state);
}

void process_consumer_report(uint8_t *raw_report, int length, uint8_t itf, hid_interface_t *iface) {
    uint8_t new_report[CONSUMER_CONTROL_LENGTH] = {0};
    uint16_t *report_ptr = (uint16_t *)new_report;

    device_t *state = &global_state;

    /* If NULL MODE is active, drop all consumer control input */
    if (state->null_mode) {
        return;
    }
    keyboard_t *keyboard = get_keyboard(iface, raw_report[0]);

    /* If consumer control is variable, read the values from cc_array and send as array. */
    if (iface->consumer.is_variable) {
        for (int i = 0; i < MAX_CC_BUTTONS && i < 8 * (length - 1); i++) {
            int bit_idx = i % 8;
            int byte_idx = i >> 3;

            if ((raw_report[byte_idx + 1] >> bit_idx) & 1) {
                report_ptr[0] = keyboard->cc_array[i];
            }
        }
    }
    else {
        for (int i = 0; i < length - 1 && i < CONSUMER_CONTROL_LENGTH; i++)
            new_report[i] = raw_report[i + 1];
    }

    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        send_consumer_control(new_report, state);
    } else {
        queue_packet((uint8_t *)new_report, CONSUMER_CONTROL_MSG, CONSUMER_CONTROL_LENGTH);
    }
}

void process_system_report(uint8_t *raw_report, int length, uint8_t itf, hid_interface_t *iface) {
    uint16_t new_report = raw_report[1];
    uint8_t *report_ptr = (uint8_t *)&new_report;
    device_t *state = &global_state;

    /* If NULL MODE is active, drop all system control input */
    if (state->null_mode) {
        return;
    }

    if (CURRENT_BOARD_IS_ACTIVE_OUTPUT) {
        send_system_control(report_ptr, state);
    } else {
        queue_packet(report_ptr, SYSTEM_CONTROL_MSG, SYSTEM_CONTROL_LENGTH);
    }
}

keyboard_t *get_keyboard(hid_interface_t *iface, uint8_t report_id) {
    /* When we have just one keyboard (most cases), or don't use report ID */
    if (iface->num_keyboards == 1 || !iface->uses_report_id)
        return &iface->keyboards[PRIMARY_KEYBOARD];

    /* Go through known keyboards and match on report ID, return pointer to keyboard_t */
    for (int i = 0; i < iface->num_keyboards && i < MAX_KEYBOARDS; i++) {
        if (iface->keyboards[i].report_id == report_id) {
            return &iface->keyboards[i];
        }
    }

    /* If nothing else is matched, return the primary keyboard. */
    return &iface->keyboards[PRIMARY_KEYBOARD];
}
