DefinitionBlock("DSDT.aml", "DSDT", 0x02, "SPRDD ", "SC8830 ", 0x00000003)
{
    Scope(_SB_)
    {
        Device(CPU0)
        {
            Name(_HID, "ACPI0007")
            Name(_UID, Zero)
        }
        Device(CPU1)
        {
            Name(_HID, "ACPI0007")
            Name(_UID, One)
        }
        Device(CPU2)
        {
            Name(_HID, "ACPI0007")
            Name(_UID, 0x2)
        }
        Device(CPU3)
        {
            Name(_HID, "ACPI0007")
            Name(_UID, 0x3)
        }
        Device(SDC1)
        {
            Name(_HID, "SPRD0120")
            Name(_CID, "ACPI/SPRD0120")
            Name(_UID, Zero)
			Method (_CRS, 0x0, NotSerialized) {
				Name (RBUF, ResourceTemplate ()
				{
					Memory32Fixed (ReadWrite, 0xF511C000, 0x00001000)
					Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, ) { 92 }
				})
				Return (RBUF)
			}
            Device(EMMC)
            {
                Method(_ADR, 0x0, NotSerialized)
                {
                    Return(0x8)
                }
                Method(_RMV, 0x0, NotSerialized)
                {
                    Return(Zero)
                }
            }
        }
    }
}
