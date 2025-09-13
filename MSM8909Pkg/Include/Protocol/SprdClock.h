#ifndef __SPRD_CLOCK_PROTOCOL_H__
#define __SPRD_CLOCK_PROTOCOL_H__

#define SPRD_CLOCK_PROTOCOL_GUID \
    { 0xb2c7cf0b, 0xf42d, 0x42d2, { 0x84, 0x03, 0x86, 0x12, 0x28, 0xdb, 0xc7, 0x6e } }

typedef struct _SPRD_SC8830_CLOCK_PROTOCOL_GUID SPRD_SC8830_CLOCK_PROTOCOL_GUID;

struct _SPRD_CLOCK_PROTOCOL_GUID {
	INTN SciClkRegister;
};

extern EFI_GUID gSprdClockProtocolGuid;

#endif