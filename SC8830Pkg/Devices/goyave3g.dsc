[Defines]
  PLATFORM_NAME                  = SC8830Pkg
  PLATFORM_GUID                  = 28f1a3bf-193a-47e3-a7b9-5a435eaab2ee
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = SC8830Pkg/SC8830Pkg.fdf

!include SC8830Pkg/SC8830Pkg.dsc

[PcdsFixedAtBuild.common]
  # System Memory (1GB)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x40000000
  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0x80C00000
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0x00040000      # 256K stack
  gSC8830PkgTokenSpaceGuid.PcdUefiMemPoolBase|0x80D00000         # DXE Heap base address
  gSC8830PkgTokenSpaceGuid.PcdUefiMemPoolSize|0x0F3B0000         # UefiMemorySize, DXE heap size
  gArmTokenSpaceGuid.PcdCpuVectorBaseAddress|0x80C40000

  # Framebuffer (1024x600)
  gSC8830PkgTokenSpaceGuid.PcdMipiFrameBufferAddress|0x9eef4000
  gSC8830PkgTokenSpaceGuid.PcdMipiFrameBufferWidth|1024
  gSC8830PkgTokenSpaceGuid.PcdMipiFrameBufferHeight|600
  gSC8830PkgTokenSpaceGuid.PcdMipiFrameBufferVisibleWidth|1024
  gSC8830PkgTokenSpaceGuid.PcdMipiFrameBufferVisibleHeight|600
  gSC8830PkgTokenSpaceGuid.PcdMipiFrameBufferPixelBpp|16

  gSC8830PkgTokenSpaceGuid.GpioBase|0xf5224000