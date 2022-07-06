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


#include "../command_control.h"
#include <stdio.h>
#include <unistd.h>
#include "list_speakers.h"
#include "client.h"


int list_speakers() {
  char *send_buf = CONTROL_HEADER_CNTL
  "speakers";
  char recv_buf[1024] = {0};

  send(sockfd, send_buf, strlen(send_buf), 0);

  ssize_t len = recv(sockfd, recv_buf, 1024, 0);
  if (len < 0) {
    perror("recv");
    cexit(1);
  }

}