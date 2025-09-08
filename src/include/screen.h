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

#include <stdint.h>


/*==============================================================================
 *  Constants
 *==============================================================================*/


/*==============================================================================
 *  Data Structures
 *==============================================================================*/



typedef struct {
    uint32_t number;           // Number of this output (e.g. OUTPUT_A = 0 etc)
    int32_t speed_x;           // Mouse speed per output, in direction X
    int32_t speed_y;           // Mouse speed per output, in direction Y
    uint8_t os;                // Operating system on this output
} output_t;
