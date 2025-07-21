#include "mmc.h"

#define SPRD_SDHCI_HOST_DEFAULT_CLOCK 26000000
#define SDHCI_FIX_PRE_COUNT			  15

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

STATIC VOID SdhciFinishData(IN SDHCI_HOST *);

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

// Low level functions

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

STATIC inline UINT32 SdhciReadl(IN SDHCI_HOST *Host, INTN Reg)
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

// Core functions

STATIC
VOID
SdhciReadBlockPio (IN SDHCI_HOST *Host)
{
	unsigned long Flags;
	UINTN Blksize, Len, Chunk;
	UINT32 Scratch;
	UINT8 *Buf;
	
	DEBUG((EFI_D_INFO, "PIO reading\n"));
	
	Blksize = Host->Data->Blksz;
	Chunk = 0;

	while (Blksize) {
		if (Chunk == 0) {
			Scratch = SdhciReadl(Host, SDHCI_BUFFER); // Read 4 bytes from FIFO
			Chunk = 4;
		}

		*Buf++ = (UINT8)(Scratch & 0xFF);
		Scratch >>= 8;
		Chunk--;
		Blksize--;
	}
}

STATIC
VOID
SdhciWriteBlockPio (IN SDHCI_HOST *Host)
{
	unsigned long Flags;
	UINTN Blksize, Len, Chunk;
	UINT32 Scratch;
	UINT8 *Buf;
	
	DEBUG((EFI_D_INFO, "PIO writing\n"));

	Blksize = Host->Data->Blksz;
	Chunk = 0;
	Scratch = 0;

	while (Blksize) {
			Scratch |= (UINT32)(*Buf++) << (Chunk * 8);
		Chunk++;
		Blksize--;

		if ((Chunk == 4) || (Blksize == 0)) {
			SdhciWritel(Host, Scratch, SDHCI_BUFFER); // Write 4 bytes to FIFO
			Chunk = 0;
			Scratch = 0;
		}
	}
}

STATIC
VOID
SdhciTransferPio (IN SDHCI_HOST *Host)
{
	UINT32 Mask;
	
	if (Host->Blocks == 0)
		return;

	if (Host->Data->Flags & MMC_DATA_READ)
		Mask = SDHCI_DATA_AVAILABLE;
	else
		Mask = SDHCI_SPACE_AVAILABLE;

	/*
	 * Some controllers (JMicron JMB38x) mess up the buffer bits
	 * for transfers < 4 bytes. As long as it is just one block,
	 * we can ignore the bits.
	 */
	if ((Host->Quirks & SDHCI_QUIRK_BROKEN_SMALL_PIO) &&
		(Host->Data->Blocks == 1))
		Mask = ~0;
	
	while (SdhciReadl(Host, SDHCI_PRESENT_STATE) & Mask) {
		if (Host->Quirks & SDHCI_QUIRK_PIO_NEEDS_DELAY)
			MicroSecondDelay(100);

		if (Host->Data->Flags & MMC_DATA_READ)
			SdhciReadBlockPio(Host);
		else
			SdhciWriteBlockPio(Host);

		Host->Blocks--;
		if (Host->Blocks == 0)
			break;
	}

	DEBUG((EFI_D_INFO, "PIO transfer complete.\n"));
}

STATIC
UINT8
SdhciCalcTimeout (IN SDHCI_HOST *Host, IN MMC_COMMAND *Cmd)
{
	UINT8 Count;
	struct mmc_data *Data = Cmd->Data;
	unsigned TargetTimeout, CurrentTimeout;
	
	/*
	 * If the host controller provides us with an incorrect timeout
	 * value, just skip the check and use 0xE.  The hardware may take
	 * longer to time out, but that's much better than having a too-short
	 * timeout value.
	 */
	if (Host->Quirks & SDHCI_QUIRK_BROKEN_TIMEOUT_VAL)
		return 0xE;
	
	/* Unspecified timeout, assume max */
	if (!Data && !Cmd->CmdTimeoutMs)
		return 0xE;

	/* timeout in us */
	if (!Data)
		TargetTimeout = Cmd->CmdTimeoutMs * 1000;
	else {
		TargetTimeout = Data->TimeoutNs / 1000;
		if (Host->clock)
			TargetTimeout += Data->TimeoutClks / Host->clock;
	}
	
	/*
	 * Figure out needed cycles.
	 * We do this in steps in order to fit inside a 32 bit int.
	 * The first step is the minimum timeout, which will have a
	 * minimum resolution of 6 bits:
	 * (1) 2^13*1000 > 2^22,
	 * (2) host->timeout_clk < 2^16
	 *     =>
	 *     (1) / (2) > 2^6
	 */
	Count = 0;
	CurrentTimeout = (1 << 13) * 1000 / Host->timeout_clk;
	while (CurrentTimeout < TargetTimeout) {
		Count++;
		CurrentTimeout <<= 1;
		if (Count >= 0xF)
			break;
	}
	
	if (Count >= 0xF) {
		DEBUG((EFI_D_WARN, "%s: Too large timeout 0x%x requested for CMD%d!\n",
		    MmcHostname(Host->Mmc), Count, Cmd->Opcode));
		Count = 0xE;
	}

	return Count;
}

