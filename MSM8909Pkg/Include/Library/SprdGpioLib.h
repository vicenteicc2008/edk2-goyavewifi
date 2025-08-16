#ifndef __SPRD_GPIO_LIB_H__
#define __SPRD_GPIO_LIB_H__

#include <Uefi.h>

VOID
SprdGpioSetDirection (
  IN UINTN GpioNum,
  IN BOOLEAN Output
  );

VOID
SprdGpioWrite (
  IN UINTN GpioNum,
  IN BOOLEAN Value
  );

BOOLEAN
SprdGpioRead (
  IN UINTN GpioNum
  );

#endif
