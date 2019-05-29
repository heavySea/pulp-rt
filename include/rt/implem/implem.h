/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna and
 * GreenWaves Technologies
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

#ifndef __RT_IMPLEM_IMPLEM_H__
#define __RT_IMPLEM_IMPLEM_H__

/// @cond IMPLEM

#include "rt/implem/utils.h"
#include "rt/implem/hyperram.h"
#include "rt/implem/dma.h"
#include "rt/implem/cluster.h"
#include "rt/implem/udma.h"
#include "rt/implem/cpi.h"

static inline struct pi_task *pi_task(struct pi_task *task)
{
  task->id = FC_TASK_NONE_ID;
  task->arg[0] = (uint32_t)0;
  task->implem.keep = 1;
  return task;
}

/// @endcond

#endif
