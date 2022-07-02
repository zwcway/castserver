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
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "common/log.h"
#include "common/event/event.h"
#include "common/package/control.h"
#include "common/error.h"
#include "server.h"
#include "common/speaker_struct.h"
#include "command_control.h"
#include "server_speaker_pusher.h"
#include "server_speaker_detect.h"
#include "server_mutex.h"
#include "server_speaker_control.h"
#include "server_http.h"


int exit_thread_flag = 0;

audio_rate_t global_speaker_rate = RATE_48000;
audio_bits_t global_speaker_bits = BIT_24;


static char *sock_path = "/tmp/castspeaker.sock";
static char *config_file = "/etc/castspeaker/daemon.conf";
static char multicast_group[16] = {0};
static char *output = NULL;
static uint16_t multicast_port = DEFAULT_MULTICAST_PORT;

LOG_TAG_DECLR("server");


int speaker_added_handle(const speaker_t *sp) {
  if (!sp || !sp->dport) return 1;

  control_package_t hd = {0};
  hd.cmd = SPCMD_SAMPLE;
  hd.sample.bits = global_speaker_bits;
  hd.sample.rate = global_speaker_rate;
  return server_spctrl_speaker(sp, &hd);
}


static void show_help(const char *arg0, int no) {
  printf("\n");
  printf("Usage: %s [-D] [-p <port>] [-i <iface>] [-g <group>]\n", arg0);
  printf("\n");
  printf("         All command line options are optional. Default is to use\n");
  printf("         multicast with group address 239.255.77.77, port 4010.\n");
  printf("\n");
  printf("         -u                        : Use unicast instead of multicast.\n");
  printf("         -p <port>                 : Use <port> instead of default port 4010.\n");
  printf("                                     Applies to both multicast and unicast.\n");
  printf("         -i <iface>                : Use local interface <iface>. Either the IP\n");
  printf("                                     or the interface name can be specified. In\n");
  printf("                                     multicast mode, uses this interface for IGMP.\n");
  printf("                                     In unicast, binds to this interface only.\n");
  printf("         -g <group>                : Multicast group address. Multicast mode only.\n");
  printf("         -m <ivshmem device path>  : Use shared memory device.\n");
  printf("         -P                        : Use libpcap to sniff the packets.\n");
  printf("\n");
  printf("         -o pulse|alsa|jack|raw    : Send audio to PulseAudio, ALSA, Jack or stdout.\n");
  printf("         -d <device>               : ALSA device name. 'default' if not specified.\n");
  printf("         -s <sink name>            : Pulseaudio sink name.\n");
  printf("         -n <stream name>          : Pulseaudio stream name/description.\n");
  printf("         -n <client name>          : JACK client name.\n");
  printf("         -t <latency>              : Target latency in milliseconds. Defaults to 50ms.\n");
  printf("                                     Only relevant for PulseAudio and ALSA output.\n");
  printf("\n");
  printf("         -v                        : Be verbose.\n");
  printf("\n");
  exit(no);
}

void signal_handle(int signum) {
  LOGD("SIGNAL: %d", signum);
  server_deinit();
  //程序退出，进行退出处理操作
  exit(0);
}


int main(int argc, char *argv[]) {
  int error, res;

  int opt, loglevel = -1;
  char *interface_name = NULL;
  char *default_iface_name = NULL;
  char *group_ip = NULL;
  sa_family_t family = AF_INET;
  static addr_t multicast_group = {0};
  static interface_t interface = {0};
  static uint16_t multicast_port = DEFAULT_MULTICAST_PORT;

  log_set_level(LOG_INFO);
  log_add_filter("queue", LOG_WARN);
  log_add_filter("event", LOG_WARN);

  while ((opt = getopt(argc, argv, "DS:c:i:g:p:m:x:o:d:s:n:t:l:Puvh")) != -1) {
    switch (opt) {
      case 'D': // daemon
        break;
      case 'S': // server address
        break;
      case 'c': // config file
        config_file = strdup(optarg);
        break;
      case 'l': // log level
        if (0 > log_set_level_from_string(optarg)) {
          printf("error log level: %s\n", optarg);
          show_help(argv[0], EERR_ARG);
        }
        break;

      case 'i':
        interface_name = strdup(optarg);
        break;
      case 'p':
        multicast_port = (int) strtol(optarg, NULL, 10);
        if (multicast_port <= 0) show_help(argv[0], EERR_ARG);
        break;
      case 'g':
        group_ip = strdup(optarg);
        break;
      case 'h':
        show_help(argv[0], 0);
        break;
      default:
        show_help(argv[0], EERR_ARG);
    }
  }

  if (optind < argc) {
    printf("Expected argument after options\n");
    show_help(argv[0], EERR_ARG);
  }

  if (interface_name) {
    if (get_interface(family, &interface, interface_name) < 0) {
      printf("Invalid interface: %s\n", interface_name);
      exit(EERR_ARG);
    }
  } else {
    default_iface_name = malloc(IF_NAMESIZE);
    if (get_default_interface(family, default_iface_name) <= 0) {
      free(default_iface_name);
      LOGE("Get default interface failed");
      return -1;
    }
    if (get_interface(family, &interface, default_iface_name) < 0) {
      free(default_iface_name);
      printf("Get default interface failed. Please use -i <iface> and try again.\n");
      exit(EERR_ARG);
    }
    free(default_iface_name);
  }

  if (group_ip) {
    ip_stoa(&multicast_group, group_ip);
    free(group_ip);
    if (multicast_group.type != family) {
      printf("The multicast group ip family is ipv6, please use -6 and try again.\n");
      show_help(argv[0], EERR_ARG);
    }
  }
//  setpriority(PRIO_PROCESS, 0, -11);

  signal(SIGINT, signal_handle);
  signal(SIGKILL, signal_handle);
  signal(SIGQUIT, signal_handle);
//  signal(SIGSEGV, handle_signal);
  signal(SIGTERM, signal_handle);

  event_init(EVENT_TYPE_SELECT, EVENT_PROTOCOL_UDP, 4096, 100);

  mutex_init(&interface);
  server_cmdctrl_init(sock_path);
  speaker_init();

  struct server_detect_config_s detect_cfg = {
    .multicast_group = multicast_group.type ? &multicast_group : NULL,
    .interface = &interface,
    .multicast_port = multicast_port,
    .data_port = 0,
    .added_cb = NULL,
  };
  server_detect_init(&detect_cfg);

  struct server_spctrl_config_s ctrl_cfg = {
    .ip = &interface.ip,
    .family = family,
  };
  server_spctrl_init(&ctrl_cfg);

  struct speaker_push_config push_cfg = {
    .ip = &interface.ip,
  };
  server_sppush_init(&push_cfg);

  server_http_init(NULL);
//  sigset_t set;
//  sigemptyset(&set);
//  sigaddset(&set, SIGEXITPROG);
//  pthread_sigmask(SIG_BLOCK, &set, NULL);


  // main loop
  while (!exit_thread_flag) {
    event_start();
  }
  LOGD("server quit");
  server_deinit();
};

void server_deinit() {
  LOGT("server deinit");

  exit_thread_flag = 1;
  server_http_deinit();

  event_deinit();

  mutex_deinit();
  server_cmdctrl_deinit();
  server_sppush_deinit();
  server_spctrl_deinit();
  server_detect_deinit();
  speaker_deinit();
}

void xexit(int no) {
  server_deinit();
  exit(no);
}