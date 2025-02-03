#include <Uefi.h>
#include <Library/UefiLib.h>
#include <string.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/ArmLib.h>
#include <Shim/list.h>

#include "clock.h"

static LIST_HEAD(clocks);
static int clocks_lock = 0;
static struct mutex clocks_mutex;

int clk_enable(struct clk *clk)
{
	unsigned long flags;
	if (clk == NULL) {
        return EFI_INVALID_PARAMETER;
    }

	clk_enable(clk->parent);

	spin_lock_irqsave(&clocks_lock, flags);
	if ((clk->usage++) == 0 && clk->enable)
		(clk->enable) (clk, 1, &flags);
	spin_unlock_irqrestore(&clocks_lock, flags);
	DEBUG((EFI_D_INFO, "clk %p, usage %d\n", clk, clk->usage));
	return 0;
}

void clk_disable(struct clk *clk)
{
	unsigned long flags;
	if (clk == NULL) {
        return;
    }

	spin_lock_irqsave(&clocks_lock, flags);
	if ((--clk->usage) == 0 && clk->enable)
		(clk->enable) (clk, 0, &flags);
	if (clk->usage < 0) {
        DEBUG((EFI_D_WARN, "warning: clock (%a) usage (%d)\n", clk->regs->name, clk->usage));
        clk->usage = 0;  /* force reset clock refcnt */
		spin_unlock_irqrestore(&clocks_lock, flags);
        return;
    }

	spin_unlock_irqrestore(&clocks_lock, flags);
	DEBUG((EFI_D_INFO, "clk %p, usage %d\n", clk, clk->usage));
	clk_disable(clk->parent);
}

void clk_force_disable(struct clk *clk)
{
	if (clk == NULL) {
        return;
    }

	DEBUG((EFI_D_INFO, "clk %p, usage %d\n", clk, clk->usage));
	while (clk->usage > 0) {
		clk_disable(clk);
	}
}

unsigned long clk_get_rate(struct clk *clk)
{
    if (clk == NULL) {
        DEBUG((EFI_D_INFO, "clk %p, rate %lu\n", clk, (UINT64)-1));
        return (UINT64)-1;
    }

    DEBUG((EFI_D_INFO, "clk %p, rate %lu\n", clk, clk->rate));

    if (clk->rate != 0) {
        return clk->rate;
    }

    if (clk->ops != NULL && clk->ops->get_rate != NULL) {
        return (clk->ops->get_rate)(clk);
    }

    if (clk->parent != NULL) {
        return clk_get_rate(clk->parent);
    }

    return clk->rate;
}

long clk_round_rate(struct clk *clk, UINT64 rate)
{
    if (clk == NULL) {
        return rate;
    }

    if (clk->ops != NULL && clk->ops->round_rate != NULL) {
        return (clk->ops->round_rate)(clk, rate);
    }

    return rate;
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret;
	unsigned long flags;
	DEBUG((EFI_D_INFO, "clk %p, rate %lu\n", clk, rate));
	if (clk == NULL || rate == 0) {
        return EFI_INVALID_PARAMETER;
    }


	/* We do not default just do a clk->rate = rate as
	 * the clock may have been made this way by choice.
	 */

	//WARN_ON(clk->ops == NULL);
	//WARN_ON(clk->ops && clk->ops->set_rate == NULL);

	if (clk->ops == NULL || clk->ops->set_rate == NULL)
		return EFI_INVALID_PARAMETER;

	spin_lock_irqsave(&clocks_lock, flags);
	ret = (clk->ops->set_rate) (clk, rate);
	spin_unlock_irqrestore(&clocks_lock, flags);
	return ret;
}

struct clk *clk_get_parent(struct clk *clk)
{
	return clk->parent;
}

UINT32 sci_glb_read(UINT32 reg, UINT32 msk)
{
	return MmioRead32(reg) & msk;
}

int sci_glb_write(UINT32 reg, UINT32 val, UINT32 msk)
{
	unsigned long flags, hw_flags;
	MmioWrite32((MmioRead32(reg) & ~msk) | val, reg);
	return 0;
}

