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


#include "server_http.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common/log.h"


#define PAGE "<html><head><title>libmicrohttpd demo</title>"\
             "</head><body>libmicrohttpd demo</body></html>"

static struct MHD_Daemon *d = NULL;
static char root_path[128] = {0};

LOG_TAG_DECLR("server");

static int ahc_echo(void *cls,
                    struct MHD_Connection *connection,
                    const char *url,
                    const char *method,
                    const char *version,
                    const char *upload_data,
                    size_t *upload_data_size,
                    void **ptr)
{
  static int dummy;
  const char *page = cls;
  struct MHD_Response *response;
  struct stat buf;
  int fd;
  int ret;
  char file_path[128] = {0};

  if (0 != strcmp(method, MHD_HTTP_METHOD_GET) &&
      (0 != strcmp(method, MHD_HTTP_METHOD_HEAD)))
    return MHD_NO; /* unexpected method */

  if (&dummy != *ptr) {
    /* The first time only the headers are valid,
       do not respond in the first round... */
    *ptr = &dummy;
    return MHD_YES;
  }
  if (0 != *upload_data_size)
    return MHD_NO; /* upload data in a GET!? */

  *ptr = NULL; /* clear context pointer */

  if (NULL != strstr(url, "./")) {
    fd = -1;
  } else {
    strcpy(file_path, root_path);
    strcat(file_path, url);
    fd = open(file_path, O_RDONLY);
  }

  if (0 < fd) {
    if ((0 != fstat(fd, &buf)) || (!S_ISREG (buf.st_mode))) {
      if (0 != close(fd)) abort();
      fd = -1;
    }
  }
  if (-1 == fd) {
    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_PERSISTENT);
  } else {
    response = MHD_create_response_from_fd64(buf.st_size, fd);
    if (NULL == response) {
      if (0 != close(fd)) abort();
      return MHD_NO;
    }
  }
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;
}

int server_http_init(const http_config_t *cfg)
{
  uint16_t port = 4415;
  if (cfg != NULL) {
    port = cfg->port;
    strcpy(root_path, cfg->root_path);
  } else {
    strcpy(root_path, "/tmp");
  }

  d = MHD_start_daemon(MHD_USE_EPOLL_INTERNAL_THREAD,
                       port,
                       NULL,
                       NULL,
                       &ahc_echo,
                       PAGE,
                       MHD_OPTION_END);

  return 0;
}

int server_http_deinit()
{
  if (d) MHD_stop_daemon(d);

  return 0;
}