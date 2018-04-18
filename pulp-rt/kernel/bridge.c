/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna
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

/* 
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#include "rt/rt_api.h"
#include "hal/debug_bridge/debug_bridge.h"

static int __rt_bridge_strlen(const char *str)
{
  int result = 0;
  while (*str)
  {
    result++;
    str++;
  }
  return result;
}



// This function should be called everytime the bridge may have new requests
// to handle, so that its pointer to ready requests is updated
static void __rt_bridge_check_bridge_req()
{
  hal_bridge_t *bridge = hal_bridge_get();

  // first_bridge_req is owned by the runtime only when it is NULL, otherwise
  // it is owned by the bridge which is modifying it everytime a request 
  // is handled
  if (bridge->first_bridge_req == 0)
  {
    // We are owning the pointer, check if we can make it point to ready
    // requests
    rt_bridge_req_t *req = (rt_bridge_req_t *)bridge->first_req;

    while (req && req->header.popped)
    {
      req = (rt_bridge_req_t *)req->header.next;
    }

    if (req)
    {
      bridge->first_bridge_req = (uint32_t )req;
    }
  }
}



// This function can be called by the API layer to commit a new request to 
// the bridge
static void __rt_bridge_post_req(rt_bridge_req_t *req, rt_event_t *event)
{
  hal_bridge_t *bridge = hal_bridge_get();

  req->header.next = 0;
  req->header.done = 0;
  req->header.popped = 0;
  req->header.size = sizeof(rt_bridge_req_t);
  req->event = event;

  if (bridge->first_req)
    ((hal_bridge_req_t *)bridge->last_req)->next = (uint32_t)req;
  else
    bridge->first_req = (uint32_t)req;

  bridge->last_req = (uint32_t)req;
  req->header.next = 0;

  __rt_bridge_check_bridge_req();

}



void rt_bridge_connect(rt_event_t *event)
{
  int irq = hal_irq_disable();

  hal_bridge_t *bridge = hal_bridge_get();

  rt_event_t *call_event = __rt_wait_event_prepare(event);

  bridge->first_req = 0;
  bridge->first_bridge_req = 0;

#ifdef ITC_VERSION
  bridge->notif_req_addr = ARCHI_FC_ITC_ADDR + ARCHI_ITC_STATUS_SET;
  bridge->notif_req_value = 1<<RT_BRIDGE_ENQUEUE_EVENT;
#else
#if defined(EU_VERSION) && EU_VERSION >= 3
#if defined(ARCHI_HAS_FC)
  bridge->notif_req_addr = ARCHI_FC_GLOBAL_ADDR + ARCHI_FC_PERIPHERALS_OFFSET + ARCHI_FC_EU_OFFSET + EU_SW_EVENTS_AREA_BASE + EU_CORE_TRIGG_SW_EVENT + (RT_BRIDGE_ENQUEUE_EVENT << 2);
  bridge->notif_req_value = 1;
#endif
#endif
#endif

  rt_bridge_req_t *req = &call_event->bridge_req;
  hal_bridge_connect(&req->header);
  __rt_bridge_post_req(req, call_event);

  __rt_wait_event_check(event, call_event);

  hal_irq_restore(irq);
}



void rt_bridge_disconnect(rt_event_t *event)
{
  int irq = hal_irq_disable();

  hal_bridge_t *bridge = hal_bridge_get();

  rt_event_t *call_event = __rt_wait_event_prepare(event);

  rt_bridge_req_t *req = &call_event->bridge_req;
  hal_bridge_disconnect(&req->header);
  __rt_bridge_post_req(req, call_event);

  __rt_wait_event_check(event, call_event);

  hal_irq_restore(irq);
}



int rt_bridge_open(const char* name, int flags, int mode, rt_event_t *event)
{
  int irq = hal_irq_disable();

  hal_bridge_t *bridge = hal_bridge_get();

  rt_event_t *call_event = __rt_wait_event_prepare(event);

  rt_bridge_req_t *req = &call_event->bridge_req;
  hal_bridge_open(&req->header, __rt_bridge_strlen(name), name, flags, mode);
  __rt_bridge_post_req(req, call_event);

  __rt_wait_event_check(event, call_event);

  hal_irq_restore(irq);

  return req->header.open.retval;
}



int rt_bridge_open_wait(rt_event_t *event)
{
  rt_bridge_req_t *req = &event->bridge_req;
  rt_event_wait(event);
  return req->header.open.retval;
}



int rt_bridge_close(int file, rt_event_t *event)
{
  int irq = hal_irq_disable();

  hal_bridge_t *bridge = hal_bridge_get();

  rt_event_t *call_event = __rt_wait_event_prepare(event);

  rt_bridge_req_t *req = &call_event->bridge_req;
  hal_bridge_close(&req->header, file);
  __rt_bridge_post_req(req, call_event);

  __rt_wait_event_check(event, call_event);

  hal_irq_restore(irq);

  return req->header.close.retval;
}



int rt_bridge_close_wait(rt_event_t *event)
{
  rt_bridge_req_t *req = &event->bridge_req;
  rt_event_wait(event);
  return req->header.close.retval;
}



int rt_bridge_read(int file, void* ptr, int len, rt_event_t *event)
{
  int irq = hal_irq_disable();

  hal_bridge_t *bridge = hal_bridge_get();

  rt_event_t *call_event = __rt_wait_event_prepare(event);

  rt_bridge_req_t *req = &call_event->bridge_req;
  hal_bridge_read(&req->header, file, ptr, len);
  __rt_bridge_post_req(req, call_event);

  __rt_wait_event_check(event, call_event);

  hal_irq_restore(irq);

  return req->header.read.retval;
}

int rt_bridge_read_wait(rt_event_t *event)
{
  rt_bridge_req_t *req = &event->bridge_req;
  rt_event_wait(event);
  return req->header.read.retval;
}



int rt_bridge_write(int file, void* ptr, int len, rt_event_t *event)
{
  int irq = hal_irq_disable();

  hal_bridge_t *bridge = hal_bridge_get();

  rt_event_t *call_event = __rt_wait_event_prepare(event);

  rt_bridge_req_t *req = &call_event->bridge_req;
  hal_bridge_write(&req->header, file, ptr, len);
  __rt_bridge_post_req(req, call_event);

  __rt_wait_event_check(event, call_event);

  hal_irq_restore(irq);

  return req->header.write.retval;
}



int rt_bridge_write_wait(rt_event_t *event)
{
  rt_bridge_req_t *req = &event->bridge_req;
  rt_event_wait(event);
  return req->header.write.retval;
}



// This is called everytime the bridge sends a notification so that we update
// the state of the request queue
void __rt_bridge_handle_notif()
{
  hal_bridge_t *bridge = hal_bridge_get();

  // Go through all the requests and handles the ones which are done
  rt_bridge_req_t *req = (rt_bridge_req_t *)bridge->first_req;

  while (req && req->header.done)
  {
    rt_bridge_req_t *next = (rt_bridge_req_t *)req->header.next;
    bridge->first_req = (uint32_t)next;

    rt_event_enqueue(req->event);

    req = next;
  }

  // Then check if we must update the bridge queue
  __rt_bridge_check_bridge_req();
}