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


#include "../common/audio.h"
#include "../common/speaker_struct.h"
#include "../common/utils.h"
#include <memory.h>
#include "command.h"


int cmd_speakers(int fd, const void *arg, size_t len) {
  uint32_t count = SPEAKER_COUNT(), idx = 0;
  speaker_t *sp, list[count];

  if (count > 0) {
    // TODO lock speaker list

    SPEAKER_FOREACH(sp) {
      if (sp->state > SPEAKER_STAT_NONE && sp->state < SPEAKER_STAT_DELETED) {
        memcpy(&list[idx++], sp, sizeof(speaker_t));
      }
    }

    send(fd, list, count * sizeof(speaker_t), 0);
  }
  return 0;
}

void cmd_speakers_deinit() {

}