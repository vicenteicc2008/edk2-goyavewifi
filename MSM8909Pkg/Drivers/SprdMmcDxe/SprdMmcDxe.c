#include "mmc.h"

#define  SDHCI_PRESENT_STATE	0x24
#define  SDHCI_CMD_INHIBIT	0x00000001
#define  SDHCI_DATA_INHIBIT	0x00000002
#define  SDHCI_DOING_WRITE	0x00000100
#define  SDHCI_DOING_READ	0x00000200
#define  SDHCI_SPACE_AVAILABLE	0x00000400
#define  SDHCI_DATA_AVAILABLE	0x00000800
#define  SDHCI_CARD_PRESENT	0x00010000
#define  SDHCI_WRITE_PROTECT	0x00080000
#define  SDHCI_DATA_LVL_MASK	0x00F00000
#define  SDHCI_DATA_LVL_SHIFT	20

#define SDHCI_DMA_ADDRESS	0x00
#define SDHCI_ARGUMENT2		SDHCI_DMA_ADDRESS

#define SDHCI_INT_STATUS	0x30
#define SDHCI_INT_ENABLE	0x34
#define SDHCI_SIGNAL_ENABLE	0x38

#define SDHCI_ADMA_ERROR	0x54
#define SDHCI_ADMA_ADDRESS	0x58


EFI_BLOCK_IO_MEDIA gSdMmc0 = {
  SIGNATURE_32('e','m','m','c'),            // MediaId
  FALSE,                                     // RemovableMedia
  FALSE,                                    // MediaPresent
  FALSE,                                    // LogicalPartition
  FALSE,                                    // ReadOnly
  FALSE,                                    // WriteCaching
  512,                                      // BlockSize
  4,                                        // IoAlign
  0,                                        // Pad
  0                                         // LastBlock
};

EFI_BLOCK_IO_MEDIA gSdMmc2 = {
  SIGNATURE_32('s','d','h','c'),            // MediaId
  TRUE,                                     // RemovableMedia
  FALSE,                                    // MediaPresent
  FALSE,                                    // LogicalPartition
  FALSE,                                    // ReadOnly
  FALSE,                                    // WriteCaching
  512,                                      // BlockSize
  4,                                        // IoAlign
  0,                                        // Pad
  0                                         // LastBlock
};

extern UINT32                     gFileSyStemSize;

EFI_HANDLE gSdMmcHandleArray[MAX_MMC_NUM];



EFI_STATUS
EFIAPI
SprdSdhciDxeInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
	EFI_STATUS  Status;
	EFI_BLOCK_IO_PROTOCOL* SprdBlockIo;
	SDHC_DEVICE_PATH* gSprdMmcDevicePath;

	UINTN i;
	UINT64 Lba;
	DEBUG((EFI_D_INFO, "SprdSdhciDxe: Initializing MMC/SD card\n"));

	// Install BlockIO Protocol
	DEBUG((EFI_D_INFO, "SprdSdhciDxe: Installing Block IO Protocol\n"));

	Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiBlockIoProtocolGuid, &SprdBlockIo,
                  &gEfiDevicePathProtocolGuid, &gSprdMmcDevicePath,
                  NULL
                  );
	ASSERT_EFI_ERROR (Status);

	return EFI_SUCCESS;
}