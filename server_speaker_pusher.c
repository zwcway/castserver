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


#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "common/log.h"
#include "config.h"
#include "common/connection.h"
#include "common/event/event.h"
#include "common/package/pcm.h"
#include "server_speaker_pusher.h"
#include "common/error.h"
#include "common/utils.h"
#include "server.h"


static int package_seq = 1;

static uint8_t *buffer = NULL;
static pcm_header_t pcm_head = {0};

static addr_t listen_ip = {AF_INET, .ipv6 = IN6ADDR_ANY_INIT};
static push_port_fn port_cb = NULL;

static connection_t conn = DEFAULT_CONNECTION_UDP_INIT;
LOG_TAG_DECLR("server");

int pusher_sendto_speaker(speaker_t *sp, const void *package, const size_t len) {
  struct sockaddr_storage servaddr;
  size_t s;
  if (sp == NULL || sp->dport == 0 || sp->state != SPEAKER_STAT_ONLINE) {
    LOGI("speaker(%s) offline", addr_ntop(&sp->ip));
    sp->state = SPEAKER_STAT_OFFLINE;
    return 1;
  }
  memset(&servaddr, 0, sizeof(servaddr));
  set_sockaddr(&servaddr, &sp->ip, sp->dport);
  s = sendto(conn.read_fd, (void *) package, len, 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
  if (s < 0) {
    LOGE("push error: %m");
    return 1;
  }
  return 0;
}

int push_speak(speaker_t *sp, const uint8_t *buf, const size_t len, const uint16_t seq) {
  PCM_HEADER_ENCODE((void *) buf, &pcm_head);

  return pusher_sendto_speaker(sp, buf, len);
}

socket_t create_pusher_socket() {
  socket_t push_sockfd = socket(listen_ip.type, SOCK_DGRAM, IPPROTO_UDP);
  if (push_sockfd < 0) {
    LOGF("create push socket failed: %m");
    xexit(EERR_SOCKET);
  }

  return push_sockfd;
}

int server_sppush_push_channel(speaker_line_t line, audio_channel_t ch, const uint8_t *buf, size_t len)
{
  if ((short) package_seq++ == -1) {
    package_seq = 1;
  }

  speaker_list_t *list = get_speaker_list(line, ch);
  if (list == NULL) return 1;

  for (int i = 0; i < list->len; ++i) {
    push_speak(list->speakers[i], buf, len, package_seq);
  }

  return 0;
}

int server_sppush_push(speaker_line_t line, const uint8_t *samples, size_t len, uint8_t bytes) {
  if (NULL == buffer) return -1;

  uint8_t *body = buffer + PCM_HEADER_SIZE;
  static uint8_t *src, *dst;

  for (int i = 0; i < channel_list->len; ++i) {
    src = (uint8_t *) samples + i * bytes;
    dst = body;
    while (dst - body < len) {
      memcpy(dst, src, bytes);
      dst += bytes;
      src += bytes * channel_list->len;
    }
    // TODO reformat rate
    server_sppush_push_channel(line, channel_list->list[i], buffer, len + PCM_HEADER_SIZE);
  }

  return 0;
}


int srv_sppush_read(connection_t *c, const struct sockaddr_storage *src, socklen_t src_len, const void *package,
                    uint32_t len) {
  int fd = c->read_fd;

  if (len < 16) {
    LOGD("error. size %d", (uint32_t) len);
    return -1;
  }

  // response server addr
//        sendto(push_sockfd, &resp, sizeof(resp), 0, &src, src_len);

  return 0;
}

int server_sppush_init(struct speaker_push_config *cfg) {
  if (cfg == NULL || cfg->family == 0) {
    LOGF("family can not empty");
    xexit(EERR_ARG);
  }
  listen_ip.type = cfg->family;
  if (cfg->ip) memcpy(&listen_ip, cfg->ip, sizeof(addr_t));
  else bzero(&listen_ip.ipv6, sizeof(struct in6_addr));

  buffer = xmalloc(config_mtu);

  conn.family = cfg->family;
  conn.read_cb = srv_sppush_read;
  conn.read_fd = create_pusher_socket();
  event_add(&conn);

  return 0;
}

int server_sppush_deinit()
{
  LOGT("deinit");

  close(conn.read_fd);

  free(buffer);

  return 0;
}