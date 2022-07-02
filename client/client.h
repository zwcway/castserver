#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "../common/package/pcm.h"

#include "../common/castspeaker.h"
#include "../common/audio.h"
#include "../common/speaker_struct.h"


#define MAXLINE 80
#define BUFFER_LIST_SIZE 16

enum output_type {
    OUTPUT_TYPE_RAW, OUTPUT_TYPE_ALSA, OUTPUT_TYPE_PULSEAUDIO, Jack
};

extern addr_t server_addr;
extern uint32_t server_data_port;
extern uint32_t server_ctrl_port;
extern uint32_t ctrl_sample_chunk;
extern uint32_t ctrl_mtu;

extern int sockfd;

typedef int (*output_send_fn)(channel_header_t *header, const uint8_t *data);

void (*main_loop)(output_send_fn fn);


void client_deinit();

void cexit(int no);

#endif
