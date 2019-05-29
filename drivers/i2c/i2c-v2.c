/*
 * Copyright (C) 2018 ETH Zurich, University of Bologna and GreenWaves Technologies
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
 * Authors: Yao ZHANG, GreenWaves Technologies (yao.zhang@greenwaves-technologies.com)
 */

#include "pmsis.h"


static RT_L2_DATA pi_i2c_t __rt_i2c[ARCHI_UDMA_NB_I2C];


void __rt_i2c_handle_tx_copy();
void __rt_i2c_handle_rx_copy();
extern void __rt_i2c_step1();
extern void __rt_i2c_step2();
extern void __rt_i2c_step3();
extern void udma_event_handler_end();


static int __rt_i2c_get_div(int i2c_freq)
{
    // Round-up the divider to obtain an SPI frequency which is below the maximum
  int div = (__rt_freq_periph_get() + i2c_freq - 1)/i2c_freq;
    // The SPIM always divide by 2 once we activate the divider, thus increase by 1
    // in case it is even to not go avove the max frequency.
  if (div & 1) div += 1;
  div >>= 1;
  return div;
}



void pi_i2c_control(struct pi_device *device, pi_i2c_control_e cmd, uint32_t arg)
{
  pi_i2c_t *handle = (pi_i2c_t *)device->data;
  int irq = rt_irq_disable();

  int set_freq = (cmd >> __PI_I2C_CTRL_SET_MAX_BAUDRATE_BIT) & 1;

  if (set_freq)
  {
    handle->max_baudrate = arg;
    handle->div = __rt_i2c_get_div(handle->max_baudrate);
  }

  rt_irq_restore(irq);
}



void __rt_i2c_write_common(struct pi_device *device, uint8_t *data, int length, int xfer_pending, pi_task_t *task, int start)
{
  int irq = rt_irq_disable();

  __rt_task_init(task);

  pi_i2c_t *i2c = (pi_i2c_t *)device->data;

  if (i2c->pending_copy)
  {
    task->implem.data[0] = (unsigned int)data;
    task->implem.data[1] = (unsigned int)length;
    task->implem.data[2] = (unsigned int)xfer_pending;
    task->implem.data[3] = (unsigned int)start;

    if (i2c->waiting_first)
      i2c->waiting_last->implem.next = task;
    else
      i2c->waiting_first = task;

    i2c->waiting_last = task;
    task->implem.next = NULL;

    goto end;
  }

  i2c->pending_copy = task;

  int seq_index = 0;
  i2c->pending_step = (uint32_t)__rt_i2c_step1;

  // Compute the next step. If we have to generate a stop bit we must go through an
  // additional copy.
  int next_step;
  if (xfer_pending)
    i2c->pending_next_step = (uint32_t)__rt_i2c_step3;
  else
    i2c->pending_next_step = (uint32_t)__rt_i2c_step2;

  i2c->udma_cmd[seq_index++] = I2C_CMD_CFG;
  i2c->udma_cmd[seq_index++] = (i2c->div >> 8) & 0xFF;
  i2c->udma_cmd[seq_index++] = (i2c->div & 0xFF);
  if (start)
  {
    i2c->udma_cmd[seq_index++] = I2C_CMD_START;
    i2c->udma_cmd[seq_index++] = I2C_CMD_WR;
    i2c->udma_cmd[seq_index++] = i2c->cs;
  }

  if (length > 1){
    i2c->udma_cmd[seq_index++] = I2C_CMD_RPT;
    i2c->udma_cmd[seq_index++] = length;
  }
  i2c->udma_cmd[seq_index++] = I2C_CMD_WR;

  unsigned int base = hal_udma_channel_base(i2c->channel + 1);

  i2c->pending_base = base;
  i2c->pending_data = (unsigned int)data;
  i2c->pending_length = length;

  plp_udma_enqueue(base, (unsigned int)i2c->udma_cmd, seq_index, UDMA_CHANNEL_CFG_EN);

end:
  rt_irq_restore(irq);
}

void pi_i2c_write_async(struct pi_device *device, uint8_t *data, int length, int xfer_pending, pi_task_t *task)
{
  __rt_i2c_write_common(device, data, length, xfer_pending, task, 1);
}

void pi_i2c_write(struct pi_device *device, uint8_t *data, int length, int xfer_pending)
{
  struct pi_task task;
  pi_i2c_write_async(device, data, length, xfer_pending, pi_task(&task));
  pi_wait_on_task(&task);
}

void pi_i2c_write_append_async(struct pi_device *device, uint8_t *data, int length, int xfer_pending, pi_task_t *task)
{
  __rt_i2c_write_common(device, data, length, xfer_pending, task, 0);
}

void pi_i2c_write_append(struct pi_device *device, uint8_t *data, int length, int xfer_pending)
{
  struct pi_task task;
  pi_i2c_write_append_async(device, data, length, xfer_pending, pi_task(&task));
  pi_wait_on_task(&task);
}

