#include <Base.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>

#include <SC8830/reg.h>

ClkRegs ClockSdio = {
  .Name      = "CLK_SDIO1",
  .Rate      = 96000000,
  .EnbReg    = 0x7120004c,
  .EnbMask   = 0x200,
  .DivReg    = 0,
  .DivMask   = 0,
  .SelReg    = 0,
  .NrSources = 2,
  .Sources   = {SRC_PLL1, SRC_OSC}
};

ClkRegs ClockEmmc = {
  .Name      = "CLK_EMMC",
  .Rate      = 384000000,
  .EnbReg    = 0x71200054,
  .EnbMask   = 0x800,
  .DivReg    = (UINT32)EMMC_CLK_DIV_REG,
  .DivMask   = EMMC_CLK_DIV_MASK,
  .SelReg    = (UINT32)EMMC_CLK_SEL_REG,
  .NrSources = 4,
  .Sources   = {SRC_PLL1, SRC_PLL2}
};



STATIC struct ClkTable sprd_clocks_8830[] =
{
	
};

EFI_STATUS
EFIAPI
SprdClockInitLib (
  struct clk_lookup **clist,
  unsigned *num
  )
{

	*clist = sprd_clocks_8830;
	*num = ARRAY_SIZE(sprd_clocks_8830);

	return EFI_SUCCESS;
}