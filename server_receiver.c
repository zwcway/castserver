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


#include <stdio.h>
#include "common/log.h"
#include "common/connection.h"
#include <sys/socket.h>
#include "common/event/event.h"
#include "server_receiver.h"


static connection_t conn = DEFAULT_CONNECTION_UDP_INIT;

LOG_TAG_DECLR("server");


int receiver_init() {
  conn.read_fd = socket(AF_INET, SOCK_DGRAM, 0);

  event_add(&conn);
}

int receiver_deinit()
{
  LOGD("receiver deinit");
  return 0;
}