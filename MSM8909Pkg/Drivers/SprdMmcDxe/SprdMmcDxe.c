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

#define SDHCI_ADMA_ERROR	0x54
#define SDHCI_ADMA_ADDRESS	0x58

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

static inline UINT32 SdhciReadl(IN SDHCI_HOST *Host, INTN Reg)
{
	return MmioRead32(Host->ioaddr + Reg);
}

STATIC VOID SdhciSetCardDetection(IN SDHCI_HOST *Host, BOOLEAN Enable)
{
	UINT32 Present, Irqs;

	if ((Host->Quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION) ||
	    (Host->Mmc->Caps & MMC_CAP_NONREMOVABLE))
		return;

	Present = SdhciReadl(Host, SDHCI_PRESENT_STATE) &
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

STATIC VOID SdhciReset(IN SDHCI_HOST *Host, IN UINT8 Mask) {
  UINT32 Timeout;
  UINT32 Ier = 0;

  if (Host->Quirks & SDHCI_QUIRK_NO_CARD_NO_RESET) {
    if (!(SdhciReadl(Host, SDHCI_PRESENT_STATE) & SDHCI_CARD_PRESENT)) {
      return;
    }
  }

  if (Host->Quirks & SDHCI_QUIRK_RESTORE_IRQS_AFTER_RESET) {
    Ier = SdhciReadl(Host, SDHCI_INT_ENABLE);
  }

  if (Host->Ops->PlatformResetEnter) {
    Host->Ops->PlatformResetEnter(Host, Mask);
  }

  SdhciWriteb(Host, Mask | 0x08, SDHCI_SOFTWARE_RESET);

  if (Mask & SDHCI_RESET_ALL) {
    Host->clock = 0;
  }

  /* Esperar un máximo de 100 ms */
  Timeout = 100;

  /* El hardware limpia el bit cuando termina */
  while (SdhciReadb(Host, SDHCI_SOFTWARE_RESET) & Mask) {
    if (Timeout == 0) {
      DEBUG((EFI_D_ERROR, "%a: Reset 0x%x never completed.\n",
             MmcHostname(Host->Mmc), (INT32)Mask));
      SdhciDumpRegs(Host);
      return;
    }
    Timeout--;
    gBS->Stall(1000); // Pausa de 1 ms
  }

  if (Host->Ops->PlatformResetExit) {
    Host->Ops->PlatformResetExit(Host, Mask);
  }

  if (Host->Quirks & SDHCI_QUIRK_RESTORE_IRQS_AFTER_RESET) {
    SdhciClearSetIrqs(Host, SDHCI_INT_ALL_MASK, Ier);
  }

  if (Host->flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA)) {
    if ((Host->Ops->EnableDma) && (Mask & SDHCI_RESET_ALL)) {
      Host->Ops->EnableDma(Host);
    }
  }
}

STATIC VOID sdhci_set_ios(struct MMC_HOST *mmc, struct mmc_ios *ios);

STATIC
VOID
SdhciInit (
  IN SDHCI_HOST *Host,
  INTN Soft
)
{
	if (Soft)
		SdhciReset(Host, SDHCI_RESET_CMD|SDHCI_RESET_DATA);
	else
		SdhciReset(Host, SDHCI_RESET_ALL);
	SdhciClearSetIrqs(Host, SDHCI_INT_ALL_MASK,
		SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
		SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
		SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
		SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE);
	
	if (Soft) {
		/* force clock reconfiguration */
		Host->clock = 0;
		// sdhci_set_ios(Host->mmc, &Host->mmc->ios);
	}
	
}

STATIC VOID SdhciActivateLed(IN SDHCI_HOST *Host)
{
	UINT8 Ctrl;

	Ctrl = SdhciReadb(Host, SDHCI_HOST_CONTROL);
	Ctrl |= SDHCI_CTRL_LED;
	SdhciWriteb(Host, Ctrl, SDHCI_HOST_CONTROL);
}

STATIC VOID SdhciDeactivateLed(IN SDHCI_HOST *Host)
{
	UINT8 Ctrl;

	Ctrl = SdhciReadb(Host, SDHCI_HOST_CONTROL);
	Ctrl &= ~SDHCI_CTRL_LED;
	SdhciWriteb(Host, Ctrl, SDHCI_HOST_CONTROL);
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