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

#include <Protocol/SprdGpio.h>

#include "ioctl.h"

#define CONFIG_GENERIC_MMC

#define __be32_to_cpu(x)  ((0x000000ff&((x)>>24)) | (0x0000ff00&((x)>>8)) |       \
               (0x00ff0000&((x)<< 8)) | (0xff000000&((x)<<24)))

#define readl(addr)         (*((volatile UINT32 *)(addr)))          /* word input */
#define writel(value,addr)  (*((volatile UINT32 *)(addr))  = (value))   /* word output */

#define SDHCI_TIMEOUT_DIVIDE_VALUE	3

#define REGULATOR_EVENT_ENABLE 		0x00

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

#define SDHCI_SOFTWARE_RESET	0x2F
#define SDHCI_RESET_ALL		0x01
#define SDHCI_RESET_CMD		0x02
#define SDHCI_RESET_DATA	0x04

#define SDHCI_INT_STATUS	0x30
#define SDHCI_INT_ENABLE	0x34
#define SDHCI_SIGNAL_ENABLE	0x38

#define SDHCI_INT_ALL_MASK	((unsigned int)-1)

#define  SDHCI_INT_DATA_END	0x00000002
#define  SDHCI_INT_BLK_GAP	0x00000004
#define  SDHCI_INT_DMA_END	0x00000008
#define  SDHCI_INT_RESPONSE	0x00000001
#define  SDHCI_INT_SPACE_AVAIL	0x00000010
#define  SDHCI_INT_DATA_AVAIL	0x00000020
#define  SDHCI_INT_CARD_INSERT	0x00000040
#define  SDHCI_INT_CARD_REMOVE	0x00000080
#define  SDHCI_INT_CARD_INT	0x00000100
#define  SDHCI_INT_ERROR	0x00008000
#define  SDHCI_INT_TIMEOUT	0x00010000
#define  SDHCI_INT_CRC		0x00020000
#define  SDHCI_INT_END_BIT	0x00040000
#define  SDHCI_INT_INDEX	0x00080000
#define  SDHCI_INT_DATA_TIMEOUT	0x00100000
#define  SDHCI_INT_DATA_CRC	0x00200000
#define  SDHCI_INT_DATA_END_BIT	0x00400000
#define  SDHCI_INT_BUS_POWER	0x00800000
#define  SDHCI_INT_ACMD12ERR	0x01000000
#define  SDHCI_INT_ADMA_ERROR	0x02000000

#define SDHCI_HOST_CONTROL	0x28
#define  SDHCI_CTRL_LED		0x01

typedef struct {
	INTN			index;
	UINTN			f_min;
	UINTN			f_max;
	UINTN			f_init;
	UINT32			ocr_avail;
	UINT32			ocr_avail_sdio;	/* SDIO-specific OCR */
	UINT32			ocr_avail_sd;	/* SD-specific OCR */
	UINT32			ocr_avail_mmc;	/* MMC-specific OCR */
	UINT32			max_current_330;
	UINT32			max_current_300;
	UINT32			max_current_180;
	UINTN 			MaxBlkCount;	/* maximum number of blocks in one req */
	UINT32			Caps;
	UINT32			Caps2;
} MMC_HOST;

