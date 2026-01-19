#ifndef _MMCHS_H_
#define _MMCHS_H_

#include <Uefi.h>

#include <Library/LKEnvLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "sprd_sdhci.h"
#include <Chipset/mmc.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#define BIT_0               0x00000001
#define BIT_1               0x00000002
#define BIT_2               0x00000004
#define BIT_3               0x00000008
#define BIT_4               0x00000010
#define BIT_5               0x00000020
#define BIT_6               0x00000040
#define BIT_7               0x00000080
#define BIT_8               0x00000100
#define BIT_9               0x00000200
#define BIT_10              0x00000400
#define BIT_11              0x00000800
#define BIT_12              0x00001000
#define BIT_13              0x00002000
#define BIT_14              0x00004000
#define BIT_15              0x00008000
#define BIT_16              0x00010000
#define BIT_17              0x00020000
#define BIT_18              0x00040000
#define BIT_19              0x00080000
#define BIT_20              0x00100000
#define BIT_21              0x00200000
#define BIT_22              0x00400000
#define BIT_23              0x00800000
#define BIT_24              0x01000000
#define BIT_25              0x02000000
#define BIT_26              0x04000000
#define BIT_27              0x08000000
#define BIT_28              0x10000000
#define BIT_29              0x20000000
#define BIT_30              0x40000000
#define BIT_31              0x80000000
#define EMMC 0
#define SPRD_EMMC_BASE 0xF511C000
#define REG_AP_CLK_EMMC_CFG (0x71200000 + 0x0054)
#define REG_AP_APB_APB_EB 0x71300000
#define BIT_AP_APB_EMMC_EB BIT_11
#define REG_AP_APB_APB_RST (0x71300000 + 0x0004)
#define BIT_AP_APB_EMMC_SOFT_RST                            BIT_14
#define REG_AON_APB_CGM_CLK_TOP_REG1 (0x402D0000 + 0x00C0)
#define BIT_EMMC_SLOT_SEL                          BIT_17
//#include "mmc_p.h"

//
// Device structures
//
typedef struct {
  VENDOR_DEVICE_PATH Mmc;
  EFI_DEVICE_PATH    End;
} MMCHS_DEVICE_PATH;

typedef struct {
  UINT32                Signature;
  EFI_HANDLE            Handle;
  EFI_BLOCK_IO_PROTOCOL BlockIo;
  EFI_BLOCK_IO_MEDIA    BlockMedia;
  MMCHS_DEVICE_PATH     DevicePath;
  struct mmc *          MmcDev;
  EFI_EVENT             ExitBsEvent;
} BIO_INSTANCE;
extern BIO_INSTANCE *Instance;
#define BIO_INSTANCE_SIGNATURE SIGNATURE_32('e', 'm', 'm', 'c')

#define BIO_INSTANCE_FROM_BLOCKIO_THIS(a)                                      \
  CR(a, BIO_INSTANCE, BlockIo, BIO_INSTANCE_SIGNATURE)

#ifdef DEBUG_SDHCI
#define DBG(...) dprintf(ALWAYS, __VA_ARGS__)
#else
#define DBG(...)
#endif

//
// Function Prototypes
//

EFI_STATUS
EFIAPI
MMCHSReset(IN EFI_BLOCK_IO_PROTOCOL *This, IN BOOLEAN ExtendedVerification);

EFI_STATUS
EFIAPI
MMCHSReadBlocks(
    IN EFI_BLOCK_IO_PROTOCOL *This, IN UINT32 MediaId, IN EFI_LBA Lba,
    IN UINTN BufferSize, OUT VOID *Buffer);

EFI_STATUS
EFIAPI
MMCHSWriteBlocks(
    IN EFI_BLOCK_IO_PROTOCOL *This, IN UINT32 MediaId, IN EFI_LBA Lba,
    IN UINTN BufferSize, IN VOID *Buffer);

EFI_STATUS
EFIAPI
MMCHSFlushBlocks(IN EFI_BLOCK_IO_PROTOCOL *This);
struct mmc *
PlatformCallbackInitSlot(struct mmc *mmc);
EFI_STATUS
BioInstanceContructor(OUT BIO_INSTANCE **NewInstance);

VOID EFIAPI MMCHSExitBsUninit(IN EFI_EVENT Event, IN VOID *Context);
#define CPOOL_HEAD_SIGNATURE   SIGNATURE_32('C','p','h','d')
VOID *memalign(UINTN Boundary, UINTN Size);
VOID *malloc(UINTN Size);
VOID *calloc(UINTN Count, UINTN Size);
VOID free(VOID *Ptr);
/* API: to initialize the controller */
int sdhci_init(struct mmc *mmc);
/* API: Send the command & transfer data using adma */
static int sdhci_send_command(struct mmc *mmc, struct mmc_cmd *cmd,
	struct mmc_data *data);
/* API: Set the bus width for the contoller */
uint8_t sdhci_set_bus_width(struct sdhci_host *, uint16_t);
/* API: Clock supply for the controller */
uint32_t sdhci_clk_supply(struct sdhci_host *, uint32_t);
/* API: To enable SDR/DDR mode */
void sdhci_set_uhs_mode(struct sdhci_host *, uint32_t);
/* API: Soft reset for the controller */
static void sdhci_reset(struct sdhci_host *host, u8 mask);

int mmc_read_blocks(struct mmc *mmc, void *dst, lbaint_t start,
			   lbaint_t blkcnt);
ulong mmc_write_blocks(struct mmc *mmc,
	lbaint_t start, lbaint_t blkcnt, const void *src);
/*
 * APIS exposed to block level driver
 */
/* API: Initialize the mmc card */
int mmc_init(struct mmc *mmc);
/* API: Read required number of blocks from card into destination */
uint32_t mmc_sdhci_read(
    struct mmc *dev, void *dest, uint64_t blk_addr, uint32_t num_blocks);
/* API: Write requried number of blocks from source to card */
uint32_t mmc_sdhci_write(
    struct mmc *dev, void *src, uint64_t blk_addr, uint32_t num_blocks);
    
/* API: Erase len bytes (after converting to number of erase groups), from
 * specified address */
uint32_t
mmc_sdhci_erase(struct mmc *dev, uint32_t blk_addr, uint64_t len);
/* API: Write protect or release len bytes (after converting to number of write
 * protect groups) from specified start address*/
uint32_t mmc_set_clr_power_on_wp_user(
    struct mmc *dev, uint32_t addr, uint64_t len, uint8_t set_clr);
/* API: Get the WP status of write protect groups starting at addr */
uint32_t
mmc_get_wp_status(struct mmc *dev, uint32_t addr, uint8_t *wp_status);
/* API: Put the mmc card in sleep mode */
void mmc_put_card_to_sleep(struct mmc *dev);
int sprd_host_init(int sdio_type);
ulong mmc_bread(struct mmc *mmc, lbaint_t start, lbaint_t blkcnt, void *dst);
int sdhci_init(struct mmc *mmc);
int mmc_start_init(struct mmc *mmc);
ulong mmc_bwrite(struct mmc *mmc, lbaint_t start, lbaint_t blkcnt, const void *src);
#endif