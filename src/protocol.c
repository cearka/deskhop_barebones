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

const field_map_t api_field_map[] = {
/* Index, Rdonly, Type, Len, Offset in struct */
    { 0,  true,  UINT8,  1, offsetof(device_t, active_output) },
    { 3,  true,  INT16,  2, offsetof(device_t, mouse_buttons) },

    /* Output A */
    { 10, false, UINT32, 4, offsetof(device_t, config.output[0].number) },
    { 12, false, INT32,  4, offsetof(device_t, config.output[0].speed_x) },
    { 13, false, INT32,  4, offsetof(device_t, config.output[0].speed_y) },
    { 16, false, UINT8,  1, offsetof(device_t, config.output[0].os) },

    /* Output B */
    { 40, false, UINT32, 4, offsetof(device_t, config.output[1].number) },
    { 42, false, INT32,  4, offsetof(device_t, config.output[1].speed_x) },
    { 43, false, INT32,  4, offsetof(device_t, config.output[1].speed_y) },
    { 46, false, UINT8,  1, offsetof(device_t, config.output[1].os) },

    /* Common config */
    { 70, false, UINT32, 4, offsetof(device_t, config.version) },
    { 71, false, UINT8,  1, offsetof(device_t, config.force_mouse_boot_mode) },
    { 72, false, UINT8,  1, offsetof(device_t, config.force_kbd_boot_protocol) },
    { 73, false, UINT8,  1, offsetof(device_t, config.kbd_led_as_indicator) },
    { 74, false, UINT8,  1, offsetof(device_t, config.hotkey_toggle) },
    { 76, false, UINT8,  1, offsetof(device_t, config.enforce_ports) },

    /* Firmware */
    { 78, true,  UINT16, 2, offsetof(device_t, _running_fw.version) },
    { 79, true,  UINT32, 4, offsetof(device_t, _running_fw.checksum) },

    { 80, true,  UINT8,  1, offsetof(device_t, keyboard_connected) },
    { 82, true,  UINT8,  1, offsetof(device_t, relative_mouse) },
};

const field_map_t* get_field_map_entry(uint32_t index) {
    for (unsigned int i = 0; i < ARRAY_SIZE(api_field_map); i++) {
        if (api_field_map[i].idx == index) {
            return &api_field_map[i];
        }
    }

    return NULL;
}


const field_map_t* get_field_map_index(uint32_t index) {
    /* Clamp potential overflows to last element. */
    if (index >= ARRAY_SIZE(api_field_map))
        index = ARRAY_SIZE(api_field_map) - 1;

    return &api_field_map[index];
}

size_t get_field_map_length(void) {
    return ARRAY_SIZE(api_field_map);
}

void _queue_packet(uint8_t *payload, device_t *state, uint8_t type, uint8_t len, uint8_t id, uint8_t inst) {
    hid_generic_pkt_t generic_packet = {
        .instance = inst,
        .report_id = id,
        .type = type,
        .len = len,
    };

    memcpy(generic_packet.data, payload, len);
    queue_try_add(&state->hid_queue_out, &generic_packet);
}

void queue_cfg_packet(uart_packet_t *packet, device_t *state) {
    uint8_t raw_packet[RAW_PACKET_LENGTH];
    write_raw_packet(raw_packet, packet);
    _queue_packet(raw_packet, state, 0, RAW_PACKET_LENGTH, REPORT_ID_VENDOR, ITF_NUM_HID_VENDOR);
}

void queue_cc_packet(uint8_t *payload, device_t *state) {
    _queue_packet(payload, state, 1, CONSUMER_CONTROL_LENGTH, REPORT_ID_CONSUMER, ITF_NUM_HID);
}

void queue_system_packet(uint8_t *payload, device_t *state) {
    _queue_packet(payload, state, 2, SYSTEM_CONTROL_LENGTH, REPORT_ID_SYSTEM, ITF_NUM_HID);
}
