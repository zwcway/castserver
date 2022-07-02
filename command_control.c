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
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include "common/log.h"
#include "command_control.h"
#include "../client/client.h"
#include "common/error.h"
#include "command/command.h"
#include "server.h"


static int listenfd;
static pthread_t ctrl_socket_thread;
static char sock_path[108];
static uint8_t socket_buffer[CONTROL_BUFFER_SIZE];
DECL_THREAD_CMD();


pipe_command_t commands[COMMAND_SIZE] = {
  {"speakers", cmd_speakers},
  {"pushwave", cmd_wave},
};

LOG_TAG_DECLR("server");

int call_command(int connfd, const uint8_t *buf, size_t len)
{
  if (memcmp(buf, CONTROL_HEADER_CNTL, 4) != 0) {
    return 1;
  }
  if (len < 12) {
    return 1;
  }
  int ret;
  char *cmd = (char *) buf + 4;
  void *arg = len > 12 ? cmd + 8 : NULL;

  for (int i = 0; i < sizeof(commands); ++i) {
    if (strncasecmp(cmd, commands[i].cmd, 8) == 0) {
      LOGD("server control command : %s", cmd);
      ret = commands[i].cb(connfd, arg, len - 12);
      return ret;
    }
  }
  return 0;
}

_Noreturn void *thread_control_socket(void *arg)
{
  socklen_t cliun_len;
  struct sockaddr_un cliun = {0};
  ssize_t s, n;
  int connfd;
  struct timeval tv = {0};

  LOGD("control_socket created");

  fd_set rfds;
  while (!exit_thread_flag) {

    cliun_len = sizeof(cliun);
    if ((connfd = accept(listenfd, (struct sockaddr *) &cliun, &cliun_len)) < 0) {
      // TODO
      LOGE("control_socket accept error: %m");
      sleep(1);
      continue;
    }
    if (errno == EINTR) {
      LOGE("control_socket socket: %m");
      goto exit_thread;
    }
    n = (int) recv(connfd, socket_buffer, CONTROL_BUFFER_SIZE, 0);
    if (errno == EINTR) {
      LOGE("control_socket socket: %m");
      goto exit_thread;
    }

    if (n < 0) {
      LOGE("control_socket read error: %m");
      sleep(1);
      continue;
    }
    if (n == 0) {
      LOGD("control_socket read EOF");
      continue;
    }

    CHK_RECV_EXIT_THREAD(socket_buffer);

    LOGD("received: %d %s", n, socket_buffer);
    call_command(connfd, socket_buffer, n);


  }
  exit_thread:
  LOGD("control_socket quit");
  close(connfd);
  pthread_exit(NULL);
}


int server_cmdctrl_init(const char *sock)
{
  strcpy(sock_path, sock);

  INIT_THREAD_CMD("control_socket");

  if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("control_socket  socket error");
    exit(3);
  }

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX; //必须跟socket的地址族一致
  strcpy(addr.sun_path, sock_path);
  unlink(sock_path);

  if ((bind(listenfd, (struct sockaddr *) &addr, SUN_LEN(&addr))) < 0) {
    perror("control_socket  bind error");
    exit(4);
  }

  if (listen(listenfd, 10) < 0) {
    perror("control_socket  listen error");
    xexit(1);
  }

  if ((pthread_create(&ctrl_socket_thread, NULL, thread_control_socket, NULL)) < 0) {
    perror("create control_socket thread error");
    xexit(EERR_THREAD);
  }
  pthread_detach(ctrl_socket_thread);

  return 0;
}

int server_cmdctrl_deinit()
{
  LOGD("control_socket deinit");

  WRITE_EXIT_THREAD_CMD();
  DEINIT_THREAD_CMD();

  unlink(sock_path);

  return 0;
}