typedef struct {
    EFI_PHYSICAL_ADDRESS ioaddr;  // Dirección base de los registros
    UINT32 flags;                 // Bandera para ver si se usa ADMA
	UINTN Quirks;
	#define SDHCI_QUIRK_CLOCK_BEFORE_RESET			(1<<0)
	/* Controller has bad caps bits, but really supports DMA */
	#define SDHCI_QUIRK_FORCE_DMA				(1<<1)
	/* Controller doesn't like to be reset when there is no card inserted. */
	#define SDHCI_QUIRK_NO_CARD_NO_RESET			(1<<2)
	/* Controller doesn't like clearing the power reg before a change */
	#define SDHCI_QUIRK_SINGLE_POWER_WRITE			(1<<3)
	/* Controller has flaky internal state so reset it on each ios change */
	#define SDHCI_QUIRK_RESET_CMD_DATA_ON_IOS		(1<<4)
	/* Controller has an unusable DMA engine */
	#define SDHCI_QUIRK_BROKEN_DMA				(1<<5)
	/* Controller has an unusable ADMA engine */
	#define SDHCI_QUIRK_BROKEN_ADMA				(1<<6)
	/* Controller can only DMA from 32-bit aligned addresses */
	#define SDHCI_QUIRK_32BIT_DMA_ADDR			(1<<7)
	/* Controller can only DMA chunk sizes that are a multiple of 32 bits */
	#define SDHCI_QUIRK_32BIT_DMA_SIZE			(1<<8)
	/* Controller can only ADMA chunks that are a multiple of 32 bits */
	#define SDHCI_QUIRK_32BIT_ADMA_SIZE			(1<<9)
	/* Controller needs to be reset after each request to stay stable */
	#define SDHCI_QUIRK_RESET_AFTER_REQUEST			(1<<10)
	/* Controller needs voltage and power writes to happen separately */
	#define SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER		(1<<11)
	/* Controller provides an incorrect timeout value for transfers */
	#define SDHCI_QUIRK_BROKEN_TIMEOUT_VAL			(1<<12)
	/* Controller has an issue with buffer bits for small transfers */
	#define SDHCI_QUIRK_BROKEN_SMALL_PIO			(1<<13)
	/* Controller does not provide transfer-complete interrupt when not busy */
	#define SDHCI_QUIRK_NO_BUSY_IRQ				(1<<14)
	/* Controller has unreliable card detection */
	#define SDHCI_QUIRK_BROKEN_CARD_DETECTION		(1<<15)
	/* Controller reports inverted write-protect state */
	#define SDHCI_QUIRK_INVERTED_WRITE_PROTECT		(1<<16)
	/* Controller has nonstandard clock management */
	#define SDHCI_QUIRK_NONSTANDARD_CLOCK			(1<<17)
	/* Controller does not like fast PIO transfers */
	#define SDHCI_QUIRK_PIO_NEEDS_DELAY			(1<<18)
	/* Controller losing signal/interrupt enable states after reset */
	#define SDHCI_QUIRK_RESTORE_IRQS_AFTER_RESET		(1<<19)
	/* Controller has to be forced to use block size of 2048 bytes */
	#define SDHCI_QUIRK_FORCE_BLK_SZ_2048			(1<<20)
	/* Controller cannot do multi-block transfers */
	#define SDHCI_QUIRK_NO_MULTIBLOCK			(1<<21)
	/* Controller can only handle 1-bit data transfers */
	#define SDHCI_QUIRK_FORCE_1_BIT_DATA			(1<<22)
	/* Controller needs 10ms delay between applying power and clock */
	#define SDHCI_QUIRK_DELAY_AFTER_POWER			(1<<23)
	/* Controller uses SDCLK instead of TMCLK for data timeouts */
	#define SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK		(1<<24)
	/* Controller reports wrong base clock capability */
	#define SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN		(1<<25)
	/* Controller cannot support End Attribute in NOP ADMA descriptor */
	#define SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC		(1<<26)
	/* Controller is missing device caps. Use caps provided by host */
	#define SDHCI_QUIRK_MISSING_CAPS			(1<<27)
	/* Controller uses Auto CMD12 command to stop the transfer */
	#define SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12		(1<<28)
	/* Controller doesn't have HISPD bit field in HI-SPEED SD card */
	#define SDHCI_QUIRK_NO_HISPD_BIT			(1<<29)
	/* Controller treats ADMA descriptors with length 0000h incorrectly */
	#define SDHCI_QUIRK_BROKEN_ADMA_ZEROLEN_DESC		(1<<30)
	/* The read-only detection via SDHCI_PRESENT_STATE register is unstable */
	#define SDHCI_QUIRK_UNSTABLE_RO_DETECT			(1<<31)
	UINTN Quirks2;
	#define SDHCI_QUIRK2_HOST_OFF_CARD_ON			(1<<0)
	#define SDHCI_QUIRK2_HOST_NO_CMD23			(1<<1)
	/* The system physically doesn't support 1.8v, even if the host does */
	#define SDHCI_QUIRK2_NO_1_8_V				(1<<2)
	#define SDHCI_QUIRK2_PRESET_VALUE_BROKEN		(1<<3)
	/* Controller data timeout counter is x times long as spec defined */
	#define SDHCI_QUIRK2_TIMEOUT_DIVIDE			(1<<5)
	#define SDHCI_QUIRK2_USE_MAX_DISCARD_SIZE		(1<<7)
	UINTN irq;		/* Device IRQ */
	#define SDHCI_USE_SDMA		(1<<0)	/* Host is SDMA capable */
	#define SDHCI_USE_ADMA		(1<<1)	/* Host is ADMA capable */
	#define SDHCI_REQ_USE_DMA	(1<<2)	/* Use DMA for this req. */
	#define SDHCI_DEVICE_DEAD	(1<<3)	/* Device unresponsive */
	#define SDHCI_SDR50_NEEDS_TUNING (1<<4)	/* SDR50 needs tuning */
	#define SDHCI_NEEDS_RETUNING	(1<<5)	/* Host needs retuning */
	#define SDHCI_AUTO_CMD12	(1<<6)	/* Auto CMD12 support */
	#define SDHCI_AUTO_CMD23	(1<<7)	/* Auto CMD23 support */
	#define SDHCI_PV_ENABLED	(1<<8)	/* Preset value enabled */
	#define SDHCI_SDIO_IRQ_ENABLED	(1<<9)	/* SDIO irq enabled */
	#define SDHCI_HS200_NEEDS_TUNING (1<<10)	/* HS200 needs tuning */
	#define SDHCI_USING_RETUNING_TIMER (1<<11)	/* Host is using a retuning timer for the card */
	UINTN version;
	UINTN max_clk;	/* Max possible freq (MHz) */
	UINTN timeout_clk;	/* Timeout freq (KHz) */
	UINTN clk_mul;	/* Clock Muliplier value */
	UINTN clock;
	UINT8 pwr;
	UINTN BaseAddress;
	struct MMC_HOST *Mmc;
	const struct SDHCI_OPS *Ops;
} SDHCI_HOST;

