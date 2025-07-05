#ifndef CLOCK_H
#define CLOCK_H

struct Module
{
	UINT32 Module;
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
	struct clk *Sources[10];
};

/**
 * struct ClkSel - list of sources for a given clock (pll)
 * @sources: array of pointers to clocks
 * @nr_sources: The size of @sources
 */
struct ClkSel {
	int NrSources;
	UINT32 Sources[];
};

#endif