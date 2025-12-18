#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#define GPIO_BASE       0xF5220000
#define GPIO_BANK_SIZE  0x100
#define GPIO_OUT        0x00
#define GPIO_OE         0x04
#define GPIO_IN         0x08

#define GPIO_PER_BANK  32

VOID
SprdGpioSetDirection (
  IN UINTN GpioNum,
  IN BOOLEAN Output
  )
{
  UINTN Bank = GpioNum / 32;
  UINTN Bit = GpioNum % 32;
  UINTN Addr = GPIO_BASE + Bank * GPIO_BANK_SIZE + GPIO_OE;

  UINT32 Val = MmioRead32(Addr);
  if (Output)
    Val &= ~(1 << Bit); // 0 = output
  else
    Val |= (1 << Bit);  // 1 = input
  MmioWrite32(Addr, Val);
}

VOID
SprdGpioWrite (
  IN UINTN GpioNum,
  IN BOOLEAN Value
  )
{
  UINTN Bank = GpioNum / 32;
  UINTN Bit = GpioNum % 32;
  UINTN Addr = GPIO_BASE + Bank * GPIO_BANK_SIZE + GPIO_OUT;

  UINT32 Val = MmioRead32(Addr);
  if (Value)
    Val |= (1 << Bit);
  else
    Val &= ~(1 << Bit);
  MmioWrite32(Addr, Val);
}

BOOLEAN
SprdGpioRead (
  IN UINTN GpioNum
  )
{
  UINTN Bank = GpioNum / 32;
  UINTN Bit = GpioNum % 32;
  UINTN Addr = GPIO_BASE + Bank * GPIO_BANK_SIZE + GPIO_IN;

  UINT32 Val = MmioRead32(Addr);
  return (Val & (1 << Bit)) != 0;
}
