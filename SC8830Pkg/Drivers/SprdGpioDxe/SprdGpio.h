#ifndef __SPRD_GPIO_H__
#define __SPRD_GPIO_H__

#include <Shim/sizes.h>

enum gpio_section_type {
    GPIO_SECTION_GPI = 0x0,
    GPIO_SECTION_GPO,
    GPIO_SECTION_GPIO,
    GPIO_SECTION_INVALID
};

struct gpio_section
{
    UINT32 PageBase;
    UINT32 PageSize;
    enum gpio_section_type  section_type;
};

enum gpio_die {
	A_DIE = 0,
	D_DIE = 1,
};

#define SPRD_GPIO_BASE			SCI_IOMAP(0x220000)
#define SPRD_GPIO_PHYS			0X40280000

#define SPRD_EIC_BASE			SCI_IOMAP(0x200000)
#define SPRD_EIC_PHYS			0X40210000

#define SPRD_ADISLAVE_BASE			SCI_IOMAP(0x3f0000 + SZ_32K)
#define SPRD_ADISLAVE_PHYS			0X40038000

#define	D_GPIO_START	0
#define	D_GPIO_NR		256

#define	A_GPIO_START	( D_GPIO_START + D_GPIO_NR )
#define	A_GPIO_NR		32

#define	D_EIC_START		( A_GPIO_START + A_GPIO_NR)
#define	D_EIC_NR		16

#define	A_EIC_START		( D_EIC_START + D_EIC_NR )
#define	A_EIC_NR		16

#define ARCH_NR_GPIOS	( D_EIC_NR + D_GPIO_NR + A_EIC_NR + A_GPIO_NR )

#define ANA_EIC_BASE			(SPRD_ADISLAVE_BASE + 0x100 )
#define ANA_GPIO_INT_BASE		(SPRD_ADISLAVE_BASE + 0x480 )

#define CTL_GPIO_BASE          (SPRD_GPIO_BASE)
#define CTL_EIC_BASE           (SPRD_EIC_BASE)

#define ANA_CTL_EIC_BASE	   (ANA_EIC_BASE)
#define ANA_CTL_GPIO_BASE      (ANA_GPIO_INT_BASE)

#define IRQ_GIC_START			(32)
#define NR_SCI_PHY_IRQS			(IRQ_GIC_START + 125)

#define SCI_IRQ(_X_)			(IRQ_GIC_START + (_X_))
#define SCI_EXT_IRQ(_X_)		(NR_SCI_PHY_IRQS + (_X_))

#define GPIO_IRQ_START			SCI_EXT_IRQ(11)
#define NR_GPIO_IRQS	( 320 )

#define NR_D_DIE_GPIOS 10

#define GPIO_MAX_PIN_NUM            271
#define GPIO_MAX_REC_NUM            10

#define SCI_IOMAP_BASE	0xF5000000
#define SCI_IOMAP(x)	(SCI_IOMAP_BASE + (x))
#define SPRD_ADI_BASE			SCI_IOMAP(0x1f0000)
#define SPRD_ADI_PHYS			0X40030000
#define SPRD_ADI_SIZE			SZ_8K

#define SPRD_MISC_BASE			((UINTN)SPRD_ADI_BASE)
#define SPRD_MISC_PHYS			((UINTN)SPRD_ADI_PHYS)
#define SPRD_ANA_GPIO_PHYS						(SPRD_MISC_PHYS + 0x8480)
#define ANA_GPIO_BASE                           SPRD_ANA_GPIO_PHYS

#define GPIO_BASE                               SPRD_GPIO_PHYS

#define GPI_DATA                        0x0000    //GPI data register, original input signal, not through de-bounce path.
#define GPI_DMSK                        0x0004    //GPI data mask register. GPIDATA register can be read if the mask bit is "1"
#define GPI_IEV                         0x0014    //Interrupt event register, "1" high levels trigger interrupts, "0" low levels trigger interrupts.
#define GPI_IE                          0x0018    //Interrupt mask register, "1" corresponding pin is not masked. "0" corresponding pin interrupt is masked
#define GPI_RIS                         0x001C    //Row interrupt status, reflect the status of interrupts trigger conditions detection on pins (prior to masking). "1" interrupt condition met "0" condition not met
#define GPI_MIS                         0x0020    //Masked interrupt status, "1" Interrupt active "0" interrupt not active
#define GPI_IC                          0x0024    //Interrupt clear, "1" clears level detection interrupt. "0" has no effect.
#define GPI_0CTRL                       0x0028    //GPI0:...
#define GPI_1CTRL                       0x002C    //GPI1:...
#define GPI_2CTRL                       0x0030    //GPI2:...
#define GPI_3CTRL                       0x0034    //GPI3:...
#define GPI_4CTRL                       0x0038    //GPI4:...
#define GPI_5CTRL                       0x003C    //GPI5:...
#define GPI_6CTRL                       0x0040    //GPI4:...
#define GPI_7CTRL                       0x0044    //GPI5:...
#define GPI_TRIG                        0x0048
#define GPI_DEBOUNCE_BIT                 BIT_8
#define GPI_DEBOUNCE_PERIED                255


static __inline int __get_gpio_die( UINT32 gpio)
{
	if (gpio < NR_D_DIE_GPIOS)
		return A_DIE;
	else if (gpio < GPIO_MAX_PIN_NUM)
		return D_DIE;
	else {
		DEBUG((EFI_D_ERROR, "wrong gpio %d\r\n", gpio));
		return -1;
	}
}

static __inline UINT32 __get_base_addr (UINT32 gpio_id)
{
    if (gpio_id < NR_D_DIE_GPIOS)
    {
       return ANA_GPIO_BASE;
    }
    return (gpio_id>>4) * 0x80 + (UINT32) GPIO_BASE;
}

static __inline UINT32 __get_bit_num (UINT32 gpio_id)
{
    return (gpio_id & 0xF);
}

static __inline void gpio_reg_set (UINT32 reg_addr, int die, UINT32 value)
{
    if (die == D_DIE) {
        MmioWrite32(value, reg_addr);
    }
    else
    {
        ANA_REG_SET(reg_addr,value);
    }
    return;
}

static __inline UINT32 gpio_reg_get (UINT32 reg_addr, int die)
{
   	if (die == D_DIE)
		return MmioWrite32(reg_addr);
	else
		return ANA_REG_GET(reg_addr);
}

static __inline void gpio_reg_and (UINT32 reg_addr, int die, UINT32 value)
{
	if (die == D_DIE)
		MmioWrite32(value, reg_addr);
    	else
		ANA_REG_AND(reg_addr, value);
}

static __inline void gpio_reg_or (UINT32 reg_addr, int die, UINT32 value)
{

	if (die == D_DIE)
		MmioWrite32(value, reg_addr);
	else
		ANA_REG_OR(reg_addr,value);
}


#endif