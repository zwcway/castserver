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
#include <stdint-gcc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <common/error.h>
#include <errno.h>
#include "pipe.h"
#include "common/utils.h"
#include "../server/command_control.h"
#include "common/codec/wave.h"
#include "client.h"


#define BUFFER_MAX_SIZE CONTROL_BUFFER_SIZE
static char buffer[BUFFER_MAX_SIZE];

LOG_TAG_DECLR("client");

int pipe_init(FILE *fd) {
  size_t read_size = BUFFER_MAX_SIZE;
  int32_t sleep_time;
  uint8_t *send_buf = xmalloc(read_size);
  size_t len = 0, offset = 12;

  memcpy(send_buf, CONTROL_HEADER_CNTL, 4);
  memcpy(send_buf + 4, "pushwave", 8);

  fread((void *) (send_buf + offset), sizeof(char), read_size - offset, fd);

  size_t header_size = header_check((uint8_t *) (send_buf + offset), read_size - offset);
  if (header_size == 0) {
    LOGF("invalid file\n");
    exit(1);
  }
  wave_chunk_t chunk;
  get_chunksize(&chunk, CHUNK_QUALITY);

  if (send(sockfd, send_buf, read_size, 0) < 0) {
    LOGE("Client: Error on send() call :%m");
  }

  free(send_buf);
  send_buf = xmalloc(chunk.chunk);
  while ((len = fread((void *) send_buf, sizeof(char), chunk.chunk, fd)) > 0) {
    if (send(sockfd, send_buf, chunk.chunk, 0) == -1) {
      LOGE("Client: Error on send() call :%m");
      if (errno == EPIPE) {
        goto exit_thread;
      }
    }
//    printf("read %d\n", chunk);
    if ((recv(sockfd, &sleep_time, 4, 0)) > 0) {
      usleep(chunk.time + sleep_time);
    }
  }
  if ((recv(sockfd, &sleep_time, 4, 0)) > 0) {
    usleep(chunk.time + sleep_time);
  }
  exit_thread:
  free(send_buf);
  close(sockfd);
  if (fd != stdin) fclose(fd);

  if (len < 0) {
    LOGF("read error");
    exit(4);
  }
  exit(0);
}