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


#ifndef SERVER_SPEAKER_CONTROL_H
#define SERVER_SPEAKER_CONTROL_H

#include "common/package/control.h"
#include "common/audio.h"
#include "common/speaker_struct.h"


typedef int(*ctrl_port_fn)(const speaker_t *sp);

struct server_spctrl_config_s {
    addr_t *ip;
    sa_family_t family;
};

extern channel_list_t *channel_list;

int server_spctrl_init(struct server_spctrl_config_s *cfg);

int server_spctrl_deinit();

void server_spctrl_set_format(const channel_list_t *list, uint32_t rate, uint32_t bits);

int server_spctrl_speaker(const speaker_t *speaker, const control_package_t *hd);

int server_spctrl_connect(const speaker_t *speaker);

#endif //SPEAKER_CONTROL_H
