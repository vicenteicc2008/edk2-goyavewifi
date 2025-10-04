/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 * Copyright (C) 2024-2025 Vicente Cortés <vicenteicc2008@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <Shim/list.h>
#include <Shim/Uboot.h>
#include <SC8830/reg.h>

#define PRE_RATE_CHANGE			BIT(0)
#define POST_RATE_CHANGE		BIT(1)
#define ABORT_RATE_CHANGE		BIT(2)

/*
 * flags used across common struct clk.  these flags should only affect the
 * top-level framework.  custom flags for dealing with hardware specifics
 * belong in struct clk_foo
 */
#define CLK_SET_RATE_GATE	BIT(0) /* must be gated across rate change */
#define CLK_SET_PARENT_GATE	BIT(1) /* must be gated across re-parent */
#define CLK_SET_RATE_PARENT	BIT(2) /* propagate rate change up one level */
#define CLK_IGNORE_UNUSED	BIT(3) /* do not gate even if unused */
#define CLK_IS_ROOT		BIT(4) /* root clk, has no parent */
#define CLK_IS_BASIC		BIT(5) /* Basic clk, can't do a to_clk_foo() */
#define CLK_GET_RATE_NOCACHE	BIT(6) /* do not use the cached clk rate */

typedef struct {
	CONST CHAR8		*Name;
	CONST CHAR8		**ParentNames;
	UINT8			NumParents;
} CLK_INIT_DATA;

struct Clk;
typedef struct {
	struct Clk *Clk;
	CONST CLK_INIT_DATA *Init;
} CLK_HW;

typedef struct {
	CLK_HW  Hw;
	unsigned long	FixedRate;
} CLK_FIXED_RATE;

typedef struct {
	CLK_HW Hw;
	VOID	*Reg;
	UINT8		BitIdx;
} CLK_GATE;

#define CLK_GATE_SET_TO_DISABLE		BIT(0)

typedef struct {
	UINTN	Val;
	UINTN	Div;
} CLK_DIV_TABLE;

typedef struct {
	CLK_HW	HW;
	VOID		*Reg;
	UINT8		Shift;
	UINT8		Width;
	UINT8		Flags;
	CONST CLK_DIV_TABLE	*Table;
	SPIN_LOCK   *Lock;
} CLK_DIVIDER;

#define CLK_DIVIDER_ONE_BASED		BIT(0)
#define CLK_DIVIDER_POWER_OF_TWO	BIT(1)
#define CLK_DIVIDER_ALLOW_ZERO		BIT(2)

typedef struct {
	CLK_HW	HW;
	VOID	*Reg;
	UINT32		*Table;
	UINT32		Mask;
	UINT8		Shift;
	UINT8		Flags;
	SPIN_LOCK   *Lock;
} CLK_MUX;

#define CLK_MUX_INDEX_ONE		BIT(0)
#define CLK_MUX_INDEX_BIT		BIT(1)

typedef struct {
	CLK_HW	HW;
	UINTN	Mult;
	UINTN	Div;
} CLK_FIXED_FACTOR;

#define HWSPINLOCK_ID_TOTAL_NUMS	(64)
#define HWLOCK_ADI	(0)
#define HWLOCK_GLB	(1)
#define HWLOCK_AGPIO	(2)
#define HWLOCK_AEIC	(3)
#define HWLOCK_ADC	(4)
#define HWLOCK_EFUSE	(8)