static int __is_glb(UINT32 reg)
{
	//return rounddown(reg, SZ_64K) == rounddown(GREG_BASE, SZ_64K) || rounddown(reg, SZ_64K) == rounddown(AHB_GEN_CTL_BEGIN, SZ_64K);
	return 1;
}

#define REG_GLB_SET(A)                  ( A + 0x1000 )
#define REG_GLB_CLR(A)                  ( A + 0x2000 )

EFI_STATUS sci_glb_set(UINT32 reg, UINT32 bit)
{
    if (__is_glb(reg)) {
        MmioWrite32(REG_GLB_SET(reg), bit);
    } else {
        DEBUG((EFI_D_WARN, "Invalid global register: %x\n", reg));
        return EFI_INVALID_PARAMETER;
    }
    return EFI_SUCCESS;
}

EFI_STATUS sci_glb_clr(UINT32 reg, UINT32 bit)
{
    if (__is_glb(reg)) {
        MmioWrite32(REG_GLB_CLR(reg), bit);
    } else {
        DEBUG((EFI_D_WARN, "Invalid global register: %x\n", reg));
        return EFI_INVALID_PARAMETER;
    }
    return EFI_SUCCESS;
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = EFI_ACCESS_DENIED;
	unsigned long flags;
	struct clk *old_parent = clk_get_parent(clk);
	DEBUG((EFI_D_INFO, "clk %p, parent %p <<< %p\n", clk, parent, old_parent));
	if (clk == NULL || parent == NULL) {
        return EFI_INVALID_PARAMETER;
    }

	spin_lock_irqsave(&clocks_lock, flags);
	if (clk->ops && clk->ops->set_parent)
		ret = (clk->ops->set_parent) (clk, parent);
	spin_unlock_irqrestore(&clocks_lock, flags);
	return ret;
}

int sci_clk_enable(struct clk *c, BOOLEAN enable, unsigned long *pflags) {
    DEBUG((EFI_D_INFO, "clk %p (%a) enb %08x, %a\n", c, c->regs->name, c->regs->enb.reg, enable ? "enable" : "disable"));

    ASSERT(c->regs->enb.reg != 0);
    if (c->regs->enb.reg & 1) {
        enable = !enable;
    }

    if (c->regs->enb.mask == 0) { // enable matrix clock
        if (pflags)
			spin_unlock_irqrestore(&clocks_lock, *pflags);
        if (enable) {
            clk_enable((struct clk *)c->regs->enb.reg);
        } else {
            clk_disable((struct clk *)c->regs->enb.reg);
        }
        if (pflags)
			spin_lock_irqsave(&clocks_lock, *pflags);

    } else {
        if (enable) {
            sci_glb_set(c->regs->enb.reg & ~1, c->regs->enb.mask);
        } else {
            sci_glb_clr(c->regs->enb.reg & ~1, c->regs->enb.mask);
        }
    }
    return EFI_SUCCESS;
}

static int sci_clk_is_enable(struct clk *c)
{
	int enable;

	DEBUG((EFI_D_INFO, "clk %p (%s) enb %08x\n", c, c->regs->name, c->regs->enb.reg));

	ASSERT(!c->regs->enb.reg);
	if (!c->regs->enb.mask) {	/* check matrix clock */
		enable = ! !sci_clk_is_enable((struct clk *)c->regs->enb.reg);
	} else {
		enable =
		    ! !sci_glb_read(c->regs->enb.reg & ~1, c->regs->enb.mask);
	}

	if (c->regs->enb.reg & 1)
		enable = !enable;
	return enable;
}

static unsigned long __ffs(unsigned int x)
{
	return ffs(x) -1;
}

