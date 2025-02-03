#ifndef __CLOCK_H_
#define __CLOCK_H_

struct clk_sel {
	int nr_sources;
	UINT32 sources[];
};

struct clk_reg {
	UINT32 reg;
	//unsigned short shift, size;
	UINT32 mask;
};

#define spin_lock_irqsave(lock, cpu_sr) do {*lock=0;cpu_sr=1;}while(0)
#define spin_unlock_irqrestore(lock, cpu_sr) do {*lock=0;int i=cpu_sr;cpu_sr=i;}while(0)

struct mutex
{
  int data;
};

struct clk {
	struct module *owner;
	struct clk *parent;
	int usage;
	UINT64 rate;
	struct CLK_OPS *ops;
	int (*enable) (struct clk *, int enable, unsigned long *);

	const struct clk_regs *regs;
	struct dentry *dent;	/* For visible tree hierarchy */
};

struct CLK_LOOKUP {
    LIST_ENTRY node;
    CONST CHAR8 *dev_id;
    CONST CHAR8 *con_id;
    struct clk *clk;
};

struct clk_regs {
	int id;
	const char *name;
	struct clk_reg enb, div, sel;

	//pll sources select
	int nr_sources;
	struct clk *sources[10];
};

struct CLK_OPS {
	int (*set_rate) (struct clk * c, unsigned long rate);
	unsigned long (*get_rate) (struct clk * c);
	unsigned long (*round_rate) (struct clk * c, unsigned long rate);
	int (*set_parent) (struct clk * c, struct clk * parent);
};

#define MAX_DIV							(1000)

#endif