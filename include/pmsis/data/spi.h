/*
 * Copyright (C) 2018 GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RT_DATA_SPIM_H__
#define __RT_DATA_SPIM_H__

#ifndef LANGUAGE_ASSEMBLY

#include "pmsis/task.h"

typedef struct {
  pi_task_t *pending_copy;
  pi_task_t *waiting_first;
  pi_task_t *waiting_last;
  unsigned int pending_repeat_base;
  unsigned int pending_repeat_len;
  unsigned int pending_repeat_callback;
  unsigned int pending_repeat_asm_callback;
  unsigned int pending_repeat_misaligned_size;
  unsigned int pending_repeat_misaligned_addr;
  unsigned int pending_repeat_misaligned_end;
  unsigned int udma_cmd[4];
  int open_count;
  int id;
  pi_task_t task;
  unsigned int pending_repeat_addr;
  unsigned int pending_repeat_dup_addr;
  unsigned int pending_repeat_send;
  unsigned int pending_repeat_flags;
  uint32_t buffer;
  uint32_t prefix;
  uint32_t suffix;
  struct pi_device *pending_repeat_device;
  uint32_t periph_base;
  uint32_t rx_cmd;
  int channel;
  int pending_is_auto;
} pi_spim_t;

#endif

#define PI_SPIM_T_PENDING_COPY          0
#define PI_SPIM_T_WAITING_FIRST         4
#define PI_SPIM_T_WAITING_LAST          8
#define PI_SPIM_T_REPEAT_BASE           12
#define PI_SPIM_T_REPEAT_LEN            16
#define PI_SPIM_T_REPEAT_CALLBACK       20
#define PI_SPIM_T_REPEAT_ASM_CALLBACK   24

#endif
