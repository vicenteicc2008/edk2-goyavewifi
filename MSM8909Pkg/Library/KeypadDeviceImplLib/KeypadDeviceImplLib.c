#include <PiDxe.h>


#include <Library/KeypadDeviceImplLib.h>
#include <Library/KeypadDeviceHelperLib.h>
#include <Protocol/KeypadDevice.h>
#include <Library/IoLib.h>

typedef struct {
  KEY_CONTEXT EfiKeyContext;
  UINT32 PinctrlBase;
  UINT32 BankOffset;
  UINT32 PinNum;
} KEY_CONTEXT_PRIVATE;

STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeDown;

STATIC KEY_CONTEXT_PRIVATE* KeyList[] = {
  &KeyContextVolumeDown
};

STATIC
VOID
KeypadInitializeKeyContextPrivate (
  KEY_CONTEXT_PRIVATE  *Context
  )
{
  Context->PinctrlBase = 0;
  Context->BankOffset  = 0;
  Context->PinNum      = 0;
}

STATIC
KEY_CONTEXT_PRIVATE*
KeypadKeyCodeToKeyContext (
  UINT32 KeyCode
  )
{
  if (KeyCode == 102){
    return &KeyContextVolumeDown;
  }else
    return NULL;
}

RETURN_STATUS
EFIAPI
KeypadDeviceImplConstructor (
  VOID
  )
{
  UINTN                          Index;
  KEY_CONTEXT_PRIVATE            *StaticContext;

  // Reset all keys
  for (Index=0; Index<ARRAY_SIZE(KeyList); Index++) {
    KeypadInitializeKeyContextPrivate (KeyList[Index]);
  }

  // Configure keys

  // home (gpio2)
  StaticContext              = KeypadKeyCodeToKeyContext(102);
  StaticContext->PinctrlBase = 0xf5224000;
  StaticContext->BankOffset  = 0;
  StaticContext->PinNum      = 113;

  return RETURN_SUCCESS;
}

EFI_STATUS EFIAPI KeypadDeviceImplReset (KEYPAD_DEVICE_PROTOCOL *This)
{
  LibKeyInitializeKeyContext(&KeyContextVolumeDown.EfiKeyContext);
  KeyContextVolumeDown.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  return EFI_SUCCESS;
}

EFI_STATUS KeypadDeviceImplGetKeys (KEYPAD_DEVICE_PROTOCOL *This, KEYPAD_RETURN_API *KeypadReturnApi, UINT64 Delta)
{
    BOOLEAN IsPressed;
    UINTN Index;

    for (Index = 0; Index < (sizeof(KeyList) / sizeof(KeyList[0])); Index++) {
        KEY_CONTEXT_PRIVATE *Context = KeyList[Index];

        IsPressed = FALSE;

  		UINT32 PinAddr = ((Context->PinctrlBase + Context->BankOffset) + 0x4);

        UINT32 PinState = MmioRead32(PinAddr);

        if ( !(PinState & (1 << Context->PinNum)) ) {
        	IsPressed = TRUE;
        }

        LibKeyUpdateKeyStatus(
            &Context->EfiKeyContext, KeypadReturnApi, IsPressed, Delta);
    }

  return EFI_SUCCESS;
}
