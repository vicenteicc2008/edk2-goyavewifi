#ifndef __SPRD_GPIO_H__
#define __SPRD_GPIO_H__

#define SPRD_GPIO_GUID                                             \
  {                                                                            \
    0xfac1b8a5, 0x8ce6, 0x4cbc,                                                \
    {                                                                          \
      0xad, 0x09, 0xa3, 0xb8, 0x2a, 0x82, 0x45, 0xe9                           \
    }                                                                          \
  }

#define GPIO_PULL_NONE   0
#define GPIO_PULL_DOWN   1
#define GPIO_PULL_UP     3

#define GPIO_DRV_FAST    0
#define GPIO_DRV_SLOW    1

#define GPIO_INPUT       0
#define GPIO_OUTPUT      1

#define GPIO_INVALID_ID 0xffff
#define INVALID_REG		(~(UINT32)0)

//
// Protocol interface structure
//
typedef struct _SPRD_GPIO SPRD_GPIO;

typedef
UINT32
(*GPIO_GET)(
  UINT32 gpioNumber
  );

struct _SPRD_GPIO {
  GPIO_GET         Get;
};

extern EFI_GUID  gSprdGpioProtocolGuid;

#endif