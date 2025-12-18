/** @file
  Spreadtrum SC8830 Timer DXE Driver (corregido)
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Timer.h>
#include <Protocol/HardwareInterrupt.h>

#define TIMER_BASE        0xF5204000   // Timer 0 base (desde tu DTS)
#define TIMER_IRQ         118          // IRQ del timer en el DTS (0x76)

// Register offsets (DesignWare APB Timer)
#define TIMER_LOAD_COUNT  0x00
#define TIMER_CUR_VALUE   0x04
#define TIMER_CONTROL     0x08
#define TIMER_INT_CLR     0x0C
#define TIMER_INT_STATUS  0x10

// CONTROL register bits (valores típicos para DW timer)
#define TIMER_CTRL_ENABLE   (1 << 0)
#define TIMER_CTRL_MODE     (1 << 1)  // periodic mode (según implementación)
#define TIMER_CTRL_INT_EN   (1 << 2)

// Default frequency (ajustar si en tu SoC difiere)
#define TIMER_FREQUENCY  26000000UL  // 26 MHz (valor típico, confirma si tienes otra fuente)

STATIC EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterrupt = NULL;

/* Estado del driver */
STATIC volatile EFI_TIMER_NOTIFY mTimerNotifyFunction = (EFI_TIMER_NOTIFY)NULL;
STATIC volatile UINT64 mTimerPeriod = 0;

/* ---- helpers ---- */

