/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2025 Vicente Cortés <vicenteicc2008@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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

#include <SC8830/reg.h>

#include <string.h>

#include <Protocol/SprdClock.h>
#include "clock.h"

#define __ffs(x) (ffs(x) - 1)

#if 1
const UINT32 __clkinit0, __clkinit_begin = 0xeeeebbbb;
const UINT32 __clkinit2, __clkinit_end   = 0xddddeeee;
#else
const UINT32 __clkinit0, __clkinit_begin = &CLK_LK_clk_mpll;
const UINT32 __clkinit2, __clkinit_end   = &CLK_LK_clk_mpll;
#endif

#define DEFINE_MUTEX(...)
static int clocks_lock = 0;
STATIC LIST_HEAD(Clocks);
STATIC struct mutex clocks_mutex;

int ClkEnable(IN struct Clk *Clk)
{
	unsigned long Flags;
	ClkEnable(Clk->Parent);

	spin_lock_irqsave(&clocks_lock, Flags);
	if ((Clk->Usage++) == 0 && Clk->Enable)
		(Clk->Enable) (Clk, 1, &Flags);
	spin_unlock_irqrestore(&clocks_lock, Flags);
	DEBUG((EFI_D_INFO, "Clk %p, Usage %d\n", Clk, Clk->Usage));

	return EFI_SUCCESS;
}

VOID ClkDisable(IN struct Clk *Clk)
{
	unsigned long Flags;
	
	spin_lock_irqsave(&clocks_lock, Flags);
	if ((--Clk->Usage) == 0 && Clk->Enable)
		(Clk->Enable) (Clk, 0, &Flags);
	spin_unlock_irqrestore(&clocks_lock, Flags);
	DEBUG((EFI_D_INFO, "Clk %p, Usage %d\n", Clk, Clk->Usage));
	ClkDisable(Clk->Parent);
}

VOID ClkForceDisable(IN struct Clk *Clk)
{
	DEBUG((EFI_D_INFO, "clk %p, usage %d\n", Clk, Clk->Usage));
	while (Clk->Usage > 0) {
		ClkDisable(Clk);
	}
}

unsigned long ClkGetRate(struct Clk *Clk)
{
	if (!Clk->Ops->GetRate)
		return 0;

	if (Clk->Parent != NULL)
		return ClkGetRate(Clk->Parent);

	return Clk->Rate;
}

long ClkRoundRate(struct Clk *Clk, unsigned long Rate)
{
	if (!IS_ERR_OR_NULL(Clk) && Clk->Ops && Clk->Ops->RoundRate)
		return (Clk->Ops->RoundRate) (Clk, Rate);

	return Rate;
}

int ClkSetRate(struct Clk *Clk, unsigned long Rate)
{
	int Ret;
	unsigned long Flags;
	DEBUG((EFI_D_INFO, "clk %p, usage %d\n", Clk, Clk->Usage));
	if (!Clk->Ops->SetRate)
		return -7;

	spin_lock_irqsave(&clocks_lock, Flags);
	Ret = (Clk->Ops->SetRate) (Clk, Rate);
	spin_unlock_irqrestore(&clocks_lock, Flags);
	return Ret;
}

struct Clk *ClkGetParent(struct Clk *Clk)
{
	return Clk->Parent;
}

int ClkSetParent(struct Clk *Clk, struct Clk *Parent)
{
	int Ret = -EFI_ACCESS_DENIED;
	unsigned long Flags;
	struct Clk *OldParent = ClkGetParent(Clk);
	DEBUG((EFI_D_INFO, "clk %p, parent %p <<< %p\n", Clk, Parent, OldParent));

	spin_lock_irqsave(&clocks_lock, Flags);
	if (Clk->Ops && Clk->Ops->SetParent)
		Ret = (Clk->Ops->SetParent) (Clk, Parent);
	spin_unlock_irqrestore(&clocks_lock, Flags);

	return Ret;
}

#define REG_GLB_SET(A)                  ( A + 0x1000 )
#define REG_GLB_CLR(A)                  ( A + 0x2000 )

STATIC inline int fls(int X)
{
	int Ret;

	asm("clz\t%0, %1": "=r"(Ret):"r"(X));
	Ret = 32 - Ret;
	return Ret;
}

