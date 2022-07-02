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


#ifndef SERVER_SPEAKER_PUSHER_H
#define SERVER_SPEAKER_PUSHER_H

#include "common/audio.h"
#include "common/speaker_struct.h"


typedef int (*push_port_fn)(const speaker_t *sp);

struct speaker_push_config {
    addr_t *ip;
    sa_family_t family;
};
extern channel_list_t *channel_list;


int server_sppush_init(struct speaker_push_config *cfg);

int server_sppush_deinit();

int server_sppush_push(speaker_line_t line, const uint8_t *samples, size_t len, uint8_t bytes);

int server_sppush_push_channel(speaker_line_t line, audio_channel_t ch, const uint8_t *buf, size_t len);

#endif
