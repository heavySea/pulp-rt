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
 * Authors: Germain Haugou, GreenWaves Technologies (germain.haugou@greenwaves-technologies.com)
 */

#include "rt/rt_api.h"
#include <stdint.h>

extern void __rt_spim_handle_tx_copy();
extern void __rt_spim_handle_rx_copy();
extern void __pi_spim_handle_eot();

RT_L2_DATA static pi_spim_t __rt_spim[ARCHI_UDMA_NB_SPIM];

typedef struct 
{
  pi_spim_t *spim;
  int max_baudrate;
  unsigned int cfg;
  char cs;
  char wordsize;
  char big_endian;
  signed char cs_gpio;
  char channel;
  char byte_align;
  unsigned char div;
  char polarity;
  char phase;
} pi_spim_cs_t;

typedef struct {
    unsigned int cmd[4];
} rt_spim_cmd_t;



static int __rt_spi_get_div(int spi_freq)
{
  int periph_freq = __rt_freq_periph_get();

  if (spi_freq >= periph_freq)
  {
    return 0;
  }
  else
  {
    // Round-up the divider to obtain an SPI frequency which is below the maximum
    int div = (__rt_freq_periph_get() + spi_freq - 1)/ spi_freq;

    // The SPIM always divide by 2 once we activate the divider, thus increase by 1
    // in case it is even to not go avove the max frequency.
    if (div & 1) div += 1;
    div >>= 1;

    return div;
  }
}



static inline int __rt_spim_get_byte_align(int wordsize, int big_endian)
{
  return wordsize == RT_SPIM_WORDSIZE_32 && big_endian;
}

int pi_spi_open(struct pi_device *device)
{
  int irq = rt_irq_disable();

  struct pi_spi_conf *conf = (struct pi_spi_conf *)device->config;

  int periph_id = ARCHI_UDMA_SPIM_ID(conf->itf);
  int channel_id = UDMA_EVENT_ID(periph_id);

  pi_spim_t *spim = &__rt_spim[conf->itf];

  pi_spim_cs_t *spim_cs = rt_alloc(RT_ALLOC_FC_DATA, sizeof(pi_spim_cs_t));
  if (spim_cs == NULL) goto error;

  device->data = (void *)spim_cs;

  spim_cs->channel = channel_id;
  spim_cs->spim = spim;
  spim_cs->wordsize = conf->wordsize;
  spim_cs->big_endian = conf->big_endian;
  spim_cs->polarity = conf->polarity;
  spim_cs->phase = conf->phase;
  spim_cs->max_baudrate = conf->max_baudrate;
  spim_cs->cs = conf->cs;
  spim_cs->byte_align = __rt_spim_get_byte_align(conf->wordsize, conf->big_endian);

  int div = __rt_spi_get_div(spim_cs->max_baudrate);
  spim_cs->div = div;

  spim_cs->cfg = SPI_CMD_CFG(div, conf->polarity, conf->phase);

  spim->open_count++;
  if (spim->open_count == 1)
  {
    plp_udma_cg_set(plp_udma_cg_get() | (1<<periph_id));
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_SPIM0_EOT + conf->itf);
    __rt_udma_extra_callback[ARCHI_SOC_EVENT_SPIM0_EOT - ARCHI_SOC_EVENT_UDMA_FIRST_EXTRA_EVT + conf->itf] = __pi_spim_handle_eot;
    __rt_udma_register_channel_callback(channel_id, __rt_spim_handle_rx_copy);
    __rt_udma_register_channel_callback(channel_id+1, __rt_spim_handle_tx_copy);
  }

  rt_irq_restore(irq);

  return 0;

error:
  rt_irq_restore(irq);
  return -1;
}

