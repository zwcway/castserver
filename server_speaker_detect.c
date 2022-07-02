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

#include <malloc.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "common/log.h"
#include "common/event/event.h"
#include "common/ip.h"
#include "common/package/detect.h"
#include "common/error.h"

#include "server_speaker_detect.h"
#include "server.h"
#include "common/castspeaker.h"
#include "server_mutex.h"
#include "server_speaker_control.h"
#include "config.h"


static interface_t interface = {0};
static addr_t multicast_group = {0};
static uint16_t multicast_port = 0;
static server_detect_fn detect_cb = NULL;

static detect_response_t server_notify_resp = {0};
static pthread_t online_detect_thread;

static connection_t conn = DEFAULT_CONNECTION_UDP_INIT;

LOG_TAG_DECLR("server");

int valid_speaker(detect_request_t *header) {
  speaker_t *sp = NULL;
  control_package_t hd = {0};

  if (header->bits_mask <= 0) {
    LOGW("error speaker bits %d", header->bits_mask);
    return 1;
  }
  if (header->rate_mask <= 0) {
    LOGW("error speaker rate %d", header->rate_mask);
    return 1;
  }

  if (!MASK_ISSET(header->bits_mask, config_speaker_bits)) {
    LOGW("Unsupported speaker bits %u", config_speaker_bits);
  }
  if (!MASK_ISSET(header->rate_mask, config_speaker_rate)) {
    LOGW("Unsupported speaker rate %u", config_speaker_rate);
  }

  sp = find_speaker_by_id(header->id);
  if (sp == NULL) {
    sp = add_speaker(header->id, DEFAULT_LINE, DEFAULT_CHANNEL);

    if (sp == NULL) {
      LOGE("add speaker error");
      return -1;
    }

    SPEAKER_STRUCT_INIT_FROM_HEADER(sp, header, CHANNEL_FRONT_LEFT);

    LOGI("found a new speaker: %u (%s)%s:%d", sp->id, mac_ntop(&sp->mac), addr_ntop(&sp->ip), sp->dport);
    speaker_check_online(sp);

    server_spctrl_connect(sp);

    // 设置 speaker 设备的音频格式
    hd.cmd = SPCMD_SAMPLE;
    hd.sample.bits = config_speaker_bits;
    hd.sample.rate = config_speaker_rate;
    hd.sample.channel = sp->channel;

    server_spctrl_speaker(sp, &hd);

    if (detect_cb != NULL) detect_cb(sp);
  } else {
    speaker_check_online(sp);
  }

  return 0;
}

/*
 * listen multicast to detect speaker
 */
