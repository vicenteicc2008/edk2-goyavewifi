#include "mmc.h"

#define MAX_TUNING_LOOP 40

#define SDHCI_USE_LEDS_CLASS

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

#define SDHCI_INT_CARD_INSERT	0x00000040
#define SDHCI_INT_CARD_REMOVE	0x00000080
#define MMC_CAP_NONREMOVABLE	(1 << 8)	/* Nonremovable e.g. eMMC */
#define MMC_CAP_WAIT_WHILE_BUSY	(1 << 9)	/* Waits while card is busy */
#define MMC_CAP_ERASE		(1 << 10)	/* Allow erase/trim commands */
#define MMC_CAP_1_8V_DDR	(1 << 11)	/* can support */

STATIC UINTN DebugQuirks = 0;
STATIC UINTN DebugQuirks2;

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

struct MMC_HOST {
	INTN			index;
	UINTN		f_min;
	UINTN		f_max;
	UINTN		f_init;
	UINT32			ocr_avail;
	UINT32			ocr_avail_sdio;	/* SDIO-specific OCR */
	UINT32			ocr_avail_sd;	/* SD-specific OCR */
	UINT32			ocr_avail_mmc;	/* MMC-specific OCR */
	UINT32			max_current_330;
	UINT32			max_current_300;
	UINT32			max_current_180;
	UINT32			Caps;
};

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
} SDHCI_HOST;

STATIC VOID SdhciDumpRegs(SDHCI_HOST *host)
{
    UINT32 i, regAddr;
    UINT32 val1, val2, val3, val4;

    DEBUG((EFI_D_INFO, "SprdSdhciDxe: =========== REGISTER DUMP ===========\n"));

    // Imprimir nombre del host si tienes un campo que lo contenga
    DEBUG((EFI_D_INFO, "SprdSdhciDxe: Host Address: 0x%p\n", host));

    regAddr = SDHCI_DMA_ADDRESS;

    // Volcar registros en bloques de 4
    for (i = 0; i < 0x08; i++) {
        val1 = MmioRead32(host->ioaddr + regAddr + 16 * i);
        val2 = MmioRead32(host->ioaddr + regAddr + 4 + 16 * i);
        val3 = MmioRead32(host->ioaddr + regAddr + 8 + 16 * i);
        val4 = MmioRead32(host->ioaddr + regAddr + 12 + 16 * i);

        DEBUG((EFI_D_ERROR, "SprdSdhciDxe: 0x%08x | 0x%08x | 0x%08x | 0x%08x\n",
                    val1, val2, val3, val4));
    }

    // Volcar registros adicionales
    DEBUG((EFI_D_ERROR, "SprdSdhciDxe: 0x%08x | 0x%08x | 0x%08x\n",
                MmioRead32(host->ioaddr + 0x80),
                MmioRead32(host->ioaddr + 0x84),
                MmioRead32(host->ioaddr + 0x88)));

    // Verificar si se usa ADMA
    if (host->flags & 0x01) {  // Asumimos que SDHCI_USE_ADMA está representado por 0x01
        DEBUG((EFI_D_ERROR, "SprdSdhciDxe: ADMA Err: 0x%08x | ADMA Ptr: 0x%08x\n",
                    MmioRead32(host->ioaddr + SDHCI_ADMA_ERROR),
                    MmioRead32(host->ioaddr + SDHCI_ADMA_ADDRESS)));
    }

    DEBUG((EFI_D_INFO, "SprdSdhciDxe: ==========================================\n"));
}

VOID SdhciClearSetIrqs (
    IN SDHCI_HOST *Host,
    IN UINT32 Clear,
    IN UINT32 Set
    ) 
{
    UINT32 Ier;

    Ier = MmioRead32 (Host->BaseAddress + SDHCI_INT_ENABLE);
    Ier &= ~Clear;
    Ier |= Set;
    MmioWrite32 (Host->BaseAddress + SDHCI_INT_ENABLE, Ier);
    MmioWrite32 (Host->BaseAddress + SDHCI_SIGNAL_ENABLE, Ier);
}

STATIC VOID SdhciUnmaskIrqs(IN SDHCI_HOST *Host, UINT32 Irqs)
{
	SdhciClearSetIrqs(Host, 0, Irqs);
}

STATIC VOID SdhciMaskIrqs(IN SDHCI_HOST *Host, UINT32 Irqs)
{
	SdhciClearSetIrqs(Host, Irqs, 0);
}

static inline UINT32 sdhci_readl(IN SDHCI_HOST *Host, INTN Reg)
{
	return MmioRead32(Host->ioaddr + Reg);
}

STATIC VOID SdhciSetCardDetection(IN SDHCI_HOST *Host, BOOLEAN Enable)
{
	UINT32 Present, Irqs;

	if ((Host->Quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION) ||
	    (Host->Mmc->Caps & MMC_CAP_NONREMOVABLE))
		return;

	Present = sdhci_readl(Host, SDHCI_PRESENT_STATE) &
			      SDHCI_CARD_PRESENT;
	Irqs = Present ? SDHCI_INT_CARD_REMOVE : SDHCI_INT_CARD_INSERT;

	if (Enable)
		SdhciUnmaskIrqs(Host, Irqs);
	else
		SdhciMaskIrqs(Host, Irqs);
}

STATIC VOID SdhciEnableCardDetection(IN SDHCI_HOST *Host)
{
	SdhciSetCardDetection(Host, TRUE);
}

STATIC VOID SdhciDisableCardDetection(IN SDHCI_HOST *Host)
{
	SdhciSetCardDetection(Host, FALSE);
}

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