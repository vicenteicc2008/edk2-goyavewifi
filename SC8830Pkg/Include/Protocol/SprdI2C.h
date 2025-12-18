#ifndef __SPRD_I2C_H__
#define __SPRD_I2C_H__

#define SPRD_I2C_GUID                                             \
  {                                                                            \
    0x1c17b697, 0xfc09, 0x4c94,                                                \
    {                                                                          \
      0x8d, 0x2d, 0x44, 0x5b, 0xd8, 0x74, 0x40, 0x5c                           \
    }                                                                          \
  }

typedef struct _SPRD_I2C SPRD_I2C;

extern EFI_GUID  gSprdI2cProtocolGuid;

#endif
