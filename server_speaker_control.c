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


#include "server_speaker_control.h"

#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "common/log.h"
#include "common/connection.h"
#include "common/event/event.h"
#include "server.h"
#include "common/error.h"
#include "config.h"


channel_list_t *server_channel_list = 0; // audio source supported channel

static addr_t listen_ip = {AF_INET, .ipv6 = IN6ADDR_ANY_INIT};

static control_package_t control_resp = {0};
static uint8_t buffer[CONTROL_PACKAGE_SIZE] = {0};

static connection_t conn = DEFAULT_CONNECTION_UDP_INIT;

LOG_TAG_DECLR("server");

uint64_t utime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000L + tv.tv_usec;
}

int server_spctrl_sendto_speaker(const speaker_t *speaker, const void *package, const size_t len) {
  size_t s;
  struct sockaddr_storage sp_addr;

  if (speaker == NULL || speaker->dport == 0 || speaker->state != SPEAKER_STAT_ONLINE) {
    LOGI("speaker(%s) offline", addr_ntop(&speaker->ip));
    return 1;
  }

  memset(&sp_addr, 0, sizeof(sp_addr));
  set_sockaddr(&sp_addr, &speaker->ip, speaker->dport);
  s = sendto(conn.read_fd, (void *) package, len, 0, (const struct sockaddr *) &sp_addr, sizeof(sp_addr));
  if (s < 0) {
    LOGE("control speaker sendto error: %m");
    return 1;
  }
  return 0;
}

int server_spctrl_speaker(const speaker_t *speaker, const control_package_t *hd) {
  LOGD("speaker control command: %d", hd->header.cmd);

  control_package_encode(buffer, hd);

  return server_spctrl_sendto_speaker(speaker, buffer, CONTROL_PACKAGE_SIZE);
}

socket_t create_control_socket() {
  socket_t ctrl_sockfd = socket(listen_ip.type, SOCK_DGRAM, IPPROTO_UDP);
  if (ctrl_sockfd < 0) {
    LOGF("speaker control socket error: %m");
    xexit(EERR_SOCKET);
  }
  return ctrl_sockfd;
}

void spctrl_time(const void *package) {
  control_time_t pk;
  control_time_decode(&pk, package);

  speaker_t *sp = find_speaker_by_id(pk.header.spid);
  control_package_t hd = {
      .header.cmd = SPCMD_TIME,
  };
  if (sp == NULL) {
    return;
  }

  hd.time.server = utime();
  hd.time.offset = hd.time.server - sp->conn_time;

  server_spctrl_speaker(sp, &hd);
}

int srv_spctrl_read(connection_t *c, const struct sockaddr_storage *src, socklen_t src_len, const void *package,
                    uint32_t len) {
//  if (len == CONTROL_RESP_SIZE && control_is_cmd(package, SPCMD_CTRL_PORT)) {
//    if (set_speaker_ctrlport((control_resp_t *) package, (const struct sockaddr *) source, addr_len) < 0) {
//      control_resp.cmd = SPCMD_UNKNOWN_SP;
//      control_resp.spid = ((control_resp_t *) package)->spid;
//      sendto(c->read_fd, &control_resp, sizeof(control_package_t), 0, (const struct sockaddr *) source, src_len);
//    }
//    return 0;
//  }
  if (len == CONTROL_TIME_SIZE && control_is_cmd(package, SPCMD_TIME)) {
    spctrl_time(package);
    return 0;
  }

  if (len < 16) {
    LOGW("control error. size %d", len);
    return -1;
  }

  return 0;
}


void server_spctrl_set_time(speaker_t *sp) {
  sp->conn_time = utime();
}

void server_spctrl_format(uint32_t rate, uint32_t bits) {
  speaker_t *s;
  control_package_t hd = {
      .header.cmd = SPCMD_SAMPLE,
      .sample = {
          .bits = bits,
          .rate = rate,
      },
  };

  SPEAKER_FOREACH(s) {
    if (SPEAKER_IS_ONLINE(s)) {
      hd.sample.channel = s->channel;
      server_spctrl_speaker(s, &hd);
    }
  }
}

void server_spctrl_set_format(const channel_list_t *list, uint32_t rate, uint32_t bits) {
  server_channel_list = (channel_list_t *) list;

  if (config_speaker_rate != rate || config_speaker_bits != bits) {
    // TODO turn on reformat rate and bits
  }

  server_spctrl_format(rate, bits);
}

void server_spctrl_sync_time(const speaker_t *sp) {

}

int server_spctrl_init(struct server_spctrl_config_s *cfg) {
  if (cfg == NULL || cfg->family == 0) {
    LOGF("family can not empty");
    xexit(EERR_ARG);
  }
  listen_ip.type = cfg->family;
  if (cfg->ip) memcpy(&listen_ip, cfg->ip, sizeof(addr_t));
  else bzero(&listen_ip.ipv6, sizeof(struct in6_addr));

  conn.family = cfg->family;
  conn.read_cb = srv_spctrl_read;
  conn.read_fd = create_control_socket();
  event_add(&conn);

  return 0;
}


int server_spctrl_deinit() {
  LOGT("control deinit");

  close(conn.read_fd);

  return 1;
}
