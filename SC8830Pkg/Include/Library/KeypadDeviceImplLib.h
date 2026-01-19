#ifndef KEYPAD_DEVICE_IMPL_LIB_H_
#define KEYPAD_DEVICE_IMPL_LIB_H_

#include <PiDxe.h>
#include <Protocol/KeypadDevice.h>
#include <Library/KeypadDeviceHelperLib.h>

RETURN_STATUS
EFIAPI
KeypadDeviceImplConstructor (
    VOID
    );

EFI_STATUS
EFIAPI
KeypadDeviceImplReset (
    IN KEYPAD_DEVICE_PROTOCOL *This
    );

EFI_STATUS
KeypadDeviceImplGetKeys (
    IN KEYPAD_DEVICE_PROTOCOL *This,
    IN KEYPAD_RETURN_API      *KeypadReturnApi,
    IN UINT64                  Delta
    );

#endif // KEYPAD_DEVICE_IMPL_LIB_H_
