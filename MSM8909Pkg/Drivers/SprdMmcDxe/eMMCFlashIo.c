#include "mmc.h"

#define SPRD_GPT_FIRST_USEABLE_LBA     34
#define SPRD_GPT_ENTRY_MAX_NUM         128

EFI_BLOCK_IO_MEDIA gFsSdMmc2 = {
  SIGNATURE_32('s','p','f','s'),            // MediaId
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

