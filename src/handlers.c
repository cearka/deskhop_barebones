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

/* =================================================== *
 * ============  Hotkey Handler Routines  ============ *
 * =================================================== */

/* This is the main hotkey for switching outputs */
void output_toggle_hotkey_handler(device_t *state, hid_keyboard_report_t *report) {
    /* Hotkey switching is always allowed */

    state->active_output ^= 1;
    set_active_output(state, state->active_output);
};










/* ==================================================== *
 * ==========  UART Message Handling Routines  ======== *
 * ==================================================== */

/* Function handles received keypresses from the other board */
void handle_keyboard_uart_msg(uart_packet_t *packet, device_t *state) {
    hid_keyboard_report_t *report = (hid_keyboard_report_t *)packet->data;
    hid_keyboard_report_t combined_report;

    /* Update the keyboard state for the remote device  */
    update_remote_kbd_state(state, report);

    /* Create a combined report from all device states */
    combine_kbd_states(state, &combined_report);

    /* Queue the combined report */
    queue_kbd_report(&combined_report, state);
    state->last_activity[BOARD_ROLE] = time_us_64();
}

/* Function handles received mouse moves from the other board */
void handle_mouse_abs_uart_msg(uart_packet_t *packet, device_t *state) {
    mouse_report_t *mouse_report = (mouse_report_t *)packet->data;
    queue_mouse_report(mouse_report, state);

    state->mouse_buttons   = mouse_report->buttons;

    state->last_activity[BOARD_ROLE] = time_us_64();
}

/* Function handles request to switch output  */
void handle_output_select_msg(uart_packet_t *packet, device_t *state) {
    state->active_output = packet->data[0];
    if (state->tud_connected)
        release_all_keys(state);

    restore_leds(state);
}



/* Process request to update keyboard LEDs */
void handle_set_report_msg(uart_packet_t *packet, device_t *state) {
    /* We got this via serial, so it's stored to the opposite of our board role */
    state->keyboard_leds[OTHER_ROLE] = packet->data[0];

    /* If we have a keyboard we can control leds on, restore state if active */
    if (global_state.keyboard_connected && !CURRENT_BOARD_IS_ACTIVE_OUTPUT)
        restore_leds(state);
}


/* When this message is received, flash the locally attached LED to verify serial comms */
void handle_flash_led_msg(uart_packet_t *packet, device_t *state) {
    blink_led(state);
}

/* When this message is received, wipe the local flash config */
void handle_wipe_config_msg(uart_packet_t *packet, device_t *state) {
    wipe_config();
    load_config(state);
}


/* Process consumer control message */
void handle_consumer_control_msg(uart_packet_t *packet, device_t *state) {
    queue_cc_packet(packet->data, state);
}

/* Process system control message */
void handle_system_control_msg(uart_packet_t *packet, device_t *state) {
    queue_system_packet(packet->data, state);
}

/* Process request to store config to flash */
void handle_save_config_msg(uart_packet_t *packet, device_t *state) {
    save_config(state);
}

/* Process request to reboot the board */
void handle_reboot_msg(uart_packet_t *packet, device_t *state) {
    reboot();
}

/* Decapsulate and send to the other box */
void handle_proxy_msg(uart_packet_t *packet, device_t *state) {
    queue_packet(&packet->data[1], (enum packet_type_e)packet->data[0], PACKET_DATA_LENGTH - 1);
}

/* Process relative mouse command */
void handle_toggle_gaming_msg(uart_packet_t *packet, device_t *state) {
    state->gaming_mode = packet->data[0];
}

/* Process api communication messages */
void handle_api_msgs(uart_packet_t *packet, device_t *state) {
    uint8_t value_idx = packet->data[0];
    const field_map_t *map = get_field_map_entry(value_idx);

    /* If we don't have a valid map entry, return immediately */
    if (map == NULL)
        return;

    /* Create a pointer to the offset into the structure we need to access */
    uint8_t *ptr = (((uint8_t *)&global_state) + map->offset);

    if (packet->type == SET_VAL_MSG) {
        /* Not allowing writes to objects defined as read-only */
        if (map->readonly)
            return;

        memcpy(ptr, &packet->data[1], map->len);
    }
    else if (packet->type == GET_VAL_MSG) {
        uart_packet_t response = {.type=GET_VAL_MSG, .data={[0] = value_idx}};
        memcpy(&response.data[1], ptr, map->len);
        queue_cfg_packet(&response, state);
    }

}

/* Handle the "read all" message by calling our "read one" handler for each type */
void handle_api_read_all_msg(uart_packet_t *packet, device_t *state) {
    uart_packet_t result = {.type=GET_VAL_MSG};

    for (int i = 0; i < get_field_map_length(); i++) {
        result.data[0] = get_field_map_index(i)->idx;
        handle_api_msgs(&result, state);
    }
}



/* Process heartbeat message - simplified without firmware upgrade */
void handle_heartbeat_msg(uart_packet_t *packet, device_t *state) {
    /* Just update the heartbeat, no firmware upgrade functionality */
}


/* ==================================================== *
 * ==============  Output Switch Routines  ============ *
 * ==================================================== */

/* Update output variable, set LED on/off and notify the other board so they are in sync. */
void set_active_output(device_t *state, uint8_t new_output) {
    state->active_output = new_output;
    restore_leds(state);
    send_value(new_output, OUTPUT_SELECT_MSG);

    /* If we were holding a key down and drag the mouse to another screen, the key gets stuck.
       Changing outputs = no more keypresses on the previous system. */
    release_all_keys(state);
}
