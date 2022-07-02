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


#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#include "microhttpd.h"


typedef struct http_config_t
{
    uint16_t port;
    char root_path[128];
} http_config_t;

int server_http_init(const http_config_t *cfg);

int server_http_deinit();

#endif //SERVER_HTTP_H
