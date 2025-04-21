#include <PiDxe.h>

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
SprdSmemDxeInitialize(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_HANDLE Handle = NULL;
  EFI_STATUS Status;


  Status = gBS->InstallMultipleProtocolInterfaces(
      &Handle, &gSprdSmemProtocolGuid, gSMEM, NULL);
  ASSERT_EFI_ERROR(Status);

  return Status;
}
