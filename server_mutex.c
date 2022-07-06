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


#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include "common/log.h"
#include "server_mutex.h"
#include "common/error.h"
#include "common/speaker_struct.h"
#include "common/common.h"
#include "server.h"


static interface_t interface;
static pthread_t mutex_thread;

LOG_TAG_DECLR("server");

void create_mutex() {
  unsigned char multicastTTL = 1;
  struct in_addr src_addr = {0};
  struct sockaddr addr = {0};
  socklen_t addr_len = sizeof(struct sockaddr);
  ssize_t len;
  char buf[sizeof(MUTEX_RSP)] = {0};
  struct timeval tv = {0};

  int mutex_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (mutex_sockfd < 0) {
    LOGF("mutex create socket failed: %m");
    exit(EERR_SOCKET);
  }

  if (setsockopt(mutex_sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicastTTL, sizeof(multicastTTL)) < 0) {
    LOGF("mutex set option error: %m");
    close(mutex_sockfd);
    exit(EERR_SOCKET);
  }

  if (interface.ip.type) {
    src_addr.s_addr = interface.ip.ipv4.s_addr;
    if (setsockopt(mutex_sockfd, IPPROTO_IP, IP_MULTICAST_IF, &src_addr, sizeof(src_addr)) < 0) {
      LOGF("cast set option error: %m");
      close(mutex_sockfd);
      exit(EERR_SOCKET);
    }
  }

  int opt = 0;
  if (setsockopt(mutex_sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt)) < 0) {
    LOGF("setsockopt");
    xexit(EERR_SOCKET);
  }

  // 设置超时
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  if (setsockopt(mutex_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
    LOGF("setsockopt failed:");
    xexit(EERR_SOCKET);
  }

  ((struct sockaddr_in *) &addr)->sin_family = AF_INET;
  ((struct sockaddr_in *) &addr)->sin_addr.s_addr = inet_addr(DEFAULT_MULTICAST_GROUP);
  ((struct sockaddr_in *) &addr)->sin_port = htons(DEFAULT_MULTICAST_PORT);

  // 通知其他实例
  if (sendto(mutex_sockfd, MUTEX_TAG, MUTEX_TAG_SIZE, 0, &addr, addr_len) < 0) {
    LOGF("mutex sendto error: %m");
    exit(1);
  }

  len = recvfrom(mutex_sockfd, buf, sizeof(buf), 0, &addr, &addr_len);

  if (len > 0 && strncasecmp(buf, MUTEX_RSP, MUTEX_RSP_SIZE) == 0) {
    LOGW("there are another server(%s) running. exiting.", inet_ntoa(((struct sockaddr_in *) &addr)->sin_addr));
    close(mutex_sockfd);
    exit(1);
  }

  // timeout or unknown response. no other server
  shutdown(mutex_sockfd, 0);
  close(mutex_sockfd);

}

int mutex_init(interface_t *inface) {
  if (inface == NULL) exit(EERR_ARG);

  memcpy(&interface, inface, sizeof(interface_t));
  LOGD("checking mutex");

  create_mutex();

  return 0;
}

int mutex_deinit()
{

  return 0;
}
