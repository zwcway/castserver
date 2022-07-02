/**
    This file is part of castspeaker
    Copyright (C) 2022-2028  zwcway

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "common/utils.h"
#include "config.h"


int32_t config_chunk_type = CHUNK_QUALITY;
int32_t config_mtu = 1500;
int32_t config_rate = 48000;
int32_t config_bits = 24;

audio_bits_t config_speaker_bits = BIT_24;
audio_rate_t config_speaker_rate = RATE_48000;