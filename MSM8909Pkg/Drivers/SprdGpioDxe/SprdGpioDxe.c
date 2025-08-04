/*
 * Copyright (c) 2024-2025, DODO vi-C <vicenteicc2008@gmail.com>
 * Based on the open source driver from edk2-tensor, the key reading code from the uniLoader fork by BotchedRPR and GPIO code from linux kernel by Thunderoar
 */

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/HardwareInterrupt.h>
#include <Protocol/SprdGpio.h>

#include "SprdGpio.h"

/* 16 GPIO share a group of registers */
#define	GPIO_GROUP_NR		(16)
#define GPIO_GROUP_MASK		(0xFFFF)

#define	GPIO_GROUP_OFFSET	(0x80)
#define	ANA_GPIO_GROUP_OFFSET	(0x40)

/* registers definitions for GPIO controller */
#define REG_GPIO_DATA		(0x0000)
#define REG_GPIO_DMSK		(0x0004)
#define REG_GPIO_DIR		(0x0008)	/* only for gpio */
#define REG_GPIO_IS		(0x000c)	/* only for gpio */
#define REG_GPIO_IBE		(0x0010)	/* only for gpio */
#define REG_GPIO_IEV		(0x0014)
#define REG_GPIO_IE		(0x0018)
#define REG_GPIO_RIS		(0x001c)
#define REG_GPIO_MIS		(0x0020)
#define REG_GPIO_IC		(0x0024)
#define REG_GPIO_INEN		(0x0028)	/* only for gpio */

/* 8 EIC share a group of registers */
#define	EIC_GROUP_NR		(8)
#define EIC_GROUP_MASK		(0xFF)

/* registers definitions for EIC controller */
#define REG_EIC_DATA		REG_GPIO_DATA
#define REG_EIC_DMSK		REG_GPIO_DMSK
#define REG_EIC_IEV		REG_GPIO_IEV
#define REG_EIC_IE		REG_GPIO_IE
#define REG_EIC_RIS		REG_GPIO_RIS
#define REG_EIC_MIS		REG_GPIO_MIS
#define REG_EIC_IC		REG_GPIO_IC
#define REG_EIC_TRIG		(0x0028)	/* only for eic */
#define REG_EIC_0CTRL		(0x0040)
#define REG_EIC_1CTRL		(0x0044)
#define REG_EIC_2CTRL		(0x0048)
#define REG_EIC_3CTRL		(0x004c)
#define REG_EIC_4CTRL		(0x0050)
#define REG_EIC_5CTRL		(0x0054)
#define REG_EIC_6CTRL		(0x0058)
#define REG_EIC_7CTRL		(0x005c)
#define REG_EIC_DUMMYCTRL	(0x0000)

/* bits definitions for register REG_EIC_DUMMYCTRL */
#define BIT_FORCE_CLK_DBNC	BIT(15)
#define BIT_EIC_DBNC_EN		BIT(14)
#define SHIFT_EIC_DBNC_CNT	(0)
#define MASK_EIC_DBNC_CNT	(0xFFF)
#define BITS_EIC_DBNC_CNT(_x_)	((_x) & 0xFFF)

#define GPIO_INVALID_ID 0xffff
#define INVALID_REG		(~(UINT32)0)

UINT32 GpioBase = FixedPcdGet32(GpioBase);

UINT32
GpioGet(UINT32 GpioNumber)
{
    UINT32 val = -1;

    val = (MmioRead32(GpioBase) >> GpioNumber) & 0x1;

    if(val == -1) DEBUG((EFI_D_ERROR, "Error Reading GPIO\n"));

    return val;
}

UINT32
GpioSet(UINT32 Offset)
{
    UINT32 val = -1;

    MmioWrite32(GpioBase, REG_GPIO_DATA);

    if(val == -1) DEBUG((EFI_D_ERROR, "Error Writing GPIO\n"));

    return val;
}

SPRD_GPIO  gSprdGpio = {
  GpioGet,
  GpioSet
};

EFI_STATUS
EFIAPI
SprdGpioDxeInitialize(
	IN EFI_HANDLE         ImageHandle,
	IN EFI_SYSTEM_TABLE   *SystemTable
)
{
  EFI_STATUS  Status = EFI_SUCCESS;
  EFI_HANDLE  Handle = NULL;

  DEBUG((EFI_D_INFO, "SprdGpioDxe: Initializing Spreadtrum GPIO Driver\n"));

  //
  // Make sure the Spreadtrum Gpio protocol has not been installed in the system yet.
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gSprdGpioProtocolGuid);

  // Install the GPIO Protocol onto a new handle
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gSprdGpioProtocolGuid,
                  &gSprdGpio,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
  }

	return Status;
}