void pi_spi_control(struct pi_device *device, pi_spi_control_e cmd, uint32_t arg)
{
  int irq = rt_irq_disable();
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;

  int polarity = (cmd >> __RT_SPIM_CTRL_CPOL_BIT) & 3;
  int phase = (cmd >> __RT_SPIM_CTRL_CPHA_BIT) & 3;
  int set_freq = (cmd >> __RT_SPIM_CTRL_SET_MAX_BAUDRATE_BIT) & 1;
  int wordsize = (cmd >> __RT_SPIM_CTRL_WORDSIZE_BIT) & 3;
  int big_endian = (cmd >> __RT_SPIM_CTRL_ENDIANNESS_BIT) & 3;

  if (set_freq)
  {
    spim_cs->max_baudrate = arg;
    spim_cs->div = __rt_spi_get_div(arg);
  }

  if (polarity) spim_cs->polarity = polarity >> 1;
  if (phase) spim_cs->phase = phase >> 1;
  if (wordsize) spim_cs->wordsize = wordsize >> 1;
  if (big_endian) spim_cs->big_endian = big_endian >> 1;


  spim_cs->cfg = SPI_CMD_CFG(spim_cs->div, spim_cs->polarity, spim_cs->phase);
  spim_cs->byte_align = __rt_spim_get_byte_align(spim_cs->wordsize, spim_cs->big_endian);

  rt_irq_restore(irq);
}

void pi_spi_close(struct pi_device *device)
{
  int irq = rt_irq_disable();
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;

  int channel = spim_cs->channel;
  int periph_id = UDMA_PERIPH_ID(channel);

  spim->open_count--;

  if (spim->open_count == 0)
  {
    // Deactivate event routing
    soc_eu_fcEventMask_clearEvent(channel);

    // Reactivate clock-gating
    plp_udma_cg_set(plp_udma_cg_get() & ~(1<<periph_id));
  }

  rt_irq_restore(irq);
}

void __pi_spi_send_async(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e cs_mode, pi_task_t *task)
{
  int irq = rt_irq_disable();

  __rt_task_init(task);

  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;

  if (spim->pending_copy)
  {
    task->implem.data[0] = 0;
    task->implem.data[1] = (int)device;
    task->implem.data[2] = (int)data;
    task->implem.data[3] = len;
    task->implem.data[4] = qspi;
    task->implem.data[5] = cs_mode;

    if (spim->waiting_first)
      spim->waiting_last->implem.next = task;
    else
      spim->waiting_first = task;

    spim->waiting_last = task;
    task->implem.next = NULL;

    goto end;
  }

  spim->pending_copy = task;

  // First enqueue the header with SPI config, cs, and send command.
  // The rest will be sent by the assembly code.
  // First the user data and finally an epilogue with the EOT command.

  spim->udma_cmd[0] = spim_cs->cfg;
  spim->udma_cmd[1] = SPI_CMD_SOT(spim_cs->cs);
  spim->udma_cmd[2] = SPI_CMD_TX_DATA(len, qspi, spim_cs->byte_align);

  int size = (len + 7) >> 3;
  unsigned int base = hal_udma_channel_base(spim_cs->channel + 1);


  if (cs_mode == PI_SPI_CS_AUTO)
  {
    // CS auto mode. We handle the termination with an EOT so we have to enqueue
    // 3 transfers.
    // Enqueue fist SOT and user buffer.
    plp_udma_enqueue(base, (unsigned int)spim->udma_cmd, 3*4, UDMA_CHANNEL_CFG_EN);
    plp_udma_enqueue(base, (unsigned int)data, size, UDMA_CHANNEL_CFG_EN);

    // Then wait until first one is finished
    while(!plp_udma_canEnqueue(base));

    // And finally enqueue the EOT.
    // The user notification will be sent as soon as the last transfer
    // is done and next pending transfer will be enqueued
    spim->udma_cmd[0] = SPI_CMD_EOT(1);
    plp_udma_enqueue(base, (unsigned int)spim->udma_cmd, 1*4, UDMA_CHANNEL_CFG_EN);
  }
  else
  {
    // CS keep mode.
    // We cannot use EOT due to HW limitations (generating EOT is always releasing CS)
    // so we have to use TX event instead.
    // TX event is current inactive, enqueue first transfer first EOT.
    plp_udma_enqueue(base, (unsigned int)spim->udma_cmd, 3*4, UDMA_CHANNEL_CFG_EN);
    // Then wait until it is finished (should be very quick).
    while(plp_udma_busy(base));
    // Then activateTX event and enqueue user buffer.
    // User notification and next pending transfer will be handled in the handler.
    soc_eu_fcEventMask_setEvent(spim_cs->channel + 1);
    plp_udma_enqueue(base, (unsigned int)data, size, UDMA_CHANNEL_CFG_EN);
  }

end:
  rt_irq_restore(irq);
}



