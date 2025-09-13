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

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/SynchronizationLib.h>

#include <SC8830/reg.h>
#include <Shim/Kernel.h>

#include <Protocol/SprdClock.h>
#include "clock.h"

#define __ffs(x)  LowBitSet32(x)

#if 1
const UINT32 __clkinit0, __clkinit_begin = 0xeeeebbbb;
const UINT32 __clkinit2, __clkinit_end   = 0xddddeeee;
#else
const UINT32 __clkinit0, __clkinit_begin = &CLK_LK_clk_mpll;
const UINT32 __clkinit2, __clkinit_end   = &CLK_LK_clk_mpll;
#endif

STATIC INTN clocks_lock = 0;
STATIC LIST_HEAD(Clocks);
STATIC Mutex ClocksMutex;

typedef struct {
	VOID *Reg;
	UINT32 Msk;
} CFG_REG;

typedef struct {
	CLK_HW Hw;
	struct Clk *Clk;
	CFG_REG Enb;
	UINT8 Flags;
	union {
		UINTN FixedRate;
		UINT32 C_Mul;
		CFG_REG Mul, Pre;
		CLK_HW *MUX_HW;
	} M;
	union {
		UINT32 C_Div;
		CFG_REG Div, Pre;
		CLK_HW *DIV_HW;
	} D;
} CLK_SPRD;

STATIC
VOID
ArchDefaultLock (
  IN  UINTN   LockAddr,
  OUT EFI_TPL *OldTpl
  )
{
  // sube el TPL = equivalente a guardar/disable IRQs
  *OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  // busy wait hasta tomar el lock
  while (MmioRead32 (LockAddr) != 0) {
    CpuPause (); // opcional
  }

  // escribe 1 para marcar lock tomado
  MmioWrite32 (LockAddr, 1);
}

STATIC
VOID
ArchDefaultUnlock (
  IN  UINTN   LockAddr,
  IN  EFI_TPL OldTpl
  )
{
  // libera lock
  MmioWrite32 (LockAddr, 0);

  // restaura interrupciones
  gBS->RestoreTPL (OldTpl);
}

#define TO_CLK_SPRD(_Hw) BASE_CR(_Hw, CLK_SPRD, Hw)

CONST CHAR8 *__ClkGetName(IN struct Clk *Clk)
{
	return !Clk ? NULL : Clk->Name;
}

STATIC VOID __GlbReg_SetClr(CLK_HW *Hw, VOID *Reg, UINT32 Msk,
				   INTN IsSet)
{
	EFI_TPL Flags;

	if (Reg == NULL) {
		return;
	}

	DEBUG((EFI_D_INFO, "%s %s %p[%x]\n", __ClkGetName(Hw->Clk),
		  (IsSet) ? "SET" : "CLR", Reg, (UINT32) Msk));

	ArchDefaultLock(HWLOCK_GLB, &Flags);

	if (IsSet)
		MmioWrite32(Msk, (UINT32) (Reg) + 0x1000);
	else
		MmioWrite32(Msk, (UINT32) (Reg) + 0x2000);

	ArchDefaultUnlock(HWLOCK_GLB, Flags);
}

#define __GlbReg_Set(Hw, Reg, Msk)	__GlbReg_SetClr(Hw, Reg, Msk, 1)
#define __GlbReg_Clr(Hw, Reg, Msk)	__GlbReg_SetClr(Hw, Reg, Msk, 0)

STATIC INTN SprdClkPrepare(IN CLK_HW *Hw)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	INTN Set = ! !(C->Flags & CLK_GATE_SET_TO_DISABLE);

	__GlbReg_SetClr(Hw, C->D.Pre.Reg, (UINT32) C->D.Pre.Msk, Set ^ 1);
	return 0;
}

STATIC VOID SprdClkUnprepare(CLK_HW *Hw)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	INTN Set = ! !(C->Flags & CLK_GATE_SET_TO_DISABLE);
	__GlbReg_SetClr(Hw, C->D.Pre.Reg, (UINT32) C->D.Pre.Msk, Set ^ 0);
}