/* registers definitions for controller REGS_AP_AHB */
#define REG_AON_CLK_PUB_AHB_CFG         SCI_ADDR(REGS_AON_CLK_BASE, 0x0020)
#define REG_AON_APB_APB_EB0             SCI_ADDR(REGS_AON_APB_BASE, 0x0000)
#define REG_AON_APB_APB_EB1             SCI_ADDR(REGS_AON_APB_BASE, 0x0004)
#define REG_AP_AHB_AHB_RST              SCI_ADDR(REGS_AP_AHB_BASE, 0x0004)
#define REG_AP_AHB_CA7_RST_SET          SCI_ADDR(REGS_AP_AHB_BASE, 0x0008)
#define REG_AP_AHB_CA7_CKG_CFG          SCI_ADDR(REGS_AP_AHB_BASE, 0x000C)
#define REG_AP_AHB_MCU_PAUSE            SCI_ADDR(REGS_AP_AHB_BASE, 0x0010)
#define REG_AP_AHB_MISC_CKG_EN          SCI_ADDR(REGS_AP_AHB_BASE, 0x0014)
#define REG_AP_AHB_MISC_CFG             SCI_ADDR(REGS_AP_AHB_BASE, 0x0018)
#define REG_AP_AHB_AP_MTX_S3_PRIO0      SCI_ADDR(REGS_AP_AHB_BASE, 0x001C)
#define REG_AP_AHB_AP_MTX_S3_PRIO1      SCI_ADDR(REGS_AP_AHB_BASE, 0x0020)
#define REG_AP_AHB_AP_MTX_S3_PRIO2      SCI_ADDR(REGS_AP_AHB_BASE, 0x0024)
#define REG_AP_AHB_AP_MTX_S2_PRIO0      SCI_ADDR(REGS_AP_AHB_BASE, 0x0028)
#define REG_AP_AHB_AP_MTX_S1_PRIO0      SCI_ADDR(REGS_AP_AHB_BASE, 0x002C)
#define REG_AP_AHB_AP_MTX_S0_PRIO0      SCI_ADDR(REGS_AP_AHB_BASE, 0x0030)
#define REG_AP_AHB_AP_MTX_S0_PRIO1      SCI_ADDR(REGS_AP_AHB_BASE, 0x0034)
#define REG_AP_AHB_AP_MTX_S0_PRIO2      SCI_ADDR(REGS_AP_AHB_BASE, 0x0038)
#define REG_AP_AHB_AP_SYS_FORCE_SLEEP_CFG SCI_ADDR(REGS_AP_AHB_BASE, 0x003C)
#define REG_AP_AHB_AP_SYS_AUTO_SLEEP_CFG SCI_ADDR(REGS_AP_AHB_BASE, 0x0040)
#define REG_AP_AHB_AP_MAIN_MTX_HPROT_CFG SCI_ADDR(REGS_AP_AHB_BASE, 0x0044)
#define REG_AP_AHB_CA7_STANDBY_STATUS   SCI_ADDR(REGS_AP_AHB_BASE, 0x0048)
#define REG_AP_AHB_HOLDING_PEN          SCI_ADDR(REGS_AP_AHB_BASE, 0x004C)
#define REG_AP_AHB_JMP_ADDR_CA7_C0      SCI_ADDR(REGS_AP_AHB_BASE, 0x0050)
#define REG_AP_AHB_JMP_ADDR_CA7_C1      SCI_ADDR(REGS_AP_AHB_BASE, 0x0054)
#define REG_AP_AHB_JMP_ADDR_CA7_C2      SCI_ADDR(REGS_AP_AHB_BASE, 0x0058)
#define REG_AP_AHB_JMP_ADDR_CA7_C3      SCI_ADDR(REGS_AP_AHB_BASE, 0x005C)
#define REG_AP_AHB_CHIP_ID              SCI_ADDR(REGS_AP_AHB_BASE, 0x00FC)

// Raw helpers: use full mask (0xFFFFFFFFUL) to avoid -1UL signed surprises
#define SciGlbRawRead(Reg)           SciGlbRead((UINTN)(Reg), 0xFFFFFFFFUL)
#define SciGlbRawWrite(Reg, Val)     SciGlbWrite((UINTN)(Reg), (UINT32)(Val), 0xFFFFFFFFUL)

typedef struct {
    INTN Data;
} Mutex;

typedef struct {
	LIST_ENTRY   Node;
	CONST CHAR8      DevId;
	CONST CHAR8      ConId;
	struct Clk*      Clk;
} ClkLookup;

typedef struct {
	UINT32 Reg;
	UINT32 Mask;
} ClkReg;

typedef struct {
	INTN Id;
	CONST CHAR8 *Name;
	ClkReg Enb, Div, Sel;

	/* pll sources select */
	INTN NrSources;
	struct Clk *Sources[10];
} ClkRegs;

#define MAX_ERRNO			4095
#define IS_ERR_VALUE(x)		((x) >= (unsigned long)-MAX_ERRNO)

STATIC inline long IS_ERR_OR_NULL(CONST VOID *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

typedef struct {
	INTN NrSources;
	UINT32 Sources[];
} ClkSel;

typedef struct {
	int		(*Prepare)(CLK_HW *Hw);
	VOID		(*Unprepare)(CLK_HW *Hw);
	int		(*IsPrepared)(CLK_HW *Hw);
	int		(*Enable)(CLK_HW *Hw);
	VOID		(*Disable)(CLK_HW *Hw);
	int		(*IsEnabled)(CLK_HW *Hw);
	int (*SetRate) (struct Clk * c, unsigned long Rate);
	unsigned long (*GetRate) (struct Clk * c);
	unsigned long	(*RecalcRate)(CLK_HW *Hw, unsigned long ParentRate);
	long (*RoundRate) (CLK_HW *Hw, unsigned long, unsigned long *);
	int (*SetParent) (CLK_HW * Hw, UINT8 Index);
	UINT8 (*GetParent)(CLK_HW *Hw);
} CLK_OPS;

extern CONST CLK_OPS CLK_MUX_OPS;
extern CONST CLK_OPS CLK_DIVIDER_OPS;

struct Clk {
	CLK_HW *Hw;
	CONST CLK_OPS *Ops;
	struct Clk *Parent;
	UINT8 NumParents;
	INTN Usage;
	UINT32 Rate;
	UINT32 Flags;
	int (*Enable) (struct Clk *, int Enable, unsigned long *);

	CONST ClkRegs *Regs;
	CONST CHAR8   *Name;
};

typedef struct {
	struct Clk **Clks;
	UINTN CLK_NUM;
} CLK_ONECELL_DATA;

#define MAX_DIV					(1000)

typedef struct {
  UINT32 Cpu;
  UINT32 Old;
  UINT32 New;
  UINT32 Flags;
} CPUFREQ_FREQS;

#endif // CLOCK_H