STATIC
VOID
SdhciSetTransferIrqs (IN SDHCI_HOST *Host)
{
	UINT32 PioIrqs = SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL;
	UINT32 DmaIrqs = SDHCI_INT_DMA_END | SDHCI_INT_ADMA_ERROR;

	if (Host->flags & SDHCI_REQ_USE_DMA)
		SdhciClearSetIrqs(Host, PioIrqs, DmaIrqs);
	else
		SdhciClearSetIrqs(Host, DmaIrqs, PioIrqs);
}

STATIC
VOID
SdhciPrepareData (
  IN SDHCI_HOST *Host,
  IN MMC_COMMAND *Cmd
  )
{
  UINT8 Timeout, Ctrl;
  struct mmc_data *Data = Cmd->Data;
  EFI_STATUS Status;

  if (Data || (Cmd->Flags & MMC_RSP_BUSY)) {
    Timeout = SdhciCalcTimeout(Host, Cmd);
    SdhciWriteb(Host, Timeout, SDHCI_TIMEOUT_CONTROL);
  }

  if (!Data)
    return;

  Host->Data = Data;
  Host->DataEarly = 0;
  Host->Data->BytesXfered = 0;

  // Inicializar modo DMA si está disponible
  if (Host->flags & (SDHCI_USE_SDMA | SDHCI_USE_ADMA))
    Host->flags |= SDHCI_REQ_USE_DMA;

  // If DMA not used, mark blocks for manual PIO
  Host->Blocks = Data->Blocks;

  SdhciSetTransferIrqs(Host);

  // Block size Setiing
  SdhciWritew(Host,
    SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG, Data->Blksz),
    SDHCI_BLOCK_SIZE);
  SdhciWritew(Host, Data->Blocks, SDHCI_BLOCK_COUNT);
}

STATIC
VOID
SdhciSetTransferMode (
  IN SDHCI_HOST *Host,
  IN MMC_COMMAND *Cmd
  )
{
	UINT16 Mode;
    struct mmc_data *Data = Cmd->Data;

	if (Data == NULL)
		return;

	Mode = SDHCI_TRNS_BLK_CNT_EN;
	if (MmcOpMulti(Cmd->Opcode) || Data->Blocks > 1) {
		Mode |= SDHCI_TRNS_MULTI;
		/*
		 * If we are sending CMD23, CMD12 never gets sent
		 * on successful completion (so no Auto-CMD12).
		 */
		if (!Host->Mrq->Sbc && (Host->flags & SDHCI_AUTO_CMD12))
			Mode |= SDHCI_TRNS_AUTO_CMD12;
		else if (Host->Mrq->Sbc && (Host->flags & SDHCI_AUTO_CMD23)) {
			Mode |= SDHCI_TRNS_AUTO_CMD23;
			SdhciWritel(Host, Host->Mrq->Sbc->Arg, SDHCI_ARGUMENT2);
		}
	}

	if (Data->Flags & MMC_DATA_READ)
		Mode |= SDHCI_TRNS_READ;
	if (Host->flags & SDHCI_REQ_USE_DMA)
		Mode |= SDHCI_TRNS_DMA;

	SdhciWritew(Host, Mode, SDHCI_TRANSFER_MODE);
}

STATIC
VOID
SdhciSendCmd (
  IN SDHCI_HOST *Host,
  IN MMC_COMMAND *Cmd
  )
{
	int Flags;
	UINT32 Mask;
	unsigned long Timeout;

	Timeout = 10;

	Mask = SDHCI_CMD_INHIBIT;
	if ((Cmd->Data != NULL) || (Cmd->Flags & MMC_RSP_BUSY))
		Mask |= SDHCI_DATA_INHIBIT;

	if (Host->Mrq->Data && ((MMC_COMMAND *)Cmd == (MMC_COMMAND *)Host->Mrq->Data->Stop))
		Mask &= ~SDHCI_DATA_INHIBIT;

	while (SdhciReadl(Host, SDHCI_PRESENT_STATE) & Mask) {
		if (Timeout == 0) {
			DEBUG((EFI_D_ERROR, "%s: Controller never released "
				"inhibit bit(s).\n", MmcHostname(Host->Mmc)));
			SdhciDumpRegs(Host);
			Cmd->Error = -5;
			return;
		}
		Timeout--;
		mdelay(1);
	}

	if (!(Cmd->Flags & MMC_RSP_PRESENT))
		Flags = SDHCI_CMD_RESP_NONE;
	else if (Cmd->Flags & MMC_RSP_136)
		Flags = SDHCI_CMD_RESP_LONG;
	else if (Cmd->Flags & MMC_RSP_BUSY)
		Flags = SDHCI_CMD_RESP_SHORT_BUSY;
	else
		Flags = SDHCI_CMD_RESP_SHORT;

	if (Cmd->Flags & MMC_RSP_CRC)
		Flags |= SDHCI_CMD_CRC;
	if (Cmd->Flags & MMC_RSP_OPCODE)
		Flags |= SDHCI_CMD_INDEX;

	if (Cmd->Data || Cmd->Opcode == MMC_SEND_TUNING_BLOCK ||
	    Cmd->Opcode == MMC_SEND_TUNING_BLOCK_HS200)
		Flags |= SDHCI_CMD_DATA;

	SdhciWritew(Host, SDHCI_MAKE_CMD(Cmd->Opcode, Flags), SDHCI_COMMAND);
}



// EntryPoint for SprdSdhciDxe

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