static int sci_clk_set_rate(struct clk *c, unsigned long rate)
{
	UINT32 div, div_shift;
	DEBUG((EFI_D_INFO, "clk %p (%s) set rate %lu\n", c, c->regs->name, rate));
	rate = clk_round_rate(c, rate);
	div = clk_get_rate(c->parent) / rate - 1;	//FIXME:
	div_shift = __ffs(c->regs->div.mask);
	DEBUG((EFI_D_INFO, "clk %p (%s) pll div reg %08x, val %08x mask %08x\n", c,
	       c->regs->name, c->regs->div.reg, div << div_shift,
	       c->regs->div.mask));
	sci_glb_write(c->regs->div.reg, div << div_shift, c->regs->div.mask);

	c->rate = 0;		/* FIXME: auto update all children after new rate if need */
	return 0;
}

static unsigned long sci_clk_get_rate(struct clk *c)
{
	UINT32 div = 0, div_shift;
	unsigned long rate;
	div_shift = __ffs(c->regs->div.mask);
	DEBUG((EFI_D_INFO, "clk %p (%s) div reg %08x, shift %u msk %08x\n", c,
	       c->regs->name, c->regs->div.reg, div_shift, c->regs->div.mask));
	rate = clk_get_rate(c->parent);

	if (c->regs->div.reg)
		div = sci_glb_read(c->regs->div.reg,
				   c->regs->div.mask) >> div_shift;
	DEBUG((EFI_D_INFO, "clk %p (%s) parent rate %lu, div %u\n", c, c->regs->name, rate,
	       div + 1));
	c->rate = rate = rate / (div + 1);	//FIXME:
	DEBUG((EFI_D_INFO, "clk %p (%s) get real rate %lu\n", c, c->regs->name, rate));
	return rate;
}

#define BIT(x) (1<<x)
#define SHFT_PLL_REFIN                 ( 16 )
#define MASK_PLL_REFIN                 ( BIT(16)|BIT(17) )

static unsigned long sci_pll_get_refin_rate(struct clk *c)
{
	int i;
	const unsigned long refin[4] = { 2, 4, 4, 13 };	/* default refin 4M */
	i = sci_glb_read(c->regs->div.reg, MASK_PLL_REFIN) >> SHFT_PLL_REFIN;
	DEBUG((EFI_D_INFO, "pll %p (%s) refin %d\n", c, c->regs->name, i));
	return refin[i] * 1000000;
}

static unsigned long sci_pll_get_rate(struct clk *c)
{
	UINT32 mn = 1, mn_shift;
	unsigned long rate;
	mn_shift = __ffs(c->regs->div.mask);
	DEBUG((EFI_D_INFO, "pll %p (%s) mn reg %08x, shift %u msk %08x\n", c, c->regs->name,
	       c->regs->div.reg, mn_shift, c->regs->div.mask));
	rate = clk_get_rate(c->parent);
	if (0 == c->regs->div.reg) ;
	else if (c->regs->div.reg < MAX_DIV) {
		mn = c->regs->div.reg;
		if (mn)
			rate = rate / mn;
	} else {
		rate = sci_pll_get_refin_rate(c);
		mn = sci_glb_read(c->regs->div.reg,
				  c->regs->div.mask) >> mn_shift;
		if (mn)
			rate = rate * mn;
	}
	c->rate = rate;
	DEBUG((EFI_D_INFO, "pll %p (%s) get real rate %lu\n", c, c->regs->name, rate));
	return rate;
}

static unsigned long sci_clk_round_rate(struct clk *c, unsigned long rate)
{
	DEBUG((EFI_D_INFO, "clk %p (%s) round rate %lu\n", c, c->regs->name, rate));
	return rate;
}

static int sci_clk_set_parent(struct clk *c, struct clk *parent)
{
	int i;
	DEBUG((EFI_D_INFO, "clk %p (%s) parent %p (%s)\n", c, c->regs->name,
	       parent, parent ? parent->regs->name : 0));

	for (i = 0; i < c->regs->nr_sources; i++) {
		if (c->regs->sources[i] == parent) {
			UINT32 sel_shift = __ffs(c->regs->sel.mask);
			DEBUG((EFI_D_INFO, "pll sel reg %08x, val %08x, msk %08x\n",
			       c->regs->sel.reg, i << sel_shift,
			       c->regs->sel.mask));
			if (c->regs->sel.reg)
				sci_glb_write(c->regs->sel.reg, i << sel_shift,
					      c->regs->sel.mask);
			c->parent = parent;
			if (c->ops)
				c->rate = 0;	/* FIXME: auto update clock rate after new parent */
			return 0;
		}
	}

	DEBUG((EFI_D_WARN, "warning: clock (%s) not support parent (%s)\n",
	     c->regs->name, parent ? parent->regs->name : 0));
	return EFI_INVALID_PARAMETER;
}

