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


#ifndef COMMAND_CONTROL_H
#define COMMAND_CONTROL_H

#include "stddef.h"


#define COMMAND_SIZE 16
#define CONTROL_HEADER_CNTL "cntl"
#define CONTROL_BUFFER_SIZE 4096


typedef struct pipe_command_s
{
    char *cmd;

    int (*cb)(int fd, const void *arg, size_t len);
} pipe_command_t;

int server_cmdctrl_init(const char *sock);
int server_cmdctrl_deinit();

#endif
