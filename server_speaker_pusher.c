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
#include <assert.h>
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

static pcm_header_t pcm_head = {0};

static uint8_t channel_buffer[1500] = {0};

static addr_t listen_ip = {AF_INET, .ipv6 = IN6ADDR_ANY_INIT};
static push_port_fn port_cb = NULL;

static connection_t conn = DEFAULT_CONNECTION_UDP_INIT;
LOG_TAG_DECLR("server");

static socket_t create_pusher_socket() {
  socket_t push_sockfd = socket(listen_ip.type, SOCK_DGRAM, IPPROTO_UDP);
  if (push_sockfd < 0) {
    LOGF("create push socket failed: %m");
    xexit(EERR_SOCKET);
  }

  return push_sockfd;
}

/**
 * 加快消息推送速度，使用connect预连接
 * 每个预连接都会优先创建新的socket
 * @param speaker
 * @return
 */
int server_sppush_connect(speaker_t *speaker) {
  struct sockaddr_storage addr = {0};
  socklen_t len;

  if (!SPEAKER_SUPPORTED(speaker)) {
    LOGE("speaker unspport, can not connect.");
    return ERROR_SPEAKER;
  }
  if (!SPEAKER_IS_ONLINE(speaker)) {
    LOGE("speaker offline, can not connect.");
    return ERROR_SPEAKER;
  }

  len = set_sockaddr(&addr, &speaker->ip, speaker->dport);

  speaker->fd = socket(listen_ip.type, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
  if (speaker->fd < 0) {
    speaker->fd = conn.read_fd;
  }

  if (connect(speaker->fd, (struct sockaddr *) &addr, len) != 0) {
    LOGE("connect speaker %d error: %m", speaker->id);
    return ERROR_SOCKET;
  }

  return OK;
}

int server_sppush_disconnect(speaker_t *speaker) {
  if (speaker == NULL)
    return 0;

  if (speaker->fd < 0 || speaker->fd == conn.read_fd)
    return 0;

  shutdown(speaker->fd, 0);
  close(speaker->fd);

  return 0;
}

int pusher_sendto_speaker(speaker_t *sp, const void *package, const size_t len) {
  struct sockaddr_storage servaddr;
  size_t s;
  if (sp == NULL || sp->dport == 0 || !SPEAKER_IS_ONLINE(sp) || sp->fd <= 0) {
    LOGI("speaker(%s) offline", addr_ntop(&sp->ip));
    speaker_check_online(sp);
    server_sppush_disconnect(sp);
    return 1;
  }
  if (!SPEAKER_SUPPORTED(sp)) {
    server_sppush_disconnect(sp);
    return 1;
  }
  memset(&servaddr, 0, sizeof(servaddr));
  set_sockaddr(&servaddr, &sp->ip, sp->dport);
  s = sendto(sp->fd, (void *) package, len, 0, (const struct sockaddr *) &servaddr, sizeof(struct sockaddr));
  if (s < 0) {
    LOGE("push to speaker error: %m", sp->id);
    return 1;
  }
  return 0;
}

int server_sppush_push_channel(speaker_line_t line, audio_channel_t ch, uint8_t *buf, size_t len) {
  if ((short) package_seq++ == -1) {
    package_seq = 1;
  }

  speaker_list_t *list = get_speaker_list(line, ch);
  if (list == NULL) return 1;

  for (int i = 0; i < list->len; ++i) {
    pcm_header_encode((void *) buf, &pcm_head);

    pusher_sendto_speaker(list->speakers[i], buf, len);
  }

  return 0;
}

int server_sppush_push(speaker_line_t line, const uint8_t *samples, size_t len, const channel_list_t *chlist,
                       uint8_t bits) {
  uint8_t *src, *dst, *end = (uint8_t *) (samples + len - bits);
  uint32_t step = bits * chlist->len;

  assert(PCM_HEADER_SIZE + len < sizeof(channel_buffer));
  assert((int) (len / (chlist->len * bits)) == (len / (chlist->len * bits * 1.0)));
  assert(len / (chlist->len) < config_mtu);

  audio_channel_t ch;

  for (int i = 0; i < chlist->len; ++i) {
    ch = chlist->list[i];
    src = (uint8_t *) (samples + i * bits);
    dst = channel_buffer + PCM_HEADER_SIZE;
    while (src <= end) {
      BITS_CPY(dst, src, bits);
      dst += bits;
      src += step;
    }

    // TODO reformat rate
    server_sppush_push_channel(line, ch, channel_buffer, len + PCM_HEADER_SIZE);
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

  conn.family = cfg->family;
  conn.read_cb = srv_sppush_read;
  conn.read_fd = create_pusher_socket();
//  event_add(&conn);

  return 0;
}

int server_sppush_deinit()
{
  LOGT("deinit");

  close(conn.read_fd);

  return 0;
}