struct mmc_ios {
	UINTN	clock;			/* clock rate */
	UINT16	vdd;

/* vdd stores the bit number of the selected voltage range from below. */

	UINT8	bus_mode;		/* command output mode */

#define MMC_BUSMODE_OPENDRAIN	1
#define MMC_BUSMODE_PUSHPULL	2

	UINT8	chip_select;		/* SPI chip select */

#define MMC_CS_DONTCARE		0
#define MMC_CS_HIGH		1
#define MMC_CS_LOW		2

	UINT8	power_mode;		/* power supply mode */

#define MMC_POWER_OFF		0
#define MMC_POWER_UP		1
#define MMC_POWER_ON		2

	UINT8	bus_width;		/* data bus width */

#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

	UINT8	timing;			/* timing specification used */

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	3
#define MMC_TIMING_UHS_SDR25	4
#define MMC_TIMING_UHS_SDR50	5
#define MMC_TIMING_UHS_SDR104	6
#define MMC_TIMING_UHS_DDR50	7
#define MMC_TIMING_MMC_HS200	8

#define MMC_SDR_MODE		0
#define MMC_1_2V_DDR_MODE	1
#define MMC_1_8V_DDR_MODE	2
#define MMC_1_2V_SDR_MODE	3
#define MMC_1_8V_SDR_MODE	4

	UINT8	signal_voltage;		/* signalling voltage (1.8V or 3.3V) */

#define MMC_SIGNAL_VOLTAGE_330	0
#define MMC_SIGNAL_VOLTAGE_180	1
#define MMC_SIGNAL_VOLTAGE_120	2

	UINT8	drv_type;		/* driver type (A, B, C, D) */

#define MMC_SET_DRIVER_TYPE_B	0
#define MMC_SET_DRIVER_TYPE_A	1
#define MMC_SET_DRIVER_TYPE_C	2
#define MMC_SET_DRIVER_TYPE_D	3
};

struct SDHCI_OPS {
  VOID   (*SetClock)(IN SDHCI_HOST *Host, IN UINT32 Clock);
  INTN   (*EnableDma)(IN SDHCI_HOST *Host);
  UINT32 (*GetMaxClock)(IN SDHCI_HOST *Host);
  UINT32 (*GetMinClock)(IN SDHCI_HOST *Host);
  UINT32 (*GetTimeoutClock)(IN SDHCI_HOST *Host);
  INTN   (*PlatformBusWidth)(IN SDHCI_HOST *Host, IN INTN Width);
  VOID   (*PlatformSendInit74Clocks)(IN SDHCI_HOST *Host, IN UINT8 PowerMode);
  UINT32 (*GetRo)(IN SDHCI_HOST *Host);
  VOID   (*PlatformResetEnter)(IN SDHCI_HOST *Host, IN UINT8 Mask);
  VOID   (*PlatformResetExit)(IN SDHCI_HOST *Host, IN UINT8 Mask);
  INTN   (*SetUhsSignaling)(IN SDHCI_HOST *Host, IN UINT32 Uhs);
  VOID   (*HwReset)(IN SDHCI_HOST *Host);
  VOID   (*PlatformSuspend)(IN SDHCI_HOST *Host);
  VOID   (*PlatformResume)(IN SDHCI_HOST *Host);
  VOID   (*AdmaWorkaround)(IN SDHCI_HOST *Host, IN UINT32 IntMask);
  VOID   (*PlatformInit)(IN SDHCI_HOST *Host);
};

STATIC inline VOID SdhciWriteb(IN SDHCI_HOST *Host, IN UINT8 Val, IN UINTN Reg) {
  MmioWrite8((UINTN)Host->ioaddr + Reg, Val);
}

STATIC inline UINT8 SdhciReadb(IN SDHCI_HOST *Host, IN UINTN Reg) {
  return MmioRead8((UINTN)Host->ioaddr + Reg);
}

CONST CHAR16* MmcHostname(IN EFI_HANDLE ControllerHandle) {
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_STATUS Status;

  Status = gBS->HandleProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath);
  if (EFI_ERROR(Status) || DevicePath == NULL) {
    return L"Unknown MMC Host";
  }

  return ConvertDevicePathToText(DevicePath, FALSE, FALSE);
}

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