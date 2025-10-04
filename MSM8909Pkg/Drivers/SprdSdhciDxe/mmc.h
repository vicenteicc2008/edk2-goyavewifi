#ifndef __MMC_DEF__
#define __MMC_DEF__

#include <Uefi.h>
#include <Uefi/UefiGpt.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DmaLib.h>

#include <Protocol/ComponentName.h>
#include <Protocol/Cpu.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

#include <Protocol/SprdClock.h>



#define CONFIG_GENERIC_MMC

#define __be32_to_cpu(x)  ((0x000000ff&((x)>>24)) | (0x0000ff00&((x)>>8)) |       \
               (0x00ff0000&((x)<< 8)) | (0xff000000&((x)<<24)))

#define readl(addr)         (*((volatile UINT32 *)(addr)))          /* word input */
#define writel(value,addr)  (*((volatile UINT32 *)(addr))  = (value))   /* word output */

#define  SDHCI_SPACE_AVAILABLE	0x00000400
#define  SDHCI_DATA_AVAILABLE	0x00000800

#define SDHCI_TIMEOUT_DIVIDE_VALUE	3

#define SPRD_SDHCI_HOST_DEFAULT_CLOCK 26000000
#define SDHCI_FIX_PRE_COUNT			  15

#define REGULATOR_EVENT_ENABLE 		0x00

#define EMMC_BOOT_START_BLOCK (34)
#define SD_BOOT_START_BLOCK (16)

#define SDHCI_TIMEOUT_CONTROL	0x2E

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
#define  SDHCI_CTRL_4BITBUS	0x02
#define  SDHCI_CTRL_HISPD	0x04
#define  SDHCI_CTRL_DMA_MASK	0x18
#define   SDHCI_CTRL_SDMA	0x00
#define   SDHCI_CTRL_ADMA1	0x08
#define   SDHCI_CTRL_ADMA32	0x10
#define   SDHCI_CTRL_ADMA64	0x18
#define   SDHCI_CTRL_8BITBUS	0x20

#define SDHCI_RESPONSE		0x10

#define SDHCI_BUFFER  0x20

typedef enum {
  UNKNOWN_CARD,
  MMC_CARD,                        // MMC Card
  SD_CARD,                         // SD 1.1 Card
  SD_CARD_2,                       // SD 2.0 or Above Standard Card
  SD_CARD_2_HIGH,                  // SD 2.0 or Above High Capacity Card
  SD_CARD_MAX
} CARD_TYPE;

typedef struct {
  UINTN     BlockSize;
  UINTN     NumBlocks;
  UINTN     TotalNumBlocks;
  UINTN     ClockFrequencySelect;
} CARD_INFO;

struct mmc_data {
	unsigned int		TimeoutNs;	/* data timeout (in ns, max 80ms) */
	unsigned int		TimeoutClks;	/* data timeout (in clocks) */
	unsigned int		Blksz;		/* data block size */
	unsigned int		Blocks;		/* number of blocks */
	unsigned int		Error;		/* data error */
	unsigned int		Flags;

	UINT32			Length;		/* length of the mapped area */

#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
#define MMC_DATA_STREAM	(1 << 10)

	unsigned int		BytesXfered;

	struct mmc_command	*Stop;		/* stop command */
	struct mmc_request	*Mrq;		/* associated request */

	INT32			HostCookie;	/* host private data */
};

typedef struct {
	EFI_BLOCK_IO_PROTOCOL BlockIo;
    EFI_BLOCK_IO_MEDIA    Media;
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
	unsigned int		ActualClock;	/* Actual HC clock rate */
	unsigned int		SlotNo;			/* used for sdio acpi binding */
	unsigned long		Private[0];
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
	UINTN Version;
	UINTN max_clk;	/* Max possible freq (MHz) */
	UINTN timeout_clk;	/* Timeout freq (KHz) */
	UINTN clk_mul;	/* Clock Muliplier value */
	UINTN clock;
	UINT8 pwr;
	BOOLEAN RuntimeSuspended;	/* Host is runtime suspended */
	UINTN BaseAddress;
	unsigned int		Blocks;		/* number of blocks */
	struct MMC_HOST *Mmc;
	const struct SDHCI_OPS *Ops;
	struct mmc_data		*Data;
	unsigned int DataEarly:1;
	struct mmc_request	*Mrq;		/* associated request */
	
} SDHCI_HOST;

#define Mmiowb()

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

STATIC inline UINT8 SdhciReadb(IN SDHCI_HOST *Host, IN UINTN Reg) {
  return MmioRead8((UINTN)Host->ioaddr + Reg);
}

