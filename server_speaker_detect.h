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


#ifndef SERVER_SPEAKER_DETECT_H
#define SERVER_SPEAKER_DETECT_H

#include "common/connection.h"


typedef int (*server_detect_fn)(const speaker_t *sp);

struct server_detect_config_s {
    addr_t *multicast_group;
    interface_t *interface;
    uint16_t multicast_port;
    uint16_t data_port;
    server_detect_fn added_cb;
};


int server_detect_init(const struct server_detect_config_s *cfg);

int server_detect_deinit();

#endif //DETECT_H
