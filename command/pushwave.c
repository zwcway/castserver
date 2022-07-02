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


#include <string.h>
#include <unistd.h>
#include "../common/log.h"
#include "../config.h"
#include "command.h"
#include "../common/codec/wave.h"
#include "../server_speaker_pusher.h"
#include "../server_speaker_control.h"


static int socket_connfd;
uint8_t *data, *buffer;
uint32_t header_size, offset, len;
uint32_t chunk = 0;

LOG_TAG_DECLR("pushwave");

void *thread_task(void *arg)
{

}

int cmd_wave(int connfd, const void *arg, size_t length)
{
  audio_format_t format;

  if (arg == NULL) return 1;

  header_size = header_check(arg, length);
  if (header_size == 0) {
    return 1;
  }

  socket_connfd = connfd;

  chunk = get_chunksize(NULL, config_chunk_type);
  get_format(&format);

  server_spctrl_set_format(get_channel_list(), format.samples_per_sec, format.bits_per_sample);

  int per_simple_chunk_bytes = chunk / format.channels;
  uint8_t bytes = format.bits_per_sample / 8;
  data = (uint8_t *) arg + header_size;
  while ((offset = length - header_size) >= chunk) {
    server_sppush_push(DEFAULT_LINE, data, per_simple_chunk_bytes, bytes);
    length -= chunk;
    data += chunk;
  }
  buffer = (uint8_t *) xmalloc(chunk);
  memcpy(buffer, data, offset);

  while ((len = recv(socket_connfd, (void *) buffer + offset, chunk - offset, 0)) > 0) {
    offset = 0;
    LOGD("recv %d", chunk - offset);
    server_sppush_push(DEFAULT_LINE, buffer, per_simple_chunk_bytes, bytes);

    uint32_t time = 0;
    send(socket_connfd, &time, 4, 0);
  }
  LOGI("wave complete");
  free(buffer);
//
//  timer_quit = false;
//  if ((pthread_create(&timer_pthread, NULL, thread_timer, NULL) )<0 ) {
//    perror("create timer thread error");
//    exit(ERROR_THREAD);
//  }
//  pthread_detach(timer_pthread);

  return 0;
}