#define __fls(x) (fls(x) - 1)
#define ffs(x) ({ unsigned long __t = (x); fls(__t & -__t); })

// SCI functions

UINT32 SciGlbRead(UINT32 Reg, UINT32 Msk)
{
	return MmioRead32 (Reg) & Msk;
}

int SciGlbWrite(UINT32 Reg, UINT32 Val, UINT32 Msk)
{
	unsigned long Flags, HwFlags;
	MmioWrite32((MmioRead32(Reg) & ~Msk) | Val, Reg);
	return EFI_SUCCESS;
}

int SciGlbSet(UINT32 Reg, UINT32 Bit)
{
	MmioWrite32(Bit, REG_GLB_SET(Reg));
	return EFI_SUCCESS;
}

int SciGlbClr(UINT32 Reg, UINT32 Bit)
{
	MmioWrite32(Bit, REG_GLB_CLR(Reg));
	return EFI_SUCCESS;
}

STATIC int SciClkEnable(struct Clk *C, int Enable, unsigned long *Pflags)
{
	DEBUG((EFI_D_INFO, "Clk %p (%s) Enb %08x, %s\n", C, C->Regs->Name,
	      C->Regs->Enb.Reg, Enable ? "enable" : "disable"));

	if (C->Regs->Enb.Reg & 1)
		Enable = !Enable;

	if (!C->Regs->Enb.Mask) {	/* enable matrix clock */
		if (Pflags)
			spin_unlock_irqrestore(&clocks_lock, *Pflags);
		if (Enable)
			ClkEnable((struct Clk *)C->Regs->Enb.Reg);
		else
			ClkDisable((struct Clk *)C->Regs->Enb.Reg);
		if (Pflags)
			spin_lock_irqsave(&clocks_lock, *Pflags);
	} else {
		if (Enable)
			SciGlbSet(C->Regs->Enb.Reg & ~1, C->Regs->Enb.Mask);
		else
			SciGlbClr(C->Regs->Enb.Reg & ~1, C->Regs->Enb.Mask);
	}
	return EFI_SUCCESS;
}

STATIC int SciClkIsEnable(struct Clk *C)
{
	int Enable;

	DEBUG((EFI_D_INFO, "Clk %p (%s) Enb %08x\n", C, C->Regs->Name, C->Regs->Enb.Reg));

	if (!C->Regs->Enb.Mask) {	/* check matrix clock */
		Enable = ! !SciClkIsEnable((struct Clk *)C->Regs->Enb.Reg);
	} else {
		Enable =
		    ! !SciGlbRead(C->Regs->Enb.Reg & ~1, C->Regs->Enb.Mask);
	}

	if (C->Regs->Enb.Reg & 1)
		Enable = !Enable;
	return Enable;
}

STATIC int SciClkSetRate(struct Clk *C, unsigned long Rate)
{
	UINT32 Div, DivShift;
	DEBUG((EFI_D_INFO, "Clk %p (%s) Set Rate %lu\n", C, C->Regs->Name, Rate));
	Rate = ClkRoundRate(C, Rate);
	Div = ClkGetRate(C->Parent) / Rate - 1;	//FIXME:
	DivShift = __ffs(C->Regs->Div.Mask);
	DEBUG((EFI_D_INFO, "Clk %p (%s) Pll Div Reg %08x, Val %08x Mask %08x\n", C,
	       C->Regs->Name, C->Regs->Div.Reg, Div << DivShift,
	       C->Regs->Div.Mask));
	SciGlbWrite(C->Regs->Div.Reg, Div << DivShift, C->Regs->Div.Mask);

	C->Rate = 0;		/* FIXME: auto update all children after new rate if need */
	return EFI_SUCCESS;
}