STATIC INTN SprdClkIsPrepared(IN CLK_HW *Hw)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	INTN Ret, Set = ! !(C->Flags & CLK_GATE_SET_TO_DISABLE);

	if (!C->D.Pre.Reg)
		return 0;

	/* if a set bit prepare this gate, flip it before masking */
	Ret = ! !(MmioRead32((UINTN)C->D.Pre.Reg) & BIT(C->D.Pre.Msk));
	return Set ^ Ret;
}

STATIC INTN SprdClkEnable(IN CLK_HW *Hw)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	__GlbReg_Set(Hw, C->Enb.Reg, (UINT32) C->Enb.Msk);
	return 0;
}

STATIC VOID SprdClkDisable(IN CLK_HW *Hw)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	__GlbReg_Clr(Hw, C->Enb.Reg, (UINT32) C->Enb.Msk);
}

STATIC INTN SprdClkIsEnable(IN CLK_HW *Hw)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	INTN Ret = ! !(MmioRead32((UINTN)C->Enb.Reg) & BIT(C->Enb.Msk));
	return Ret;
}

STATIC unsigned long SprdClkFixedPllRecalcRate(IN CLK_HW *Hw,
						    unsigned long ParentRate)
{
	return TO_CLK_SPRD(Hw)->M.FixedRate;
}

#define BITS_MPLL_REFIN(_X_)                              ( (_X_) << 24 & (BIT(24)|BIT(25)) )

/* bits definitions for register REG PLL CFG1 */
#define BITS_PLL_KINT(_X_)                               ( (_X_) << 12 & (BIT(12)|BIT(13)|BIT(14)|BIT(15)|BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|BIT(23)|BIT(24)|BIT(25)|BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30)|BIT(31)) )
#define BIT_PLL_DIV_S                                    ( BIT(10) )
#define BITS_PLL_RSV(_X_)                                ( (_X_) << 8 & (BIT(8)|BIT(9)) )
#define BIT_PLL_MOD_EN                                   ( BIT(7) )
#define BIT_PLL_SDM_EN                                   ( BIT(6) )
#define BITS_PLL_NINT(_X_)                               ( (_X_) & (BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)) )

#define SHFT_PLL_KINT                                     ( 12 )
#define SHFT_PLL_NINT                                     ( 0 )

STATIC UINTN __PllGetRefinRate(VOID *Reg)
{
	CONST unsigned long Refin[4] = { 2000000, 4000000, 13000000, 26000000 };
	UINT32 I, Msk = BITS_MPLL_REFIN(-1);
	I = (MmioRead32((UINTN)Reg) & Msk) >> __ffs(Msk);
	return Refin[I];
}

STATIC UINT32 SprdClkAdjustablePllRecalcRate(IN CLK_HW *Hw,
							 UINT32
							 ParentRate)
{
	CLK_SPRD *Pll = TO_CLK_SPRD(Hw);
	UINT64 Rate;

	UINT32 K = 0;
	UINT32 Mn;
	UINT32 Cfg1;
	Cfg1 = MmioRead32((UINTN)Pll->M.Mul.Reg);
	Mn = (Cfg1 & BITS_PLL_NINT(~0)) >> SHFT_PLL_NINT;

	/* FIXME: Kint only valid while sdm_en = 1 */
	if ((Cfg1 & BIT_PLL_SDM_EN))
		K = (Cfg1 & BITS_PLL_KINT(~0)) >> SHFT_PLL_KINT;

	Rate = (UINT64)26 * (UINT64)(Mn) * 1000000 + (UINT64)DIV_ROUND_CLOSEST(26 * (UINT64)K * 100, 1048576) * 10000;
	DEBUG((EFI_D_INFO, "Rate %u, k %u, mn %u\n", Rate, K, Mn));
	return Rate;
}

