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

#ifndef __HAL_MAESTRO_PMU_V1_H__
#define __HAL_MAESTRO_PMU_V1_H__

#include "hal/pulp.h"
#include "archi/maestro/maestro_v1.h"

/* APB Interface */
#define PCTRL     0x0
#define PRDATA      0x4
#define DLC_SR      0x8
#define DLC_IMR     0xC
#define DLC_IFR     0x10
#define DLC_IOIFR   0x14
#define DLC_IDIFR   0x18
#define DLC_IMCIFR    0x1C

#define PMU_Read(reg)   __builtin_pulp_OffsetedRead((void *) PMU_DLC_BASE_ADDRESS, (reg))
#define PMU_Write(reg, value) IP_WRITE(PMU_DLC_BASE_ADDRESS, (reg), (value))

#define PMU_ICU_SUBREG_BITS 5

/* Offsets for the ICU internal registers */
#define PMU_ICU_CR    0x0
#define PMU_ICU_MR    0x1
#define PMU_ICU_ISMR    0x2
#define PMU_ICU_DMR(n)    (0x2+(n))
#define PMU_ICU_REG_ADDRESS(i, r) (((i) << PMU_ICU_SUBREG_BITS) | ((r) & ((1 << PMU_ICU_SUBREG_BITS) - 1)))

/* Maestro internal events */
#define MAESTRO_EVENT_ICU_OK    (1<<0)
#define MAESTRO_EVENT_ICU_DELAYED (1<<1)
#define MAESTRO_EVENT_MODE_CHANGED  (1<<2)
#define MAESTRO_EVENT_PICL_OK   (1<<3)
#define MAESTRO_EVENT_SCU_OK    (1<<4)

// #define	SOC_PERIPHERALS_BASE_ADDR	(0x1A100000)
// #define	ARCHI_APB_SOC_CTRL_ADDR		(SOC_PERIPHERALS_BASE_ADDR+0x3000)

/* Base address for programming interface of PMU: 0x1A106000 */
// #define PMU_DLC_OFFSET				0x6000
#define PMU_DLC_OFFSET				ARCHI_PMU_OFFSET
#define PMU_DLC_BASE_ADDRESS			(ARCHI_SOC_PERIPHERALS_ADDR + PMU_DLC_OFFSET)

/* Cluster Reset and transaction isolation */
#define	PMU_CLUSTER_RESET_OFFSET		0x8
#define PMU_CLUSTER_RESET_REG			(ARCHI_APB_SOC_CTRL_ADDR + PMU_CLUSTER_RESET_OFFSET)

#define	PMU_CLUSTER_TRANSAC_ISOLATE_OFFSET	0xC
#define PMU_CLUSTER_TRANSAC_ISOLATE_REG		(ARCHI_APB_SOC_CTRL_ADDR + PMU_CLUSTER_TRANSAC_ISOLATE_OFFSET)

/* Voltage definition, 0x1A103100 */
#define PMU_DCDC_CONFIG_OFFSET			0x100
#define PMU_DCDC_CONFIG_REG			(ARCHI_APB_SOC_CTRL_ADDR + PMU_DCDC_CONFIG_OFFSET)

/* PMU bypass for cluster ON/OFF control */
#define	PMU_BYPASS_OFFSET			0x70
#define	PMU_BYPASS_REG				(ARCHI_APB_SOC_CTRL_ADDR + PMU_BYPASS_OFFSET)

/* Retention control, 0x1A103104. Aliased at 0x1A10007C */
#define PMU_RETENTION_CONFIG_OFFSET		0x104
#define PMU_RETENTION_CONFIG_ALIAS_OFFSET	0x7C
#define PMU_RETENTION_CONFIG_REG		(ARCHI_APB_SOC_CTRL_ADDR + PMU_RETENTION_CONFIG_OFFSET)
#define PMU_RETENTION_CONFIG_ALIAS_REG		(ARCHI_APB_SOC_CTRL_ADDR + PMU_RETENTION_CONFIG_ALIAS_OFFSET)

/* Retention forcing, 0x1A103108 */
#define PMU_RETENTION_FORCE_CONFIG_OFFSET	0x108
#define PMU_RETENTION_FORCE_CONFIG_REG		(ARCHI_APB_SOC_CTRL_ADDR + PMU_RETENTION_FORCE_CONFIG_OFFSET)

/* Events definition for PMU controller */
#define GAP8_PMU_EVENT_CLUSTER_ON_OFF 		31
#define GAP8_PMU_EVENT_MSP              	32
#define GAP8_PMU_EVENT_ICU_MODE_CHANGED 	33
#define GAP8_PMU_EVENT_ICU_OK           	34
#define GAP8_PMU_EVENT_ICU_DELAYED      	35
#define GAP8_PMU_EVENT_PICL_OK          	36
#define GAP8_PMU_EVENT_SCU_OK           	37



/* Fll models */

#define	GAP8_FLL_REF_CLOCK	32768
#define	GAP8_MAX_LV_FREQUENCY	150000000
#define	GAP8_MAX_NV_FREQUENCY	250000000