static int sci_clk_get_parent(struct clk *c)
{
	int i = 0;
	UINT32 sel_shift = __ffs(c->regs->sel.mask);
	DEBUG((EFI_D_INFO, "pll sel reg %08x, val %08x, msk %08x\n",
	       c->regs->sel.reg, i << sel_shift, c->regs->sel.mask));
	if (c->regs->sel.reg) {
		i = sci_glb_read(c->regs->sel.reg,
				 c->regs->sel.mask) >> sel_shift;
	}
	return i;
}

static struct CLK_OPS generic_clk_ops = {
	.set_rate = sci_clk_set_rate,
	.get_rate = sci_clk_get_rate,
	.round_rate = sci_clk_round_rate,
	.set_parent = sci_clk_set_parent,
};

static struct CLK_OPS generic_pll_ops = {
	.set_rate = 0,
	.get_rate = sci_pll_get_rate,
	.round_rate = 0,
	.set_parent = sci_clk_set_parent,
};

static int __clk_is_dummy_pll(struct clk *c)
{
	return (c->regs->enb.reg & 1) || strstr(c->regs->name, "pll");
}

static struct clk_lookup *clk_find(const char *dev_id, const char *con_id)
{
	struct clk_lookup *p, *cl = NULL;
	int match, best = 0;

	list_for_each_entry(p, &clocks, node) {
		match = 0;
		if (p->dev_id) {
			if (!dev_id || AsciiStrCmp(p->dev_id, dev_id))
				continue;
			match += 2;
		}
		if (p->con_id) {
			if (!con_id || AsciiStrCmp(p->con_id, con_id))
				continue;
			match += 1;
		}

		if (match > best) {
			cl = p;
			if (match != 3)
				best = match;
			else
				break;
		}
	}
	return cl;
}

void mutex_lock(struct mutex* lock)
{
  return;
}

void mutex_unlock(struct mutex* lock)
{
  return;
}

VOID clkdev_add(struct CLK_LOOKUP *cl)
{
	mutex_lock(&clocks_mutex);
	list_add_tail(&cl->node, &clocks);
	mutex_unlock(&clocks_mutex);
}

static int sci_clk_register(struct CLK_LOOKUP *cl)
{
    struct clk *c = cl->clk;

    if (c->ops == NULL) {
        c->ops = &generic_clk_ops;
        if (c->rate) {  /* fixed OSC */
            c->ops = NULL;
        } else if ((c->regs->div.reg >= 0 && c->regs->div.reg < MAX_DIV) ||
                   AsciiStrStr(c->regs->name, "pll")) {
            c->ops = &generic_pll_ops;
        }
    }

    DEBUG((EFI_D_INFO, "clk %p (%a) rate %lu ops %p enb %08x sel %08x div %08x nr_sources %u\n",
           c, c->regs->name, c->rate, c->ops, c->regs->enb.reg,
           c->regs->sel.reg, c->regs->div.reg, c->regs->nr_sources));

    if (c->enable == NULL && c->regs->enb.reg) {
        c->enable = sci_clk_enable;
        /* FIXME: dummy update some pll clocks usage */
        if (sci_clk_is_enable(c) && __clk_is_dummy_pll(c)) {
            clk_enable(c);
        }
    }

    if (!c->rate) {  /* FIXME: dummy update clock parent and rate */
        clk_set_parent(c, c->regs->sources[sci_clk_get_parent(c)]);
        /* clk_set_rate(c, clk_get_rate(c)); */
    }

    clkdev_add(cl);

    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
SprdClockDxeInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
	DEBUG((EFI_D_INFO, "SprdClockDxe: Initializing Clocks\n"));

	return EFI_SUCCESS;
}