STATIC INTN SprdClkAdjustablePllRoundRate(IN CLK_HW *Hw,
					       UINT32 Rate,
					       UINT32 *Prate)
{
	//CLK_SPRD *Pll = TO_CLK_SPRD(Hw);
	DEBUG((EFI_D_INFO, "Rate %lu, %lu\n", Rate, *Prate));
	return Rate;
}

STATIC VOID __PllRegWrite(IN VOID *Reg, IN UINT32 Val, IN UINT32 Msk)
{
	UINTN Addr = (UINTN)Reg;
	UINT32 Tmp;

	Tmp = MmioRead32(Addr);
	Tmp = (Tmp & ~Msk) | Val;
	MmioWrite32(Addr, Tmp);
}

STATIC INT32 __PllEnableTime(IN CLK_HW *Hw, UINT32 OldRate)
{
	/* FIXME: for mpll, each step (100MHz) takes 50us */
	UINT32 Rate = SprdClkAdjustablePllRecalcRate(Hw, 0) / 1000000;
	INTN Dly = ABS(Rate - OldRate) * 50 / 100;
	MicroSecondDelay(Dly);
	return 0;
}

STATIC INTN SprdClkAdjustablePllSetRate(IN CLK_HW *Hw,
					    UINT32 Rate,
					    UINT32 ParentRate)
{
	CLK_SPRD *Pll = TO_CLK_SPRD(Hw);
	UINT32 OldRate = SprdClkAdjustablePllRecalcRate(Hw, 0) / 1000000;
	UINT32  K;
	UINT32	Mn;
	UINT32 Cfg1;

	Mn = (Rate / 1000000) / 26;
	K = (UINT64)DIV_ROUND_CLOSEST(((Rate / 10000) - 26 * Mn * 100) * 1048576,
			      26 * 100);

	Cfg1 = BITS_PLL_NINT(Mn);
	if (K)
		Cfg1 |= BITS_PLL_KINT(K) | BIT_PLL_SDM_EN;

	DEBUG((EFI_D_INFO, "%s rate %u, k %u, mn %u\n", __ClkGetName(Hw->Clk),
		  (UINT32) Rate, K, Mn));
	__PllRegWrite(Pll->M.Mul.Reg, Cfg1,
		       BITS_PLL_KINT(~0) | BITS_PLL_NINT(~0) | BIT_PLL_SDM_EN);
	__PllEnableTime(Hw, OldRate);
	return 0;
}

#define TO_CLK_MUX(_Hw) BASE_CR(_Hw, CLK_MUX, HW)

UINT8 __ClkGetNumParents(struct Clk *Clk)
{
	return !Clk ? 0 : Clk->NumParents;
}

STATIC UINT8 ClkMuxGetParent(IN CLK_HW *Hw)
{
	CLK_MUX *Mux = TO_CLK_MUX(Hw);
	INTN NumParents = __ClkGetNumParents(Hw->Clk);
	UINT32 Val;

	/*
	 * FIXME need a mux-specific flag to determine if val is bitwise or numeric
	 * e.g. sys_clkin_ck's clksel field is 3 bits wide, but ranges from 0x1
	 * to 0x7 (index starts at one)
	 * OTOH, pmd_trace_clk_mux_ck uses a separate bit for each clock, so
	 * val = 0x4 really means "bit 2, index starts at bit 0"
	 */
	Val = MmioRead32((UINTN)Mux->Reg) >> Mux->Shift;
	Val &= Mux->Mask;

	if (Mux->Table) {
		INTN I;

		for (I = 0; I < NumParents; I++)
			if (Mux->Table[I] == Val)
				return I;
		return -2;
	}

	if (Val && (Mux->Flags & CLK_MUX_INDEX_BIT))
		Val = LowBitSet32(Val) - 1;

	if (Val && (Mux->Flags & CLK_MUX_INDEX_ONE))
		Val--;

	if (Val >= NumParents)
		return -2;

	return Val;
}