/* Voltage regulator and power states */

#define DCDC_OPER_POINTS	4

#define	DCDC_DEFAULT_NV		1200
#define	DCDC_DEFAULT_MV		1200
#define	DCDC_DEFAULT_LV		1000
#define	DCDC_DEFAULT_RET	800

#define	DCDC_RANGE		5
#define	DCDC_BASE_VALUE		550
#define DCDC_STEP		50

#define mVtoDCDCSetting(mV)	((unsigned int) (((mV) - DCDC_BASE_VALUE)/DCDC_STEP))
#define DCDCSettingtomV(Dc)	((unsigned int) ((Dc)*DCDC_STEP + DCDC_BASE_VALUE))

#define MAX_DCDC_VARIATION  	(int) (0.1*32767)

typedef enum {
	REGU_NV  = 0,
	REGU_LV  = 1,
	REGU_RET = 2,
	REGU_OFF = 3
} RegulatorStateT;

typedef enum {
	CLUSTER_OFF = 0,
	CLUSTER_ON  = 1
} ClusterStateT;

#define PMU_STATES			6
#define PMU_LAST_STATE			6
#define PMU_LN2_STATES			3
#define	REGU_STATE_SIZE			2
#define	REGU_STATE_OFFSET		1
#define	CLUSTER_STATE_SIZE		1
#define	CLUSTER_STATE_OFFSET		0

typedef enum {
	SOC_CLUSTER_HP = ((REGU_NV<<1)  | CLUSTER_ON),	/* 001 = 1 */
	SOC_CLUSTER_LP = ((REGU_LV<<1)  | CLUSTER_ON),	/* 011 = 3 */
	SOC_HP = 	 ((REGU_NV<<1)  | CLUSTER_OFF), /* 000 = 0 */
	SOC_LP = 	 ((REGU_LV<<1)  | CLUSTER_OFF),	/* 010 = 2 */
	RETENTIVE = 	 ((REGU_RET<<1) | CLUSTER_OFF), /* 100 = 4 */
	DEEP_SLEEP = 	 ((REGU_OFF<<1) | CLUSTER_OFF)  /* 110 = 6 */
} Gap8PMUSystemStateT;

#define REGULATOR_STATE(State)	((RegulatorStateT) (pulp_bitextractu((State), REGU_STATE_SIZE, REGU_STATE_OFFSET)))
#define CLUSTER_STATE(State)	((ClusterStateT) (pulp_bitextractu((State), CLUSTER_STATE_SIZE, CLUSTER_STATE_OFFSET)))

typedef enum {
        PMU_CHANGE_OK = 0,
	PMU_CHANGE_ERROR = 1
} Gap8PMUStatusT;

#define LN2_DCDC_OPER_POINTS	2
typedef enum {
        DCDC_Nominal   = 0,
        DCDC_Medium    = 1,
        DCDC_Low       = 2,
        DCDC_Retentive = 3,
} Gap8_DCDC_HW_OperatingPointT;


typedef enum {
	SCU_DEEP_SLEEP     = 0,
	SCU_RETENTIVE      = 1,
	SCU_SOC_HP         = 2,
	SCU_SOC_LP         = 3,
	SCU_SOC_CLUSTER_HP = 4,
	SCU_SOC_CLUSTER_LP = 5,
} Gap8_SCU_Seq;

/* PMU Events */
#define PMU_EVENT_CLUSTER_ON_OFF	(1<<0)
#define PMU_EVENT_MSP			(1<<1)
#define PMU_EVENT_ICU_MODE_CHANGED	(1<<2)
#define PMU_EVENT_ICU_OK		(1<<3)
#define PMU_EVENT_ICU_DELAYED		(1<<4)
#define PMU_EVENT_PICL_OK		(1<<5)
#define PMU_EVENT_SCU_OK		(1<<6)
#define PMU_EVENT_SCU_PENDING		(1<<7)

/* Deprecated events */
#define PMU_EVENT_CLUSTER_CLOCK_GATE   (PLP_SOC_EVENT_ICU_DELAYED)

/* Retention state */
#define L2_RETENTIVE_REGION 4
#define N_GPIO      32
#define LN2_N_GPIO    5

typedef enum {                  /* Enum encoding follows the definition of the boot_type field of PMU_RetentionStateT */
        COLD_BOOT=0,            /* SoC cold boot, from Flash usually */
        DEEP_SLEEP_BOOT=1,      /* Reboot from deep sleep state, state has been lost, somehow equivalent to COLD_BOOT */
        RETENTIVE_BOOT=2,       /* Reboot from Retentive state, state has been retained, should bypass flash reload */
} PMU_WakeupStateT;


