Attempt to create a minimal EDK2 for Samsung Galaxy Tab E 7.0 WIFI (SM-T113NU) but should work on SM-T116 (3G variant)

## Status
Boots to UEFI Shell but buttons, eMMC and USB are not working.

## WARNING
this EDK2 does working only on DEBUG target because if you compile on the RELEASE target you have a bug that causes a data abort exception to appear.

## Building
Tested on Ubuntu 22.04.

First, clone EDK2.

```
cd ..
git clone https://github.com/tianocore/edk2.git -b edk2-stable202405 --recursive
git clone https://github.com/tianocore/edk2-platforms.git
```

You should have all three directories side by side.

Next, install dependencies:

20.04 or later:

```
sudo apt install build-essential uuid-dev iasl git nasm python3-distutils crossbuild-essential-armel
```

Also see [EDK2 website](https://github.com/tianocore/tianocore.github.io/wiki/Using-EDK-II-with-Native-GCC#Install_required_software_from_apt)

Then ./scripts/firstrun.sh

Finally, "./scripts/goyavewifi.sh" to compile or "./scripts/goyave3g.sh" to compile for goyave3g.

Then make a Image from AIK (Android Image Kitchen) and rename image-new.img to boot.img and make a tar for flash with odin.

Finally, flash it with Odin.

or use one of my releases in github (flash it with TWRP)

# Credits

SimpleFbDxe screen driver is from imbushuo's [Lumia950XLPkg](https://github.com/WOA-Project/Lumia950XLPkg).

Based on sonic011gamer [edk2-msm8909](https://github.com/sonic011gamer/edk2-msm8909).

SprdGpioDxe driver is from halal-beef's [edk2-exynos9830](https://github.com/halal-beef/edk2-exynos9830).

some drivers like SprdClockDxe (WIP), SprdSdhciDxe (not working for now) and SprdI2CDxe are from U-Boot and Linux kernel

some functions from SprdSdhciDxe (like SdhciFlushBlocks) are from [Mu-Silicium](https://github.com/Project-Silicium/Mu-Silicium)

DSDT and TimerDxe (disabled for now) are from AistopGit's [edk2-exynos5410](https://github.com/AistopGit/edk2-exynos5410).

Dominduchami for some changes for 16 Bpp FB in [HtcLeoPkg](https://github.com/HTC-Leo-Revival-Project/HtcLeoPkg) which are not used but i used it for fixing framebuffer.