STATIC INT32 ClkMuxSetParent(IN CLK_HW *Hw, UINT8 Index)
{
	CLK_MUX *Mux = TO_CLK_MUX(Hw);
	UINT32 Val;
	UINT32 Flags = 0;

	if (Mux->Table)
		Index = Mux->Table[Index];

	else {
		if (Mux->Flags & CLK_MUX_INDEX_BIT)
			Index = (1 << LowBitSet32(Index));

		if (Mux->Flags & CLK_MUX_INDEX_ONE)
			Index++;
	}

	if (Mux->Lock)
		AcquireSpinLock(Mux->Lock);

	Val = MmioRead32((UINTN)Mux->Reg);
	Val &= ~(Mux->Mask << Mux->Shift);
	Val |= Index << Mux->Shift;
	MmioWrite32((UINT32)Val, (UINT32)Mux->Reg);

	if (Mux->Lock)
		ReleaseSpinLock(Mux->Lock);

	return 0;
}

CONST CLK_OPS CLK_MUX_OPS = {
	.GetParent = ClkMuxGetParent,
	.SetParent = ClkMuxSetParent,
};

STATIC UINT8 SprdClkMuxGetParent(IN CLK_HW *Hw)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	if (!C->M.MUX_HW->Clk)
		C->M.MUX_HW->Clk = C->Hw.Clk;
	DEBUG((EFI_D_INFO, "%s\n", __ClkGetName(Hw->Clk)));
	return CLK_MUX_OPS.GetParent(C->M.MUX_HW);
}

STATIC INTN SprdClkMuxSetParent(IN CLK_HW *Hw, UINT8 Index)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	if (!C->M.MUX_HW->Clk)
		C->M.MUX_HW->Clk = C->Hw.Clk;
	DEBUG((EFI_D_INFO, "%s\n", __ClkGetName(Hw->Clk), (UINT32) Index));
	return CLK_MUX_OPS.SetParent(C->M.MUX_HW, Index);
}

#define TO_CLK_DIVIDER(_Hw) BASE_CR(_Hw, CLK_DIVIDER, HW)

#define DIV_MASK(D)   ((1 << ((D)->Width)) - 1)

STATIC UINT32 _GetTableDiv(IN CONST CLK_DIV_TABLE *Table,
							UINT32 Val)
{
	CONST CLK_DIV_TABLE *Clkt;

	for (Clkt = Table; Clkt->Div; Clkt++)
		if (Clkt->Val == Val)
			return Clkt->Div;
	return 0;
}