void __pi_spi_send(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e cs_mode)
{
  pi_task_t task;
  __pi_spi_send_async(device, data, len, qspi, cs_mode, pi_task(&task));
  pi_wait_on_task(&task);
}



void __pi_spi_receive_async(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e cs_mode, pi_task_t *task)
{
  rt_trace(RT_TRACE_SPIM, "[SPIM] Receive bitstream (handle: %p, buffer: %p, len: 0x%x, qspi: %d, keep_cs: %d, event: %p)\n", handle, data, len, qspi, cs_mode, event);

  int irq = rt_irq_disable();

  __rt_task_init(task);

  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;

  if (spim->pending_copy)
  {
    task->implem.data[0] = 1;
    task->implem.data[1] = (int)device;
    task->implem.data[2] = (int)data;
    task->implem.data[3] = len;
    task->implem.data[4] = qspi;
    task->implem.data[5] = cs_mode;

    if (spim->waiting_first)
      spim->waiting_last->implem.next = task;
    else
      spim->waiting_first = task;

    spim->waiting_last = task;
    task->implem.next = NULL;

    goto end;
  }

  spim->pending_copy = task;

  int cmd_size = 3*4;
  spim->udma_cmd[0] = spim_cs->cfg;
  spim->udma_cmd[1] = SPI_CMD_SOT(spim_cs->cs);
  spim->udma_cmd[2] = SPI_CMD_RX_DATA(len, qspi, spim_cs->byte_align);
  if (cs_mode == PI_SPI_CS_AUTO)
  {
    spim->udma_cmd[3] = SPI_CMD_EOT(1);
    cmd_size += 4;
  }

  int size = (len + 7) >> 3;
  unsigned int rx_base = hal_udma_channel_base(spim_cs->channel);

  if (cs_mode != PI_SPI_CS_AUTO)
  {
    soc_eu_fcEventMask_setEvent(spim_cs->channel);
  }

  unsigned int tx_base = hal_udma_channel_base(spim_cs->channel + 1);
  plp_udma_enqueue(rx_base, (unsigned int)data, size, UDMA_CHANNEL_CFG_EN | (2<<1));
  plp_udma_enqueue(tx_base, (unsigned int)spim->udma_cmd, cmd_size, UDMA_CHANNEL_CFG_EN);

end:
  rt_irq_restore(irq);
}

void __pi_spi_receive(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e cs_mode)
{
  pi_task_t task;
  __pi_spi_receive_async(device, data, len, qspi, cs_mode, pi_task(&task));
  pi_wait_on_task(&task);
}