typedef union {
  struct {
    unsigned int L2Retention:L2_RETENTIVE_REGION;   /* 1 enable bit per area */
    unsigned int FllSoCRetention:1;       /* 1: Soc Fll is state retentive */
    unsigned int FllClusterRetention:1;     /* 1: Cluster Fll is state retentive */
    unsigned int ExternalWakeUpSource:LN2_N_GPIO;   /* 00000: GPIO0, 00001: GPIO1, ..., 11111: GPIO31 */
    unsigned int ExternalWakeUpMode: 2;     /* 00: raise, 01: fall, 10: high, 11: low */
    unsigned int ExternalWakeupEnable:1;      /* 0: disable ext wake up, 1: enable ext wake up */
    unsigned int WakeupState:2;       /* 00: SOC_HP, 01: SOC_LP, 10: SOC_CLUSTER_HP, 11: SOC_CLUSTER_LP */
    unsigned int BootMode:1;        /* 0: boot from rom, 1: boot from L2 */
    unsigned int WakeupCause:1;       /* Wake up it status, read clears, 0: RTC, 1: External event */
    unsigned int BootType:2;        /* 00: cold boot, 01: deep sleep, 10: retentive deep sleep */
  } Fields;
  unsigned int Raw;
} PMU_RetentionStateT;


typedef union {
	struct {
		unsigned int L2_Area_Retentive:4;	/* L2 Area 0, 1, 2, 3 is forced to retentive state */
		unsigned int L2_Area_Down:4;		/* L2 Area 0, 1, 2, 3 is forced to shutdown */
		unsigned int Fll_Cluster_Retentive:1;	/* Cluster Fll is forced to retentive */
		unsigned int Fll_Cluster_Down:1;	/* Cluster Fll is forced to shutdown */
	} Fields;
	unsigned int Raw;
} ForcedRetentionStateT;


/* PMU bypass control for fast Cluster ON/OFF sequences */
typedef union {
  struct {
    unsigned int Bypass:1;      /* b0      1: Bypass maestro */
    unsigned int BypassConfig:1;    /* b1      0: Use default config, 1: Use user config, fields are bellow */
    unsigned int Pad0:1;      /* b2      */
    unsigned int ClusterState:1;    /* b3      0: Cluster Off, 1: Cluster On */
    unsigned int TRCMaxCurrent:3;   /* b4..6   When in BypassConfig and On state, max current allowed on the TRC */
    unsigned int TRCDelay:2;    /* b7..8   When in BypassConfig and On state number of 32K cycles after pwr ok to release isolation */
    unsigned int BypassClock:1;   /* b9    1: Bypass clock and reset control by Maestro */
    unsigned int ClusterClockGate:1;  /* b10     1: Clock gate the cluster, should always be used before shutdown or retention */
    unsigned int ClockDown:1;   /* b11     1: Set Cluster FLL to off state */
    unsigned int ClockRetentive:1;    /* b12     1: Set Cluster FLL to retentive state */
    unsigned int ClusterReset:1;    /* b13     1: If in Bypass Clock controls the cluster reset */
    unsigned int Pad1:2;      /* b14..15 Padding */
    unsigned int TRCPowerOk:1;    /* b16     Power ok coming from TRC */
    unsigned int PMUPowerDown:1;    /* b17     Power down as reported from maestro */
  } Fields; 
  unsigned int Raw;
} PMU_BypassT;

static inline void PMU_ResetCluster(unsigned int Value) {
	IP_WRITE(ARCHI_APB_SOC_CTRL_ADDR, PMU_CLUSTER_RESET_OFFSET, Value);
}

static inline void PMU_IsolateCluster(unsigned int Value) {
	IP_WRITE(ARCHI_APB_SOC_CTRL_ADDR, PMU_CLUSTER_TRANSAC_ISOLATE_OFFSET, Value);
}

static inline unsigned int GetPMUBypass() {
  return IP_READ(ARCHI_APB_SOC_CTRL_ADDR, PMU_BYPASS_OFFSET);
}

static inline void SetPMUBypass(unsigned int Value) {
	IP_WRITE(ARCHI_APB_SOC_CTRL_ADDR, PMU_BYPASS_OFFSET, Value);
}

static inline unsigned int GetRetentiveState() {
  return IP_READ(ARCHI_APB_SOC_CTRL_ADDR, PMU_RETENTION_CONFIG_OFFSET);
}

static inline void SetRetentiveState(unsigned Value) {
	IP_WRITE(ARCHI_APB_SOC_CTRL_ADDR, PMU_RETENTION_CONFIG_OFFSET, Value);
}

static inline unsigned int GetForcedRetentiveState() {
  return IP_READ(ARCHI_APB_SOC_CTRL_ADDR, PMU_RETENTION_FORCE_CONFIG_OFFSET);
}

static inline void SetForcedRetentiveState(unsigned Value) {
	IP_WRITE(ARCHI_APB_SOC_CTRL_ADDR, PMU_RETENTION_FORCE_CONFIG_OFFSET, Value);
}

static inline unsigned int GetDCDCSetting() {
  return IP_READ(ARCHI_APB_SOC_CTRL_ADDR, PMU_DCDC_CONFIG_OFFSET);
}

static inline void SetDCDCSetting(unsigned int Value) {
	IP_WRITE(ARCHI_APB_SOC_CTRL_ADDR, PMU_DCDC_CONFIG_OFFSET, Value);
}

#endif