#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <signal.h>
#include <common/error.h>
#include <common/log.h>
#include "client.h"
#include "pipe.h"


int exit_thread_flag = 0;
int verbosity = 0;

int sockfd;

static char *sock_path = "/tmp/castspeaker.sock";

LOG_TAG_DECLR("client");

static void show_help(const char *arg0) {
  printf("Usage: %s [-f <file>] [-s <sockpath>]\n", arg0);
  printf("\n");
  printf("         Controller for Castserver.\n");
  printf("\n");
  printf("         -f <file>                 : Audio file format by WAV.\n");
  printf("         -s <sockpath>             : Unix domain socket file for Castserver.\n");
  printf("                                     Default is %s.\n", sock_path);
  printf("         -l <level>                : Log level. Default is 'info'.\n");
  printf("\n");
  exit(1);
}


void signal_handle(int signum) {
  LOGD("SIGNAL %d", signum);
  client_deinit();
  exit(0);
}


void connect_socket(const char *path) {
  struct sockaddr_un remote;

  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("Client: Error on socket() call");
    exit(EERR_SOCKET);
  }

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, path);
  size_t data_len = strlen(remote.sun_path) + sizeof(remote.sun_family);

  if (connect(sockfd, (struct sockaddr *) &remote, data_len) == -1) {
    LOGE("Error on connect call %s : %m", path);
    exit(EERR_SOCKET);
  }
}


int main(int argc, char *argv[]) {
  int error, res;

  char *config_file = "/etc/castspeaker/daemon.conf";
  FILE *read_file = NULL;
  int opt;
  while ((opt = getopt(argc, argv, "f:s:l:Lh")) != -1) {
    switch (opt) {
      case 'L': // speaker list
        config_file = strdup(optarg);
        break;
      case 'f':
        read_file = (optarg[0] == '-') ? stdin : fopen(optarg, "r");
        if (read_file == NULL) {
          fprintf(stderr, "File not exists %s\n", optarg);
          exit(1);
        }
        break;
      case 'l': // log level
        if (0 > log_set_level_from_string(optarg)) {
          fprintf(stderr, "error log level: %s\n", optarg);
          show_help(argv[0]);
        }
        break;
      default:
        show_help(argv[0]);
    }
  }

  if (optind < argc) {
    printf("Expected argument after options\n");
    show_help(argv[0]);
  }

  setpriority(PRIO_PROCESS, 0, -11);
  signal(SIGINT, signal_handle);
  signal(SIGKILL, signal_handle);
  signal(SIGSEGV, signal_handle);
  signal(SIGTERM, signal_handle);
  signal(SIGQUIT, signal_handle);

  connect_socket(sock_path);
  if (read_file) {
    pipe_init(read_file);
    exit(0);
  }
};

void client_deinit() {
  exit_thread_flag = 1;

  close(sockfd);
}

void cexit(int no) {
  client_deinit();
  exit(no);
}