/*
  Convierte TimerPeriod (unidades de 100ns) -> ticks del timer (UINT64),
  usando la fórmula: ticks = TimerPeriod * TIMER_FREQUENCY / 10_000_000
  Devuelve error si hay overflow o el resultado no cabe en 32 bits.
*/
STATIC
EFI_STATUS
SafeTimerPeriodToTicks (
  IN  UINT64  TimerPeriod100ns,
  OUT UINT64 *Ticks
  )
{
  UINT64 q;
  UINT64 r;
  UINT64 part1;
  UINT64 part2;

  if (Ticks == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  /* divisor = 10,000,000 (porque TimerPeriod*1e-7 s * freq = TimerPeriod*freq/1e7) */
  q = DivU64x32 (TimerPeriod100ns, 10000000U);
  r = TimerPeriod100ns - q * 10000000ULL;

  /* comprobar overflow en q * TIMER_FREQUENCY */
  if (q > DivU64x32 (~(UINT64)0, (UINT32)TIMER_FREQUENCY)) {
    return EFI_UNSUPPORTED;
  }

  part1 = q * (UINT64)TIMER_FREQUENCY;

  /* r < 10_000_000, r * TIMER_FREQUENCY < ~2.6e14 (seguro para 64-bit si TIMER_FREQUENCY ~26e6) */
  part2 = DivU64x32 (r * (UINT64)TIMER_FREQUENCY, 10000000U);

  *Ticks = part1 + part2;
  return EFI_SUCCESS;
}

/* ---- ISR y funciones del protocolo ---- */

STATIC
VOID
EFIAPI
TimerInterruptHandler (
  IN  HARDWARE_INTERRUPT_SOURCE   Source,
  IN  EFI_SYSTEM_CONTEXT          SystemContext
  )
{
  EFI_TPL OriginalTpl;

  /* Clear the timer interrupt (DW APB timer clears on write to INTCLR) */
  MmioWrite32 (TIMER_BASE + TIMER_INT_CLR, 1);

  /* Call registered handler at TPL_HIGH_LEVEL as required by DXE */
  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  if (mTimerNotifyFunction) {
    mTimerNotifyFunction (mTimerPeriod);
  }
  gBS->RestoreTPL (OriginalTpl);
}

STATIC
EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  )
{
  /* Follow UEFI spec semantics */
  if ((NotifyFunction == NULL) && (mTimerNotifyFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NotifyFunction != NULL) && (mTimerNotifyFunction != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mTimerNotifyFunction = NotifyFunction;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod   // in 100ns units
  )
{
  EFI_STATUS Status;
  UINT64     Ticks;
  UINT32     Load;

  if (gInterrupt == NULL) {
    return EFI_DEVICE_ERROR;
  }

  /* Disable the interrupt at the controller while we reprogram */
  Status = gInterrupt->DisableInterruptSource (gInterrupt, TIMER_IRQ);
  if (EFI_ERROR (Status)) {
    /* if disable failed, continue carefully */
    DEBUG ((EFI_D_WARN, "Warning: DisableInterruptSource failed: %r\n", Status));
  }

  if (TimerPeriod == 0) {
    /* Stop timer */
    MmioWrite32 (TIMER_BASE + TIMER_CONTROL, 0);
    mTimerPeriod = 0;
    return Status;
  }

  Status = SafeTimerPeriodToTicks (TimerPeriod, &Ticks);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /* Check that ticks fit in 32-bit timer (DW APB typical) */
  if (Ticks == 0 || Ticks > 0xFFFFFFFFULL) {
    return EFI_UNSUPPORTED;
  }

  Load = (UINT32)Ticks;

  /* Clear any pending interrupt */
  MmioWrite32 (TIMER_BASE + TIMER_INT_CLR, 1);

  /* Program load and current value (some DW variants expect both) */
  MmioWrite32 (TIMER_BASE + TIMER_LOAD_COUNT, Load);
  MmioWrite32 (TIMER_BASE + TIMER_CUR_VALUE, Load);

  /* Enable timer: enable + periodic/mode + irq enable */
  MmioWrite32 (TIMER_BASE + TIMER_CONTROL,
               TIMER_CTRL_ENABLE | TIMER_CTRL_MODE | TIMER_CTRL_INT_EN);

  /* Re-enable interrupt at interrupt controller */
  Status = gInterrupt->EnableInterruptSource (gInterrupt, TIMER_IRQ);
  if (!EFI_ERROR (Status)) {
    mTimerPeriod = TimerPeriod;
  } else {
    DEBUG ((EFI_D_ERROR, "EnableInterruptSource failed: %r\n", Status));
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                   *TimerPeriod
  )
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = mTimerPeriod;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
{
  if (mTimerNotifyFunction == NULL) {
    return EFI_UNSUPPORTED;
  }

  /* Call the registered handler as if a hardware tick occurred */
  EFI_TPL OrigTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  mTimerNotifyFunction (mTimerPeriod);
  gBS->RestoreTPL (OrigTpl);
  return EFI_SUCCESS;
}

/* Protocol instance (order: Register, Set, Get, GenerateSoftInterrupt) */
STATIC EFI_TIMER_ARCH_PROTOCOL mTimerProtocol = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};

/* ---- Entrypoint ---- */

EFI_STATUS
EFIAPI
TimerInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle = NULL;

  /* Locate Hardware Interrupt Protocol */
  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to locate HW Interrupt Protocol: %r\n", Status));
    return Status;
  }

  /* Register interrupt handler for the timer IRQ */
  Status = gInterrupt->RegisterInterruptSource (gInterrupt, TIMER_IRQ, TimerInterruptHandler);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to register timer IRQ %d: %r\n", TIMER_IRQ, Status));
    return Status;
  }

  /* Optionally, program an initial timer period here (or leave disabled).
     If you want default behavior, uncomment and set a PCD or constant:
     Status = TimerDriverSetTimerPeriod (&mTimerProtocol, DEFAULT_TIMER_PERIOD_100NS);
  */

  /* Install the Timer Architectural Protocol on a new handle */
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiTimerArchProtocolGuid, &mTimerProtocol,
                  NULL
                );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "InstallMultipleProtocolInterfaces failed: %r\n", Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, "Spreadtrum TimerDxe installed (base=0x%08x irq=%u)\n", TIMER_BASE, TIMER_IRQ));
  return EFI_SUCCESS;
}
