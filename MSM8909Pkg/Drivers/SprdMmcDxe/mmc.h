#ifndef __MMC_DEF__
#define __MMC_DEF__

#include <Uefi.h>
#include <Uefi/UefiGpt.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/ComponentName.h>
#include <Protocol/Cpu.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

#define DXE_DRIVER_NAME "SprdSdhciDxe"

#define SDHCI_USE_LEDS_CLASS

#define CONFIG_GENERIC_MMC

#define __be32_to_cpu(x)  ((0x000000ff&((x)>>24)) | (0x0000ff00&((x)>>8)) |       \
               (0x00ff0000&((x)<< 8)) | (0xff000000&((x)<<24)))

#define readl(addr)         (*((volatile UINT32 *)(addr)))          /* word input */
#define writel(value,addr)  (*((volatile UINT32 *)(addr))  = (value))   /* word output */

#define SDHCI_TIMEOUT_DIVIDE_VALUE	3

#define REGULATOR_EVENT_ENABLE 		0x00

#define MAX_TUNING_LOOP 40

#define EMMC_BOOT_START_BLOCK (34)
#define SD_BOOT_START_BLOCK (16)

#define SD_VERSION_SD 0x20000
#define SD_VERSION_2  (SD_VERSION_SD | 0x20)
#define SD_VERSION_1_0  (SD_VERSION_SD | 0x10)
#define SD_VERSION_1_10 (SD_VERSION_SD | 0x1a)
#define MMC_VERSION_MMC   0x10000
#define MMC_VERSION_UNKNOWN (MMC_VERSION_MMC)
#define MMC_VERSION_1_2   (MMC_VERSION_MMC | 0x12)
#define MMC_VERSION_1_4   (MMC_VERSION_MMC | 0x14)
#define MMC_VERSION_2_2   (MMC_VERSION_MMC | 0x22)
#define MMC_VERSION_3   (MMC_VERSION_MMC | 0x30)
#define MMC_VERSION_4   (MMC_VERSION_MMC | 0x40)
#define MMC_VERSION_4_1   (MMC_VERSION_MMC | 0x41)
#define MMC_VERSION_4_2   (MMC_VERSION_MMC | 0x42)
#define MMC_VERSION_4_3   (MMC_VERSION_MMC | 0x43)
#define MMC_VERSION_4_41  (MMC_VERSION_MMC | 0x44)
#define MMC_VERSION_4_5   (MMC_VERSION_MMC | 0x45)
#define MMC_VERSION_5_0   (MMC_VERSION_MMC | 0x50)

#define MMC_MID_HYNIX   0x90
#define MMC_MID_SANDISK 0x45
#define MMC_MID_SAMSUNG 0x15
#define MMC_MID_TOSHIBA 0x11


typedef enum {
  READ,
  WRITE
} OPERATION_TYPE;

typedef struct {
  VENDOR_DEVICE_PATH  Mmc;
  EFI_DEVICE_PATH     End;
} SDHC_DEVICE_PATH;

#define MAX_MMC_NUM     3

#endif