STATIC inline VOID SdhciWriteb(IN SDHCI_HOST *Host, IN UINT8 Val, IN UINTN Reg) {
  MmioWrite8((UINTN)Host->ioaddr + Reg, Val);
}

STATIC inline UINT8 SdhciReadw(IN SDHCI_HOST *Host, IN UINTN Reg) {
  return MmioRead16((UINTN)Host->ioaddr + Reg);
}

STATIC inline VOID SdhciWritew(IN SDHCI_HOST *Host, IN UINT16 Val, IN UINTN Reg)
{
  MmioWrite16((UINTN)Host->ioaddr + Reg, Val);
}

STATIC inline VOID SdhciWritel(IN SDHCI_HOST *Host, IN UINT8 Val, IN UINTN Reg) {
  MmioWrite32((UINTN)Host->ioaddr + Reg, Val);
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

#define MMC_SET_BLOCKLEN         16   /* ac   [31:0] block len   R1  */
#define MMC_READ_SINGLE_BLOCK    17   /* adtc [31:0] data addr   R1  */
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */
#define MMC_SEND_TUNING_BLOCK    19   /* adtc                    R1  */
#define MMC_SEND_TUNING_BLOCK_HS200	21	/* adtc R1  */
#define MMC_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0] data addr   R1  */
#define MMC_SET_BLOCK_COUNT      23   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_BLOCK          24   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */
#define MMC_PROGRAM_CID          26   /* adtc                    R1  */
#define MMC_PROGRAM_CSD          27   /* adtc                    R1  */

STATIC inline BOOLEAN MmcOpMulti(UINT32 opcode)
{
	return opcode == MMC_WRITE_MULTIPLE_BLOCK ||
	       opcode == MMC_READ_MULTIPLE_BLOCK;
}

typedef struct  {
	UINT32			Opcode;
	UINT32			Arg;
#define MMC_CMD23_ARG_REL_WR	(1 << 31)
#define MMC_CMD23_ARG_PACKED	((0 << 31) | (1 << 30))
#define MMC_CMD23_ARG_TAG_REQ	(1 << 29)
	UINT32			resp[4];
	unsigned int		Flags;
#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_CMD_MASK	(3 << 5)		/* non-SPI command type */
#define MMC_CMD_AC	(0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC	(2 << 5)
#define MMC_CMD_BCR	(3 << 5)

#define MMC_RSP_SPI_S1	(1 << 7)		/* one status byte */
#define MMC_RSP_SPI_S2	(1 << 8)		/* second byte */
#define MMC_RSP_SPI_B4	(1 << 9)		/* four data bytes */
#define MMC_RSP_SPI_BUSY (1 << 10)		/* card may send busy */

/*
 * These are the native response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define mmc_resp_type(Cmd)	((Cmd)->Flags & (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC|MMC_RSP_BUSY|MMC_RSP_OPCODE))

/*
 * These are the SPI response types for MMC, SD, and SDIO cards.
 * Commands return R1, with maybe more info.  Zero is an error type;
 * callers must always provide the appropriate MMC_RSP_SPI_Rx flags.
 */
#define MMC_RSP_SPI_R1	(MMC_RSP_SPI_S1)
#define MMC_RSP_SPI_R1B	(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY)
#define MMC_RSP_SPI_R2	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R3	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R4	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)
#define MMC_RSP_SPI_R5	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
#define MMC_RSP_SPI_R7	(MMC_RSP_SPI_S1|MMC_RSP_SPI_B4)

#define mmc_spi_resp_type(Cmd)	((Cmd)->Flags & \
		(MMC_RSP_SPI_S1|MMC_RSP_SPI_BUSY|MMC_RSP_SPI_S2|MMC_RSP_SPI_B4))

/*
 * These are the command types.
 */
#define mmc_cmd_type(Cmd)	((Cmd)->Flags & MMC_CMD_MASK)

	unsigned int		Retries;	/* max number of retries */
	unsigned int		Error;		/* command error */

/*
 * Standard errno values are used for errors, but some have specific
 * meaning in the MMC layer:
 *
 * ETIMEDOUT    Card took too long to respond
 * EILSEQ       Basic format problem with the received or sent data
 *              (e.g. CRC check failed, incorrect opcode in response
 *              or bad end bit)
 * EINVAL       Request cannot be performed because of restrictions
 *              in hardware and/or the driver
 * ENOMEDIUM    Host can determine that the slot is empty and is
 *              actively failing requests
 */

	unsigned int		CmdTimeoutMs;	/* in milliseconds */

	struct mmc_data		*Data;		/* data segment associated with cmd */
	struct mmc_request	*Mrq;		/* associated request */
} MMC_COMMAND;

