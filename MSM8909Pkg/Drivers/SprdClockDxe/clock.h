#ifndef CLOCK_H
#define CLOCK_H

#include <Shim/list.h>
#include <Shim/Uboot.h>

struct Module
{
	UINT32 Module;
};

struct mutex
{
  int data;
};

struct Clk;
struct ClkLookup
{
	struct list_head Node;
	const char*      DevId;
	const char*      ConId;
	struct Clk*      Clk;
};

struct ClkReg {
	UINT32 Reg;
	//unsigned short shift, size;
	UINT32 Mask;
};

struct ClkRegs {
	int Id;
	const char *Name;
	struct ClkReg Enb, Div, Sel;

	//pll sources select
	int NrSources;
	struct Clk *Sources[10];
};

#define MAX_ERRNO		4095
#define IS_ERR_VALUE(x)		((x) >= (unsigned long)-MAX_ERRNO)

static inline long IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

/**
 * struct ClkSel - list of sources for a given clock (pll)
 * @Sources: array of pointers to clocks
 * @NrSources: The size of @sources
 */
struct ClkSel {
	int NrSources;
	UINT32 Sources[];
};

struct ClkOps {
	int (*SetRate) (struct Clk * c, unsigned long Rate);
	unsigned long (*GetRate) (struct Clk * c);
	unsigned long (*RoundRate) (struct Clk * c, unsigned long Rate);
	int (*SetParent) (struct Clk * c, struct Clk * Parent);
};

struct Clk {
	struct Module *Owner;
	struct Clk *Parent;
	int Usage;
	unsigned long Rate;
	struct ClkOps *Ops;
	int (*Enable) (struct Clk *, int Enable, unsigned long *);

	const struct ClkRegs *Regs;
};

#define MAX_DIV							(1000)

#define SCI_CLK_ADD(ID, RATE, ENB, ENB_BIT, DIV, DIV_MSK, SEL, SEL_MSK, NR_CLKS, ...)        \
static const struct ClkRegs REGS_##ID = {  \
	.name = #ID,                            \
	.id = 0,                                \
	.enb = {                                \
		.reg = (UINT32)ENB,.mask = ENB_BIT,      	\
		},                                  \
	.div = {                                \
		.reg = (UINT32)DIV,.mask = DIV_MSK,			\
		},                                  \
	.sel = {                                \
		.reg = (UINT32)SEL,.mask = SEL_MSK,			\
		},                                  \
	.nr_sources = NR_CLKS,                  \
	.sources = {__VA_ARGS__},               \
};                                          \
static struct Clk ID = {              		\
	.owner = THIS_MODULE,                   \
	.parent = 0,                            \
	.usage = 0,                             \
	.rate = RATE,                           \
	.regs = &REGS_##ID,                     \
	.ops = 0,                               \
	.enable = 0,                            \
};                                          \
const struct ClkLookup __clkinit1 CLK_LK_##ID = { \
	.dev_id = 0,							\
	.con_id = #ID,                          \
	.clk = &ID,                       		\
};                                       	\

#define spin_lock_irqsave(lock, cpu_sr) do {*lock=0;cpu_sr=1;}while(0)
#define spin_unlock_irqrestore(lock, cpu_sr) do {*lock=0;int i=cpu_sr;cpu_sr=i;}while(0)

typedef struct {
  UINT32 Cpu;
  UINT32 Old;
  UINT32 New;
  UINT32 Flags;
} CPUFREQ_FREQS;

#endif