STATIC UINT32 _GetDiv(IN CLK_DIVIDER *Divider, UINT32 Val)
{
	if (Divider->Flags & CLK_DIVIDER_ONE_BASED)
		return Val;
	if (Divider->Flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << Val;
	if (Divider->Table)
		return _GetTableDiv(Divider->Table, Val);
	return Val + 1;
}

STATIC unsigned long ClkDividerRecalcRate(IN CLK_HW *Hw, unsigned long ParentRate)
{
	CLK_DIVIDER *Divider = TO_CLK_DIVIDER(Hw);
	UINTN Div, Val;

	Val = MmioRead32((UINTN)Divider->Reg) >> Divider->Shift;
	Val &= DIV_MASK(Divider);

	Div = _GetDiv(Divider, Val);
	if (!Div) {
		if (!(Divider->Flags & CLK_DIVIDER_ALLOW_ZERO)) {
			DEBUG((EFI_D_WARN, "%a: Zero divisor and CLK_DIVIDER_ALLOW_ZERO not set\n",
			__ClkGetName(Hw->Clk)));
		}
		return ParentRate;
	}

	return ParentRate / Div;
}

#define IS_POWER_OF_2(x)  ((x) != 0 && (((x) & ((x) - 1)) == 0))

STATIC BOOLEAN _IsValidTableDiv(CONST CLK_DIV_TABLE *Table,
							 unsigned int Div)
{
	CONST CLK_DIV_TABLE *Clkt;

	for (Clkt = Table; Clkt->Div; Clkt++)
		if (Clkt->Div == Div)
			return true;
	return false;
}

STATIC BOOLEAN _IsValidDiv(IN CLK_DIVIDER *Divider, unsigned int Div)
{
	if (Divider->Flags & CLK_DIVIDER_POWER_OF_TWO)
		return IS_POWER_OF_2(Div);
	if (Divider->Table)
		return _IsValidTableDiv(Divider->Table, Div);
	return true;
}



STATIC unsigned int _GetTableMaxdiv(CONST CLK_DIV_TABLE *Table)
{
	unsigned int Maxdiv = 0;
	CONST CLK_DIV_TABLE *Clkt;

	for (Clkt = Table; Clkt->Div; Clkt++)
		if (Clkt->Div > Maxdiv)
			Maxdiv = Clkt->Div;
	return Maxdiv;
}

STATIC unsigned int _GetMaxdiv(IN CLK_DIVIDER *Divider)
{
	if (Divider->Flags & CLK_DIVIDER_ONE_BASED)
		return DIV_MASK(Divider);
	if (Divider->Flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << DIV_MASK(Divider);
	if (Divider->Table)
		return _GetTableMaxdiv(Divider->Table);
	return DIV_MASK(Divider) + 1;
}

unsigned long __ClkRoundRate(IN struct Clk *Clk, UINT32 Rate)
{
	unsigned long ParentRate = 0;

	if (!Clk)
		return 0;

	if (!Clk->Ops->RoundRate) {
		if (Clk->Flags & CLK_SET_RATE_PARENT)
			return __ClkRoundRate(Clk->Parent, Rate);
		else
			return Clk->Rate;
	}

	if (Clk->Parent)
		ParentRate = Clk->Parent->Rate;

	return Clk->Ops->RoundRate(Clk->Hw, Rate, &ParentRate);
}

struct Clk *__ClkGetParent(IN struct Clk *Clk)
{
	return !Clk ? NULL : Clk->Parent;
}

#define MULT_ROUND_UP(R, M)  ((R) * (M) + (M) - 1)

unsigned long __ClkGetFlags(IN struct Clk *Clk)
{
	return !Clk ? 0 : Clk->Flags;
}

STATIC int ClkDividerBestdiv(IN CLK_HW *Hw, unsigned long Rate,
		unsigned long *BestParentRate)
{
	CLK_DIVIDER *Divider = TO_CLK_DIVIDER(Hw);
	int I, Bestdiv = 0;
	unsigned long ParentRate, Best = 0, Now, Maxdiv;

	if (!Rate)
		Rate = 1;

	Maxdiv = _GetMaxdiv(Divider);

	if (!(__ClkGetFlags(Hw->Clk) & CLK_SET_RATE_PARENT)) {
		ParentRate = *BestParentRate;
		Bestdiv = DIV_ROUND_UP(ParentRate, Rate);
		Bestdiv = Bestdiv == 0 ? 1 : Bestdiv;
		Bestdiv = Bestdiv > Maxdiv ? Maxdiv : Bestdiv;
		return Bestdiv;
	}

	/*
	 * The maximum divider we can use without overflowing
	 * unsigned long in rate * I below
	 */
	Maxdiv = min(ULONG_MAX / Rate, Maxdiv);

	for (I = 1; I <= Maxdiv; I++) {
		if (!_IsValidDiv(Divider, I))
			continue;
		ParentRate = __ClkRoundRate(__ClkGetParent(Hw->Clk),
				MULT_ROUND_UP(Rate, I));
		Now = ParentRate / I;
		if (Now <= Rate && Now > Best) {
			Bestdiv = I;
			Best = Now;
			*BestParentRate = ParentRate;
		}
	}

	if (!Bestdiv) {
		Bestdiv = _GetMaxdiv(Divider);
		*BestParentRate = __ClkRoundRate(__ClkGetParent(Hw->Clk), 1);
	}

	return Bestdiv;
}

STATIC long ClkDividerRoundRate(IN CLK_HW *Hw, unsigned long Rate,
				unsigned long *Prate)
{
	INTN Div;
	Div = ClkDividerBestdiv(Hw, Rate, Prate);

	return *Prate / Div;
}



CONST CLK_OPS CLK_DIVIDER_OPS = {
	.RecalcRate = ClkDividerRecalcRate,
	.RoundRate = ClkDividerRoundRate,
	// .SetRate = ClkDividerSetRate,
};

STATIC UINTN SprdClkDividerRecalcRate(IN CLK_HW *Hw,
						  UINTN ParentRate)
{
	CLK_SPRD *C = TO_CLK_SPRD(Hw);
	if (!C->D.DIV_HW->Clk)
		C->D.DIV_HW->Clk = C->Hw.Clk;
	DEBUG((EFI_D_INFO, "%s %lu\n", __ClkGetName(Hw->Clk), ParentRate));
	return CLK_DIVIDER_OPS.RecalcRate(C->D.DIV_HW, ParentRate);
}

CONST CLK_OPS SprdClkFixedPllOps = {
	.Prepare = SprdClkPrepare,
	.Unprepare = SprdClkUnprepare,
	.IsPrepared = SprdClkIsPrepared,
	.RecalcRate = SprdClkFixedPllRecalcRate,
};

CONST CLK_OPS SprdClkAdjustablePllOps  = {
	.Prepare = SprdClkPrepare,
	.Unprepare = SprdClkUnprepare,
	// .RoundRate = SprdClkAdjustablePllRoundRate,
	// .SetRate = SprdClkAdjustablePllSetRate,
	// .RecalcRate = SprdClkAdjustablePllRecalcRate,
};

CONST CLK_OPS SprdClkGateOps = {
	.Prepare = SprdClkPrepare,
	.Unprepare = SprdClkUnprepare,
	.Enable = SprdClkEnable,
	.Disable = SprdClkDisable,
	.IsEnabled = SprdClkIsEnable,
};



STATIC VOID __MmRegSetClr(CLK_HW *Hw, VOID *Reg, UINT32 Msk,
				  int IsSet)
{
	if (!Reg)
		return;

	DEBUG((EFI_D_INFO, "%s %s %p[%x]\n", __ClkGetName(Hw->Clk),
		  (IsSet) ? "SET" : "CLR", Reg, (UINT32) Msk));

	if (IsSet)
		MmioWrite32(MmioRead32((UINTN)Reg) | Msk, (UINTN)Reg);
	else
		MmioWrite32(MmioRead32((UINTN)Reg) & ~Msk, (UINTN)Reg);
}

#define __MmRegSet(Hw, Reg, Msk)	__MmRegSetClr(Hw, Reg, Msk, 1)
#define __MmRegClr(Hw, Reg, Msk)	__MmRegSetClr(Hw, Reg, Msk, 0)
#define __SPRD_MM_TIMEOUT		(3 * 1000)

STATIC UINT32 SavedMmCkg[10];



STATIC
EFI_STATUS
SciClockInit(VOID)
{
  // Register all clocks sources

  // Optional Debug message
  DEBUG((EFI_D_INFO, "[SprdClockDxe]: Clock init complete\n"));

  return EFI_SUCCESS;
}

/**
  Initialize the state information for the SprdClockDxe

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
**/
EFI_STATUS
EFIAPI
SprdClockDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status;
	EFI_HANDLE SprdHandle = NULL;

	DEBUG((EFI_D_INFO, "[SprdClockDxe]: Initializing Spreadtrum Clock Driver\n"));
	
	Status = SciClockInit();
	if (EFI_ERROR(Status)) {
		DEBUG((EFI_D_ERROR, "[SprdClockDxe]: SciClockInit failed: %r", Status));
		return Status;
	}

    

	Status = gBS->InstallMultipleProtocolInterfaces(
        &SprdHandle,
        &gSprdClockProtocolGuid,
        NULL
    );

    ASSERT_EFI_ERROR(Status);

	return EFI_SUCCESS;
}