void pi_i2c_read_async(struct pi_device *device, uint8_t *rx_buff, int length, int xfer_pending, pi_task_t *task)
{
  int irq = rt_irq_disable();

  __rt_task_init(task);

  pi_i2c_t *i2c = (pi_i2c_t *)device->data;

  i2c->pending_copy = task;

  int seq_index = 0;
  i2c->pending_step = (uint32_t)udma_event_handler_end;

  i2c->udma_cmd[seq_index++] = I2C_CMD_CFG;
  i2c->udma_cmd[seq_index++] = (i2c->div >> 8) & 0xFF;
  i2c->udma_cmd[seq_index++] = (i2c->div & 0xFF);
  i2c->udma_cmd[seq_index++] = I2C_CMD_START;
  i2c->udma_cmd[seq_index++] = I2C_CMD_WR;
  i2c->udma_cmd[seq_index++] = i2c->cs | 0x1;

  if (length > 1){
    i2c->udma_cmd[seq_index++] = I2C_CMD_RPT;
    i2c->udma_cmd[seq_index++] = length - 1;
    i2c->udma_cmd[seq_index++] = I2C_CMD_RD_ACK;
  }
  i2c->udma_cmd[seq_index++] = I2C_CMD_RD_NACK;

  if (!xfer_pending)
    i2c->udma_cmd[seq_index++] = I2C_CMD_STOP;
    
  unsigned int base = hal_udma_channel_base(i2c->channel);

  plp_udma_enqueue(base + UDMA_CHANNEL_RX_OFFSET, (unsigned int)rx_buff, length, UDMA_CHANNEL_CFG_EN);
  plp_udma_enqueue(base + UDMA_CHANNEL_TX_OFFSET, (unsigned int)i2c->udma_cmd, seq_index, UDMA_CHANNEL_CFG_EN);

  rt_irq_restore(irq);
}

void pi_i2c_read(struct pi_device *device, uint8_t *data, int length, int xfer_pending)
{
  struct pi_task task;
  pi_i2c_read_async(device, data, length, xfer_pending, pi_task(&task));
  pi_wait_on_task(&task);
}

int pi_i2c_open(struct pi_device *device)
{
  int irq = rt_irq_disable();

  struct pi_i2c_conf *conf = (struct pi_i2c_conf *)device->config;

  int periph_id = ARCHI_UDMA_I2C_ID(conf->itf);
  int channel_id = UDMA_EVENT_ID(periph_id);

  pi_i2c_t *i2c = &__rt_i2c[conf->itf];

  device->data = (void *)i2c;
  i2c->open_count++;

  if (i2c->open_count == 1)
  {
    plp_udma_cg_set(plp_udma_cg_get() | (1<<periph_id));
    soc_eu_fcEventMask_setEvent(channel_id);
    soc_eu_fcEventMask_setEvent(channel_id + 1);

    __rt_udma_register_channel_callback(channel_id, __rt_i2c_handle_rx_copy);
    __rt_udma_register_channel_callback(channel_id+1, __rt_i2c_handle_tx_copy);

    i2c->pending_copy = NULL;
    i2c->waiting_first = NULL;
  }

  i2c->channel = channel_id;
  i2c->cs = conf->cs;
  i2c->max_baudrate = conf->max_baudrate;
  i2c->div = __rt_i2c_get_div(i2c->max_baudrate);

  rt_irq_restore(irq);

  return 0;
}



void pi_i2c_close (struct pi_device *device)
{
  pi_i2c_t *i2c = (pi_i2c_t *)device->data;

  int channel = i2c->channel;
  int periph_id = UDMA_PERIPH_ID(channel);

  i2c->open_count--;

  if (i2c->open_count == 0)
  {
    // Deactivate event routing
    soc_eu_fcEventMask_clearEvent(channel);

    // Reactivate clock-gating
    plp_udma_cg_set(plp_udma_cg_get() & ~(1<<periph_id));
  }
}



void pi_i2c_conf_init(pi_i2c_conf_t *conf)
{
  conf->cs = 0;
  conf->max_baudrate = 200000;
  conf->itf = 0;
}

static void __attribute__((constructor)) __rt_i2c_init()
{
  for (int i=0; i<ARCHI_UDMA_NB_I2C; i++)
  {
    __rt_i2c[i].open_count = 0;
    __rt_i2c[i].udma_stop_cmd = I2C_CMD_STOP;
    __rt_udma_channel_reg_data(UDMA_EVENT_ID(ARCHI_UDMA_I2C_ID(0) + i), &__rt_i2c[i]);
    __rt_udma_channel_reg_data(UDMA_EVENT_ID(ARCHI_UDMA_I2C_ID(0) + i)+1, &__rt_i2c[i]);
  }
}