typedef enum {
  READ,
  WRITE
} OPERATION_TYPE;

typedef struct {
  VENDOR_DEVICE_PATH  Mmc;
  EFI_DEVICE_PATH     End;
} SDHCI_DEVICE_PATH;

struct mmc_request {
	IN MMC_COMMAND	*Sbc;		/* SET_BLOCK_COUNT for multiblock */
	IN MMC_COMMAND	*Cmd;
	IN MMC_COMMAND	*Stop;

	struct mmc_data		*Data;
	VOID			(*done)(struct mmc_request *);/* completion function */
	struct mmc_host		*Host;
};

#define   SDHCI_ACMD12_ERR	0x3C

#define   SDHCI_HOST_CONTROL2			0x3E
#define   SDHCI_CTRL_UHS_MASK			0x0007
#define   SDHCI_CTRL_UHS_SDR12			0x0000
#define   SDHCI_CTRL_UHS_SDR25			0x0001
#define   SDHCI_CTRL_UHS_SDR50			0x0002
#define   SDHCI_CTRL_UHS_SDR104			0x0003
#define   SDHCI_CTRL_UHS_DDR50			0x0004
#define   SDHCI_CTRL_HS_SDR200			0x0005 /* reserved value in SDIO spec */
#define   SDHCI_CTRL_VDD_180			0x0008
#define   SDHCI_CTRL_DRV_TYPE_MASK		0x0030
#define   SDHCI_CTRL_DRV_TYPE_B			0x0000
#define   SDHCI_CTRL_DRV_TYPE_A		    0x0010
#define   SDHCI_CTRL_DRV_TYPE_C		    0x0020
#define   SDHCI_CTRL_DRV_TYPE_D		    0x0030
#define   SDHCI_CTRL_EXEC_TUNING		0x0040
#define   SDHCI_CTRL_TUNED_CLK		    0x0080
#define   SDHCI_CTRL_PRESET_VAL_ENABLE	0x8000

#define   SDHCI_CAPABILITIES	    0x40
#define   SDHCI_TIMEOUT_CLK_MASK	0x0000003F
#define   SDHCI_TIMEOUT_CLK_SHIFT   0
#define   SDHCI_TIMEOUT_CLK_UNIT	0x00000080
#define   SDHCI_CLOCK_BASE_MASK		0x00003F00
#define   SDHCI_CLOCK_V3_BASE_MASK	0x0000FF00
#define   SDHCI_CLOCK_BASE_SHIFT	8
#define   SDHCI_MAX_BLOCK_MASK		0x00030000
#define   SDHCI_MAX_BLOCK_SHIFT  	16
#define   SDHCI_CAN_DO_8BIT			0x00040000
#define   SDHCI_CAN_DO_ADMA2		0x00080000
#define   SDHCI_CAN_DO_ADMA1		0x00100000
#define   SDHCI_CAN_DO_HISPD		0x00200000
#define   SDHCI_CAN_DO_SDMA			0x00400000
#define   SDHCI_CAN_VDD_330			0x01000000
#define   SDHCI_CAN_VDD_300			0x02000000
#define   SDHCI_CAN_VDD_180			0x04000000
#define   SDHCI_CAN_64BIT			0x10000000

#define  SDHCI_SUPPORT_SDR50	0x00000001
#define  SDHCI_SUPPORT_SDR104	0x00000002
#define  SDHCI_SUPPORT_DDR50	0x00000004
#define  SDHCI_DRIVER_TYPE_A	0x00000010
#define  SDHCI_DRIVER_TYPE_C	0x00000020
#define  SDHCI_DRIVER_TYPE_D	0x00000040
#define  SDHCI_RETUNING_TIMER_COUNT_MASK	0x00000F00
#define  SDHCI_RETUNING_TIMER_COUNT_SHIFT	8
#define  SDHCI_USE_SDR50_TUNING			0x00002000
#define  SDHCI_RETUNING_MODE_MASK		0x0000C000
#define  SDHCI_RETUNING_MODE_SHIFT		14
#define  SDHCI_CLOCK_MUL_MASK	0x00FF0000
#define  SDHCI_CLOCK_MUL_SHIFT	16

#define SDHCI_CAPABILITIES_1	0x44

#define  SDHCI_MAX_CURRENT				0x48
#define  SDHCI_MAX_CURRENT_LIMIT		0xFF
#define  SDHCI_MAX_CURRENT_330_MASK		0x0000FF
#define  SDHCI_MAX_CURRENT_330_SHIFT	0
#define  SDHCI_MAX_CURRENT_300_MASK		0x00FF00
#define  SDHCI_MAX_CURRENT_300_SHIFT	8
#define  SDHCI_MAX_CURRENT_180_MASK		0xFF0000
#define  SDHCI_MAX_CURRENT_180_SHIFT	16
#define  SDHCI_MAX_CURRENT_MULTIPLIER	4