STATIC unsigned long SciClkGetRate(struct Clk *C)
{
	UINT32 Div = 0, DivShift;
	unsigned long Rate;
	DivShift = __ffs(C->Regs->Div.Mask);
	DEBUG((EFI_D_INFO, "Clk %p (%s) Div Reg %08x, Shift %u Msk %08x\n", C,
	       C->Regs->Name, C->Regs->Div.Reg, DivShift, C->Regs->Div.Mask));
	Rate = ClkGetRate(C->Parent);

	if (C->Regs->Div.Reg)
		Div = SciGlbRead(C->Regs->Div.Reg,
				   C->Regs->Div.Mask) >> DivShift;
	DEBUG((EFI_D_INFO, "Clk %p (%s) Parent Rate %lu, Div %u\n", C, C->Regs->Name, Rate,
	       Div + 1));
	C->Rate = Rate = Rate / (Div + 1);	//FIXME:
	DEBUG((EFI_D_INFO, "Clk %p (%s) Get Real Rate %lu\n", C, C->Regs->Name, Rate));
	return Rate;
}

#define SHFT_PLL_REFIN                 ( 16 )
#define MASK_PLL_REFIN                 ( BIT(16)|BIT(17) )

STATIC unsigned long SciPllGetRefinRate(struct Clk *C)
{
	int I;
	const unsigned long Refin[4] = { 2, 4, 4, 13 };	/* default refin 4M */
	I = SciGlbRead(C->Regs->Div.Reg, MASK_PLL_REFIN) >> SHFT_PLL_REFIN;
	DEBUG((EFI_D_INFO, "Pll %p (%s) Refin %d\n", C, C->Regs->Name, I));
	return Refin[I] * 1000000;
}

STATIC unsigned long SciPllGetRate(struct Clk *C)
{
	UINT32 Mn = 1, MnShift;
	unsigned long Rate;
	MnShift = __ffs(C->Regs->Div.Mask);
	DEBUG((EFI_D_INFO, "pll %p (%s) mn reg %08x, shift %u msk %08x\n", C, C->Regs->Name,
	       C->Regs->Div.Reg, MnShift, C->Regs->Div.Mask));
	Rate = ClkGetRate(C->Parent);
	if (0 == C->Regs->Div.Reg) ;
	else if (C->Regs->Div.Reg < MAX_DIV) {
		Mn = C->Regs->Div.Reg;
		if (Mn)
			Rate = Rate / Mn;
	} else {
		Rate = SciPllGetRefinRate(C);
		Mn = SciGlbRead(C->Regs->Div.Reg,
				  C->Regs->Div.Mask) >> MnShift;
		if (Mn)
			Rate = Rate * Mn;
	}
	C->Rate = Rate;
	DEBUG((EFI_D_INFO, "Pll %p (%s) Get Real Rate %lu\n", C, C->Regs->Name, Rate));
	return Rate;
}

STATIC unsigned long SciClkRoundRate(struct Clk *C, unsigned long Rate)
{
	DEBUG((EFI_D_INFO, "clk %p (%s) round rate %lu\n", C, C->Regs->Name, Rate));
	return Rate;
}

STATIC int SciClkSetParent(struct Clk *C, struct Clk *Parent)
{
	int I;
	DEBUG((EFI_D_INFO, "clk %p (%s) parent %p (%s)\n", C, C->Regs->Name,
	       Parent, Parent ? Parent->Regs->Name : 0));

	for (I = 0; I < C->Regs->NrSources; I++) {
		if (C->Regs->Sources[I] == Parent) {
			UINT32 SelShift = __ffs(C->Regs->Sel.Mask);
			DEBUG((EFI_D_INFO, "Pll Sel Reg %08x, Val %08x, Msk %08x\n",
			       C->Regs->Sel.Reg, I << SelShift,
			       C->Regs->Sel.Mask));
			if (C->Regs->Sel.Reg)
				SciGlbWrite(C->Regs->Sel.Reg, I << SelShift,
					      C->Regs->Sel.Mask);
			C->Parent = Parent;
			if (C->Ops)
				C->Rate = 0;	/* FIXME: auto update clock rate after new parent */
			return 0;
		}
	}

	DEBUG((EFI_D_WARN, "[SprdClockDxe]: warning: clock (%s) not support parent (%s)\n",
	     C->Regs->Name, Parent ? Parent->Regs->Name : 0));
	return EFI_INVALID_PARAMETER;
}

STATIC int SciClkGetParent(struct Clk *C)
{
	int I = 0;
	UINT32 SelShift = __ffs(C->Regs->Sel.Mask);
	DEBUG((EFI_D_INFO, "Pll Sel Reg %08x, Val %08x, Msk %08x\n",
	       C->Regs->Sel.Reg, I << SelShift, C->Regs->Sel.Mask));
	if (C->Regs->Sel.Reg) {
		I = SciGlbRead(C->Regs->Sel.Reg,
				 C->Regs->Sel.Mask) >> SelShift;
	}
	return I;
}

