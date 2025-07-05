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

#include "clock.h"



#define SHFT_PLL_REFIN                 ( 16 )

EFI_STATUS
EFIAPI
SprdClockDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable)
{
	DEBUG((EFI_D_INFO, "SprdClockDxe: Starting Spreadtrum Clock Driver\n"));
	DEBUG((EFI_D_INFO, "SprdClockDxe: Initializing Voltage\n"));
	DEBUG((EFI_D_INFO, "SprdClockDxe: Initializing PLL\n"));
	
	return EFI_SUCCESS;
}