int create_detect_socket() {
  unsigned char multicastTTL = 1;
  struct in6_addr *src_addr = NULL;
  socklen_t sock_len = 0;
  struct sockaddr_storage group_addr = {0};
  struct ipv6_mreq imreq = {0};
  int optname = 0, dont_loop = 0;
  sa_family_t af = interface.ip.type;
  addr_t bind_addr = {.type = af, .ipv6 = IN6ADDR_ANY_INIT};

  int detect_sockfd = socket(af, SOCK_DGRAM, IPPROTO_UDP);
  if (detect_sockfd < 0) {
    LOGF("detect socket error: %m");
    xexit(EERR_SOCKET);
  }

  set_sockaddr(&group_addr, &bind_addr, multicast_port);

  if ((bind(detect_sockfd, (struct sockaddr *) &group_addr, sizeof(group_addr))) < 0) {
    LOGF("detect bind error: %m");
    xexit(EERR_SOCKET);
  }

  if (setsockopt(detect_sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0) {
    LOGE("set option error: %m");
    close(detect_sockfd);
    exit(EERR_SOCKET);
  }

  if (interface.ip.type == AF_INET) {
    sock_len = sizeof(struct in_addr);
    if (setsockopt(detect_sockfd, IPPROTO_IP, IP_MULTICAST_IF, &interface.ip.ipv4.s_addr, sock_len) < 0) {
      LOGE("set option error: %m");
      close(detect_sockfd);
      xexit(EERR_SOCKET);
    }

  } else if (interface.ip.type == AF_INET6) {
    sock_len = sizeof(struct in6_addr);
    if (setsockopt(detect_sockfd, IPPROTO_IP, IPV6_MULTICAST_IF, &interface.ip.ipv6, sock_len) < 0) {
      LOGE("set option error: %m");
      close(detect_sockfd);
      xexit(EERR_SOCKET);
    }
  }

  if (af == AF_INET) {
    ((struct ip_mreq *) &imreq)->imr_interface.s_addr = interface.ip.ipv4.s_addr;
    if (multicast_group.ipv4.s_addr) {
      ((struct ip_mreq *) &imreq)->imr_multiaddr.s_addr = multicast_group.ipv4.s_addr;
    } else {
      ((struct ip_mreq *) &imreq)->imr_multiaddr.s_addr = inet_addr(DEFAULT_MULTICAST_GROUP);
    }
    sock_len = sizeof(struct ip_mreq);
    optname = IP_ADD_MEMBERSHIP;
  } else {
    ((struct ipv6_mreq *) &imreq)->ipv6mr_interface = interface.ifindex;
    sock_len = sizeof(struct ipv6_mreq);
    optname = IPV6_ADD_MEMBERSHIP;
  }

  if ((setsockopt(detect_sockfd, IPPROTO_IP, optname, &imreq, sock_len)) < 0) {
    LOGF("Failed add to multicast group: %m");
    xexit(EERR_SOCKET);
  }

  if (af == AF_INET) {
    optname = IP_MULTICAST_LOOP;
  } else {
    optname = IPV6_MULTICAST_LOOP;
  }
  if (setsockopt(detect_sockfd, IPPROTO_IP, optname, &dont_loop, sizeof(dont_loop)) < 0) {
    LOGF("setsockopt: %m");
    xexit(EERR_SOCKET);
  }

  return detect_sockfd;
}

void multicast_serverinfo() {
  unsigned char multicastTTL = 1;
  struct sockaddr_in group_addr = {0};
  group_addr.sin_family = AF_INET;
  group_addr.sin_addr.s_addr = inet_addr(DEFAULT_MULTICAST_GROUP);
  group_addr.sin_port = htons(DEFAULT_MULTICAST_PORT);

  LOGI("multicast server info(%d) %s", sizeof(server_notify_resp), addr_ntop(&server_notify_resp.addr));
//
//  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//  setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &interface.ip.ipv4.s_addr, sizeof(struct in_addr));
//  if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0) {
//    LOGE("set option error: %m");
//    close(sockfd);
//    exit(ERROR_SOCKET);
//  }
//
//  if (connect(sockfd, &group_addr, sizeof(struct sockaddr)) < 0) {
//    LOGE("connect error : %m");
//    shutdown(sockfd, 0);
//    close(sockfd);
//    return;
//  }
  // multicast
  ssize_t s = sendto(conn.read_fd,
                     &server_notify_resp, sizeof(server_notify_resp),
                     0,
                     (struct sockaddr *) &group_addr, sizeof(struct sockaddr_in));
  if (s < 0) {
    LOGE("sendto error: %m");
  }
  server_notify_resp.type = 0;
//  shutdown(sockfd, 0);
//  close(sockfd);
}

int srv_detect_read(connection_t *c, const struct sockaddr_storage *src, socklen_t src_len, const void *package,
                    uint32_t len) {
  if (len == MUTEX_TAG_SIZE && memcmp(package, MUTEX_TAG, MUTEX_TAG_SIZE) == 0) {
    LOGI("detect another server %s, just kill it.", sockaddr_ntop(src));
    len = sendto(c->read_fd,
                 MUTEX_RSP, MUTEX_RSP_SIZE,
                 0,
                 (const struct sockaddr *) src, src_len);

    if (len < 0) {
      LOGE("detect mutex error: %m");
    }
    return 0;
  }
  if (len != sizeof(detect_request_t)) {
    LOGD("detect receive error. size %d need %d", (uint32_t) len, sizeof(detect_request_t));
    return -1;
  }
  if (valid_speaker((detect_request_t *) package) != 0) {
    return -1;
  }

  server_notify_resp.type = 0;
  // response server addr
  ssize_t n = sendto(c->read_fd,
                     &server_notify_resp, sizeof(server_notify_resp),
                     0,
                     (const struct sockaddr *) src, src_len);
  if (n < 0) {
    LOGE("response server info error: %m");
    return -1;
  }
  if (n != sizeof(server_notify_resp)) {
    LOGE("response server info fail. sended %d/%d", len, sizeof(server_notify_resp));
  }
  LOGD("response server info to %s:%d", sockaddr_ntop(src), sockaddr_port(src));
  return 0;
}

void *thread_online_detect(void *arg) {
  speaker_t *sp;
  while (!exit_thread_flag) {

    SPEAKER_FOREACH(sp) {
      if (sp->time-- == 0) {
        sp->state = SPEAKER_STAT_OFFLINE;
      }
    };

    sleep(5);
  }
  pthread_exit(NULL);
}

int server_detect_init(const struct server_detect_config_s *cfg) {
  if (cfg == NULL || NULL == cfg->interface) {
    xexit(EERR_ARG);
  }
  memcpy(&interface, cfg->interface, sizeof(interface_t));
  if (cfg->multicast_group) {
    memcpy(&multicast_group, cfg->multicast_group, sizeof(addr_t));
  } else {
    if (interface.ip.type == AF_INET) ip_stoa(&multicast_group, DEFAULT_MULTICAST_GROUP);
    else ip_stoa(&multicast_group, DEFAULT_MULTICAST_GROUPV6);
  }

  multicast_port = cfg->multicast_port ? cfg->multicast_port : DEFAULT_MULTICAST_PORT;

  detect_cb = cfg->added_cb;

  conn.read_cb = srv_detect_read;
  conn.read_fd = create_detect_socket();
  event_add(&conn);

  memcpy(&server_notify_resp.addr, &cfg->interface->ip, sizeof(addr_t));
  server_notify_resp.ver = 1;
  server_notify_resp.type = DETECT_TYPE_FIRST_RUN;
  multicast_serverinfo();

  // 创建在线状态监控线程
  pthread_create(&online_detect_thread, NULL, thread_online_detect, NULL);
  pthread_detach(online_detect_thread);

  return 0;
}

int server_detect_deinit() {
  LOGT("detect deinit");

  if (online_detect_thread) {
    pthread_cancel(online_detect_thread);
    online_detect_thread = 0;
  }

  server_notify_resp.type = DETECT_TYPE_EXIT;
  multicast_serverinfo();

  close(conn.read_fd);

  return 0;
}