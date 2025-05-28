#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/MemoryAllocationLib.h>

#define SPRD_DMA_BASE    0xF5112000

#define MISC_DYNAMIC_MINOR	255

#define DMA_COPY_MINOR MISC_DYNAMIC_MINOR
#define DMA_COPY_USER_MAX 4

EFI_STATUS
EFIAPI
SprdDmaDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable)
{
	
	DEBUG((EFI_D_INFO, "Initializing Spreadtrum DMA Driver\n"));
	return EFI_SUCCESS;
}