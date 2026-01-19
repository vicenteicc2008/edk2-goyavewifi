#include "MMCHS.h"
#include <Chipset/mmc.h>
int sprd_slot = 0;
struct mmc *
PlatformCallbackInitSlot(struct mmc *mmc)
{
  EFI_STATUS    Status;
  BIO_INSTANCE *Instance;

  // Initialize MMC device
  Status = mmc_init(mmc);
  if (Status != 0) {
    return NULL;
  }

  // Allocate instance
  Status = BioInstanceContructor(&Instance);
  if (EFI_ERROR(Status)) {
    return mmc;
  }

  // Set data
  Instance->MmcDev               = mmc;
  Instance->BlockMedia.BlockSize = mmc->read_bl_len;
  Instance->BlockMedia.LastBlock =
      mmc->capacity / Instance->BlockMedia.BlockSize - 1;

  // Give every device a slighty different GUID
  Instance->DevicePath.Mmc.Guid.Data4[7] = sprd_slot;
  // Register for ExitBS event
  Status = gBS->CreateEventEx(
      EVT_NOTIFY_SIGNAL, TPL_NOTIFY, MMCHSExitBsUninit, (VOID *)Instance,
      &gEfiEventExitBootServicesGuid, &Instance->ExitBsEvent);
  ASSERT_EFI_ERROR(Status);

  // Publish BlockIO
  Status = gBS->InstallMultipleProtocolInterfaces(
      &Instance->Handle, &gEfiBlockIoProtocolGuid, &Instance->BlockIo,
      &gEfiDevicePathProtocolGuid, &Instance->DevicePath, NULL);
  ASSERT_EFI_ERROR(Status);

  return mmc;
}

STATIC BIO_INSTANCE mBioTemplate = {
    BIO_INSTANCE_SIGNATURE,
    NULL, // Handle
    {
        // BlockIo
        EFI_BLOCK_IO_INTERFACE_REVISION, // Revision
        NULL,                            // *Media
        MMCHSReset,                      // Reset
        MMCHSReadBlocks,                 // ReadBlocks
        MMCHSWriteBlocks,                // WriteBlocks
        MMCHSFlushBlocks                 // FlushBlocks
    },
    {
        // BlockMedia
        BIO_INSTANCE_SIGNATURE, // MediaId
        FALSE,                  // RemovableMedia
        TRUE,                   // MediaPresent
        FALSE,                  // LogicalPartition
        FALSE,                  // ReadOnly
        FALSE,                  // WriteCaching
        0,                      // BlockSize
        4,                      // IoAlign
        0,                      // Pad
        0                       // LastBlock
    },
    {
        // DevicePath
        {
            {
                HARDWARE_DEVICE_PATH,
                HW_VENDOR_DP,
                {(UINT8)(sizeof(VENDOR_DEVICE_PATH)),
                 (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)},
            },
            // Hardware Device Path for Bio
            EFI_CALLER_ID_GUID // Use the driver's GUID
        },

        {
            END_DEVICE_PATH_TYPE,
            END_ENTIRE_DEVICE_PATH_SUBTYPE,
            {sizeof(EFI_DEVICE_PATH_PROTOCOL), 0},
        },
    },
    NULL, // MMCDev
    NULL, // ExitBS Event
};

EFI_STATUS
EFIAPI
MMCHSReset(IN EFI_BLOCK_IO_PROTOCOL *This, IN BOOLEAN ExtendedVerification)
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MMCHSReadBlocks(
    IN EFI_BLOCK_IO_PROTOCOL *This, IN UINT32 MediaId, IN EFI_LBA Lba,
    IN UINTN BufferSize, OUT VOID *Buffer)
{
  BIO_INSTANCE *      Instance;
  EFI_BLOCK_IO_MEDIA *Media;
  EFI_TPL             OldTpl;
  UINTN               BlockSize;
  UINT64               RC;

  Instance  = BIO_INSTANCE_FROM_BLOCKIO_THIS(This);
  Media     = &Instance->BlockMedia;
  BlockSize = Media->BlockSize;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL(TPL_NOTIFY);
  RC     = mmc_bread(Instance->MmcDev, (UINT64)Lba, BufferSize/BlockSize, Buffer);
  gBS->RestoreTPL(OldTpl);

  if (RC != 0)
    return EFI_SUCCESS;
  else
    return EFI_DEVICE_ERROR;
}



EFI_STATUS
EFIAPI
MMCHSWriteBlocks(
    IN EFI_BLOCK_IO_PROTOCOL *This, IN UINT32 MediaId, IN EFI_LBA Lba,
    IN UINTN BufferSize, IN VOID *Buffer)
{
  BIO_INSTANCE *      Instance;
  EFI_BLOCK_IO_MEDIA *Media;
  UINTN               BlockSize;
  UINTN               RC;
  EFI_TPL             OldTpl;

  Instance  = BIO_INSTANCE_FROM_BLOCKIO_THIS(This);
  Media     = &Instance->BlockMedia;
  BlockSize = Media->BlockSize;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Lba > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Lba + (BufferSize / BlockSize) - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  // Here goes a fail-safe design (see issue #5)
  // Assume the partition layout before partition 36 is identical on our target
  // devices
  // Only slot 1 (eMMC) is protected

  OldTpl = gBS->RaiseTPL(TPL_NOTIFY);
  RC     = mmc_bwrite(Instance->MmcDev, (UINT64)Lba, BufferSize/BlockSize, Buffer);
  gBS->RestoreTPL(OldTpl);

  if (RC != 0)
    return EFI_SUCCESS;
  else
    return EFI_DEVICE_ERROR;
}

EFI_STATUS
EFIAPI
MMCHSFlushBlocks(IN EFI_BLOCK_IO_PROTOCOL *This)
{
  // Nothing required
  return EFI_SUCCESS;
}

VOID EFIAPI MMCHSExitBsUninit(IN EFI_EVENT Event, IN VOID *Context)
{
  EFI_TPL OldTpl;

  BIO_INSTANCE *Instance = (BIO_INSTANCE *)Context;
  ASSERT(Instance != NULL);

  OldTpl = gBS->RaiseTPL(TPL_NOTIFY);

  // Put card into sleep
  //mmc_put_card_to_sleep(Instance->MmcDev);

  gBS->RestoreTPL(OldTpl);
}

EFI_STATUS
BioInstanceContructor(OUT BIO_INSTANCE **NewInstance)
{
  BIO_INSTANCE *Instance;

  Instance = AllocateCopyPool(sizeof(BIO_INSTANCE), &mBioTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->BlockIo.Media = &Instance->BlockMedia;

  *NewInstance = Instance;
  return EFI_SUCCESS;
}





EFI_STATUS
EFIAPI
MMCHSInitialize(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  sprd_host_init(sprd_slot);
  return EFI_SUCCESS;
}