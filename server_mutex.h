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


#ifndef SERVER_MUTEX_H
#define SERVER_MUTEX_H


#include "common/ip.h"

#define MUTEX_TAG         "castspeaker server"
#define MUTEX_TAG_SIZE    18
#define MUTEX_RSP         "hey, kill yourself"
#define MUTEX_RSP_SIZE    18


int mutex_init(interface_t *inface);

int mutex_deinit();

#endif //MUTEX_H
