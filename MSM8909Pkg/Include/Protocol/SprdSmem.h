#ifndef __SPRD_PROTOCOL_SMEM_H__
#define __SPRD_PROTOCOL_SMEM_H__

#define SPRD_SMEM_PROTOCOL_GUID                                                \
  {                                                                            \
    0x9c726a9b, 0xaa5f, 0x4067,                                                \
    {                                                                          \
      0xa6, 0x06, 0x9d, 0x01, 0x61, 0x66, 0xee, 0x12                           \
    }                                                                          \
  }

typedef struct _SPRD_SMEM_PROTOCOL SPRD_SMEM_PROTOCOL;

extern EFI_GUID gSprdSmemProtocolGuid;

#endif