void pi_spi_transfer_async(struct pi_device *device, void *tx_data, void *rx_data, size_t len, pi_spi_cs_e cs_mode, pi_task_t *task)
{
  rt_trace(RT_TRACE_SPIM, "[SPIM] Transfering bitstream (handle: %p, tx_buffer: %p, rx_buffer: %p, len: 0x%x, keep_cs: %d, event: %p)\n", handle, tx_data, rx_data, len, cs_mode, event);

  int irq = rt_irq_disable();

  __rt_task_init(task);

  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;

  if (spim->pending_copy)
  {
    task->implem.data[0] = 2;
    task->implem.data[1] = (int)device;
    task->implem.data[2] = (int)tx_data;
    task->implem.data[3] = (int)rx_data;
    task->implem.data[4] = len;
    task->implem.data[5] = cs_mode;

    if (spim->waiting_first)
      spim->waiting_last->implem.next = task;
    else
      spim->waiting_first = task;

    spim->waiting_last = task;
    task->implem.next = NULL;

    goto end;
  }

  spim->pending_copy = task;

  // First enqueue the header with SPI config, cs, and send command.
  // The rest will be sent by the assembly code.
  // First the user data and finally an epilogue with the EOT command.
  spim->udma_cmd[0] = spim_cs->cfg;
  spim->udma_cmd[1] = SPI_CMD_SOT(spim_cs->cs);
  spim->udma_cmd[2] = SPI_CMD_FUL(len, spim_cs->byte_align);

  int channel_id = spim_cs->channel;

  int size = (len + 7) >> 3;
  unsigned int rx_base = hal_udma_channel_base(channel_id);
  unsigned int tx_base = hal_udma_channel_base(channel_id+1);

  if (cs_mode == PI_SPI_CS_AUTO)
  {
    plp_udma_enqueue(tx_base, (unsigned int)spim->udma_cmd, 3*4, UDMA_CHANNEL_CFG_EN);
    plp_udma_enqueue(rx_base, (unsigned int)rx_data, size, UDMA_CHANNEL_CFG_EN | (2<<1));
    plp_udma_enqueue(tx_base, (unsigned int)tx_data, size, UDMA_CHANNEL_CFG_EN);

    while(!plp_udma_canEnqueue(tx_base));

    spim->udma_cmd[0] = SPI_CMD_EOT(1);
    plp_udma_enqueue(tx_base, (unsigned int)spim->udma_cmd, 1*4, UDMA_CHANNEL_CFG_EN);
  }
  else
  {
    plp_udma_enqueue(tx_base, (unsigned int)spim->udma_cmd, 3*4, UDMA_CHANNEL_CFG_EN);
    soc_eu_fcEventMask_setEvent(channel_id);
    plp_udma_enqueue(rx_base, (unsigned int)rx_data, size, UDMA_CHANNEL_CFG_EN | (2<<1));
    plp_udma_enqueue(tx_base, (unsigned int)tx_data, size, UDMA_CHANNEL_CFG_EN);
  }

end:
  rt_irq_restore(irq);
}

void pi_spi_transfer(struct pi_device *device, void *tx_data, void *rx_data, size_t len, pi_spi_cs_e cs_mode)
{
  pi_task_t task;
  pi_spi_transfer_async(device, tx_data, rx_data, len, cs_mode, pi_task(&task));
  pi_wait_on_task(&task);
}

void __pi_handle_waiting_copy(pi_task_t *task)
{
  if (task->implem.data[0] == 0)
    __pi_spi_send_async((struct pi_device *)task->implem.data[1], (void *)task->implem.data[2], task->implem.data[3], task->implem.data[4], task->implem.data[5], task);
  else if (task->implem.data[0] == 1)
    __pi_spi_receive_async((struct pi_device *)task->implem.data[1], (void *)task->implem.data[2], task->implem.data[3], task->implem.data[4], task->implem.data[5], task);
  else
    pi_spi_transfer_async((struct pi_device *)task->implem.data[1], (void *)task->implem.data[2], (void *)task->implem.data[3], task->implem.data[4], task->implem.data[5], task);
}

void pi_spi_conf_init(struct pi_spi_conf *conf)
{
  conf->wordsize = RT_SPIM_WORDSIZE_8;
  conf->big_endian = 0;
  conf->max_baudrate = 10000000;
  conf->cs_gpio = -1;
  conf->cs = -1;
  conf->itf = 0;
  conf->polarity = 0;
  conf->phase = 0;
}

static RT_FC_BOOT_CODE void __attribute__((constructor)) __rt_spim_init()
{
  for (int i=0; i<ARCHI_UDMA_NB_SPIM; i++)
  {
    __rt_spim[i].open_count = 0;
    __rt_spim[i].pending_copy = NULL;
    __rt_spim[i].waiting_first = NULL;
    __rt_udma_channel_reg_data(UDMA_EVENT_ID(ARCHI_UDMA_SPIM_ID(0) + i), &__rt_spim[i]);
    __rt_udma_channel_reg_data(UDMA_EVENT_ID(ARCHI_UDMA_SPIM_ID(0) + i)+1, &__rt_spim[i]);
  }
}