STATIC struct ClkOps GenericClkOps = {
	.SetRate = SciClkSetRate,
	.GetRate = SciClkGetRate,
	.RoundRate = SciClkRoundRate,
	.SetParent = SciClkSetParent,
};

STATIC struct ClkOps GenericPllOps = {
	.SetRate = 0,
	.GetRate = SciPllGetRate,
	.RoundRate = 0,
	.SetParent = SciClkSetParent,
};

STATIC int __clk_is_dummy_pll(struct Clk *C)
{
	return (C->Regs->Enb.Reg & 1) || AsciiStrStr(C->Regs->Name, "Pll");
}

VOID mutex_lock(struct mutex* lock)
{
  return;
}

VOID mutex_unlock(struct mutex* lock)
{
  return;
}

VOID ClkDevAdd(struct ClkLookup *Cl)
{
	mutex_lock(&clocks_mutex);
	list_add_tail(&Cl->Node, &Clocks);
	mutex_unlock(&clocks_mutex);
}

int SciClkRegister(struct ClkLookup *Cl)
{
	struct Clk *C = Cl->Clk;

	if (C->Ops == NULL) {
		C->Ops = &GenericClkOps;
		if (C->Rate)	/* fixed OSC */
			C->Ops = NULL;
		else if ((C->Regs->Div.Reg >= 0 && C->Regs->Div.Reg < MAX_DIV)
			 || AsciiStrStr(C->Regs->Name, "Pll")) {
			C->Ops = &GenericPllOps;
		}
	}

	DEBUG
	    ((EFI_D_INFO, "Clk %p (%s) Rate %lu Ops %p Enb %08x Sel %08x Div %08x NrSources %u\n",
	     C, C->Regs->Name, C->Rate, C->Ops, C->Regs->Enb.Reg,
	     C->Regs->Sel.Reg, C->Regs->Div.Reg, C->Regs->NrSources));

	if (C->Enable == NULL && C->Regs->Enb.Reg) {
		C->Enable = SciClkEnable;
		/* FIXME: dummy update some pll clocks usage */
		if (SciClkIsEnable(C) && __clk_is_dummy_pll(C)) {
			ClkEnable(C);
		}
	}

	if (!C->Rate) {		/* FIXME: dummy update clock parent and rate */
		ClkSetParent(C, C->Regs->Sources[SciClkGetParent(C)]);
		/* clk_set_rate(C, clk_get_rate(C)); */
	}

	ClkDevAdd(Cl);

	return 0;
}

STATIC
EFI_STATUS
SciClockInit(VOID)
{
  // MMIO writes
  MmioWrite32(REG_PMU_APB_PD_MM_TOP_CFG,
              MmioRead32(REG_PMU_APB_PD_MM_TOP_CFG) & ~(BIT_PD_MM_TOP_FORCE_SHUTDOWN));

  MmioWrite32(REG_PMU_APB_PD_GPU_TOP_CFG,
              MmioRead32(REG_PMU_APB_PD_GPU_TOP_CFG) & ~(BIT_PD_GPU_TOP_FORCE_SHUTDOWN));

  // Register all clocks sources
  {
	struct ClkLookup *Cl =
	    (struct ClkLookup *)(&__clkinit_begin + 1);
	DEBUG((EFI_D_INFO, "%p (%x) -- %p -- %p (%x)\n",
	       &__clkinit_begin, __clkinit_begin, Cl, &__clkinit_end,
	       __clkinit_end));
	while (Cl < (struct ClkLookup *)&__clkinit_end) {
		SciClkRegister(Cl);			
		Cl++;
	}
  }

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

	DEBUG((EFI_D_INFO, "[SprdClockDxe]: Starting Spreadtrum Clock Driver\n"));

	Status = gBS->InstallMultipleProtocolInterfaces(
        &SprdHandle,
        &gSprdClockProtocolGuid,
        NULL
    );

    ASSERT_EFI_ERROR(Status);

	return EFI_SUCCESS;
}