/* 4C-4F reserved for more max current */

#define SDHCI_SET_ACMD12_ERROR	0x50
#define SDHCI_SET_INT_ERROR		0x52

#define SDHCI_ADMA_ERROR	0x54

/* 55-57 reserved */

#define SDHCI_ADMA_ADDRESS	0x58

/* 60-FB reserved */

#define SDHCI_PRESET_FOR_SDR12 0x66
#define SDHCI_PRESET_FOR_SDR25 0x68
#define SDHCI_PRESET_FOR_SDR50 0x6A
#define SDHCI_PRESET_FOR_SDR104        0x6C
#define SDHCI_PRESET_FOR_DDR50 0x6E
#define SDHCI_PRESET_DRV_MASK  0xC000
#define SDHCI_PRESET_DRV_SHIFT  14
#define SDHCI_PRESET_CLKGEN_SEL_MASK   0x400
#define SDHCI_PRESET_CLKGEN_SEL_SHIFT	10
#define SDHCI_PRESET_SDCLK_FREQ_MASK   0x3FF
#define SDHCI_PRESET_SDCLK_FREQ_SHIFT	0

#define SDHCI_SLOT_INT_STATUS	0xFC

#define   SDHCI_HOST_VERSION		0xFE
#define   SDHCI_VENDOR_VER_MASK		0xFF00
#define   SDHCI_VENDOR_VER_SHIFT	8
#define   SDHCI_SPEC_VER_MASK		0x00FF
#define   SDHCI_SPEC_VER_SHIFT		0
#define   SDHCI_SPEC_100			0
#define   SDHCI_SPEC_200			1
#define   SDHCI_SPEC_300			2

/*
 * End of controller registers.
 */

#define SDHCI_MAX_DIV_SPEC_200	256
#define SDHCI_MAX_DIV_SPEC_300	2046



#define SDHCI_MAX_DIV_SPEC_200	256
#define SDHCI_MAX_DIV_SPEC_300	2046

#define SDHCI_DEFAULT_BOUNDARY_SIZE  (512 * 1024)
#define SDHCI_DEFAULT_BOUNDARY_ARG   ((SDHCI_DEFAULT_BOUNDARY_SIZE) - 12)

#define SDHCI_BLOCK_SIZE	0x04
#define SDHCI_MAKE_BLKSZ(Dma, Blksz) (((Dma & 0x7) << 12) | (Blksz & 0xFFF))

#define SDHCI_BLOCK_COUNT	0x06

#define SDHCI_ARGUMENT		0x08

#define SDHCI_TRANSFER_MODE	0x0C
#define  SDHCI_TRNS_DMA		0x01
#define  SDHCI_TRNS_BLK_CNT_EN	0x02
#define  SDHCI_TRNS_AUTO_CMD12	0x04
#define  SDHCI_TRNS_AUTO_CMD23	0x08
#define  SDHCI_TRNS_READ	0x10
#define  SDHCI_TRNS_MULTI	0x20

#define MAX_MMC_NUM     3

#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
#define MMC_DATA_STREAM	(1 << 10)

#define  SDHCI_COMMAND		0x0E
#define  SDHCI_CMD_RESP_MASK	0x03
#define  SDHCI_CMD_CRC		0x08
#define  SDHCI_CMD_INDEX	0x10
#define  SDHCI_CMD_DATA		0x20
#define  SDHCI_CMD_ABORTCMD	0xC0

#define  SDHCI_CMD_RESP_NONE	0x00
#define  SDHCI_CMD_RESP_LONG	0x01
#define  SDHCI_CMD_RESP_SHORT	0x02
#define  SDHCI_CMD_RESP_SHORT_BUSY 0x03

#define SDHCI_MAKE_CMD(c, f) (((c & 0xff) << 8) | (f & 0xff))
#define SDHCI_GET_CMD(c) ((c>>8) & 0x3f)

#define SDHCI_PRESENT_STATE	0x24
#define SDHCI_CMD_INHIBIT	0x00000001

STATIC inline VOID *MmcPriv(IN MMC_HOST *Host)
{
	return (VOID *)Host->Private;
}

#define  SDHCI_CLOCK_CARD_EN	0x0004
#define  SDHCI_CLOCK_INT_STABLE	0x0002
#define  SDHCI_CLOCK_INT_EN		0x0001

//
//  Power Enable Register
//
#define POWER_ENABLE             (0x1)



#define mdelay(ms) MicroSecondDelay((ms)*1000)

#endif