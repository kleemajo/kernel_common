/*
 *  arch/arm/mach-apple_iphone/clock.c
 *
 *  Copyright (C) 2008 Yiduo Wang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <mach/iphone-clock.h>

/* Constants */
#define NUM_PLL 4
#define PLL0_INFREQ_DIV FREQUENCY_BASE /* 12 MHz */
#define PLL1_INFREQ_DIV FREQUENCY_BASE /* 12 MHz */
#define PLL2_INFREQ_DIV FREQUENCY_BASE /* 12 MHz */
#define PLL3_INFREQ_DIV 13500000 /* 13.5 MHz */
#define PLL0_INFREQ_MULT 0x4000
#define PLL1_INFREQ_MULT 0x4000
#define PLL2_INFREQ_MULT 0x4000
#define PLL3_INFREQ_MULT FREQUENCY_BASE

/* Devices */
#define CLOCK1 IO_ADDRESS(0x3C500000)

/* Registers */
#define CLOCK1_CONFIG0 0x0
#define CLOCK1_CONFIG1 0x4
#define CLOCK1_CONFIG2 0x8
#define CLOCK1_PLL0CON 0x20
#define CLOCK1_PLL1CON 0x24
#define CLOCK1_PLL2CON 0x28
#define CLOCK1_PLL3CON 0x2C
#define CLOCK1_PLLMODE 0x44
#define CLOCK1_CL2_GATES 0x48
#define CLOCK1_CL3_GATES 0x4C

/* Values */
#define CLOCK1_Separator 0x20

#define CLOCK1_CLOCKPLL(x) GET_BITS((x), 12, 2)
#define CLOCK1_CLOCKDIVIDER(x) (GET_BITS((x), 0, 4) + 1)
#define CLOCK1_CLOCKHASDIVIDER(x) GET_BITS((x), 8, 1)

#define CLOCK1_MEMORYPLL(x) GET_BITS((x), 12, 2)
#define CLOCK1_MEMORYDIVIDER(x) (GET_BITS((x), 16, 4) + 1)
#define CLOCK1_MEMORYHASDIVIDER(x) GET_BITS((x), 24, 1)

#define CLOCK1_BUSPLL(x) GET_BITS((x), 12, 2)
#define CLOCK1_BUSDIVIDER(x) (GET_BITS((x), 16, 4) + 1)
#define CLOCK1_BUSHASDIVIDER(x) GET_BITS((x), 24, 1)

#define CLOCK1_UNKNOWNPLL(x) GET_BITS((x), 12, 2)
#define CLOCK1_UNKNOWNDIVIDER1(x) (GET_BITS((x), 0, 4) + 1)
#define CLOCK1_UNKNOWNDIVIDER2(x) (GET_BITS((x), 4, 4) + 1)
#define CLOCK1_UNKNOWNDIVIDER(x) \
			(CLOCK1_UNKNOWNDIVIDER1(x) * CLOCK1_UNKNOWNDIVIDER2(x))
#define CLOCK1_UNKNOWNHASDIVIDER(x) GET_BITS((x), 8, 1)

#define CLOCK1_PERIPHERALDIVIDER(x) GET_BITS((x), 20, 2)

#define CLOCK1_DISPLAYPLL(x) GET_BITS((x), 28, 2)
#define CLOCK1_DISPLAYDIVIDER(x) GET_BITS((x), 16, 4)
#define CLOCK1_DISPLAYHASDIVIDER(x) GET_BITS((x), 24, 1)

#define CLOCK1_PLLMODE_ONOFF(x, y) (((x) >> (y)) & 0x1)
#define CLOCK1_PLLMODE_DIVIDERMODE(x, y) (((x) >> (y + 4)) & 0x1)
#define CLOCK1_PLLMODE_DIVIDE 1
#define CLOCK1_PLLMODE_MULTIPLY 0

#define CLOCK1_MDIV(x) (((x) >> 8) & 0x3FF)
#define CLOCK1_PDIV(x) (((x) >> 24) & 0x3F)
#define CLOCK1_SDIV(x) ((x) & 0x3)

/*
void iphone_clock_gate_switch(u32 gate, int on_off)
{
	u32 gate_register;
	u32 gate_flag;
	u32 gates;

	if (gate < CLOCK1_Separator) {
		gate_register = CLOCK1 + CLOCK1_CL2_GATES;
		gate_flag = gate;
	} else {
		gate_register = CLOCK1 + CLOCK1_CL3_GATES;
		gate_flag = gate - CLOCK1_Separator;
	}

	gates = __raw_readl(gate_register);

	if (on_off)
		gates &= ~(1 << gate_flag);
	else
		gates |= 1 << gate_flag;

	__raw_writel(gates, gate_register);

}
*/


#define CLOCK 0x3C500000
#define CLOCK_GATES_0 0x48
#define CLOCK_GATES_1 0x4C
#define CLOCK_GATES_2 0x58
#define CLOCK_GATES_3 0x68
#define CLOCK_GATES_4 0x6C

#define NUM_CLOCK_GATE_REGISTERS 5
typedef struct {
	uint32_t id;
	uint32_t reg_0_bits;
	uint32_t reg_1_bits;
	uint32_t reg_2_bits;
	uint32_t reg_3_bits;
	uint32_t reg_4_bits;
} ClockGate;

ClockGate ClockGateTable[29] = {
	// id	reg_0_bits	reg_1_bits	reg_2_bits	reg_3_bits	reg_4_bits
	{0x0,	0x00000080,	0x0,		0x0,		0x0,		0x0		},
	{0x1,	0x0,		0x00004000,	0x0,		0x0,		0x0		},
	{0x2,	0x00004000,	0x0,		0x0,		0x00004E00,	0x0		},
	{0x3,	0x00000800,	0x0,		0x0,		0x0,		0x0		},
	{0x4,	0x00001000,	0x0,		0x0,		0x0,		0x0		},
	{0x5,	0x00000220,	0x0,		0x0,		0x0,		0x0		},
	{0x6,	0x0,		0x00001000,	0x0,		0x0,		0x0		},
	{0x7,	0x0,		0x00000010,	0x0,		0x0,		0x00000800	},
	{0x8,	0x0,		0x00000040,	0x0,		0x0,		0x00001000	},
	{0x9,	0x00000002,	0x0,		0x0,		0x0,		0x00010000	},
	{0xA,	0x0,		0x00080000,	0x0,		0x0,		0x0		},
	{0xB,	0x0,		0x00002000,	0x0,		0x0,		0x0		},
	{0xC,	0x00000400,	0x0,		0x0,		0x0,		0x0		},
	{0xD,	0x00000001,	0x0,		0x0,		0x0,		0x0		},
	{0xE,	0x0,		0x00000004,	0x0,		0x0,		0x00002000	},
	{0xF,	0x0,		0x00000800,	0x0,		0x0,		0x00004000	},
	{0x10,	0x0,		0x00008000,	0x0,		0x0,		0x00008000	},
	{0x11,	0x0,		0x0,		0x00000002,	0x0,		0x00080000	},
	{0x12,	0x0,		0x0,		0x00000010,	0x0,		0x00100000	},
	{0x13,	0x0,		0x1F800020,	0x00000060,	0x0,		0x00C0007F	},
	{0x14,	0x0,		0x00000200,	0x0,		0x0,		0x00000080	},
	{0x15,	0x0,		0x20000000,	0x0,		0x0,		0x00000100	},
	{0x16,	0x0,		0x40000000,	0x0,		0x0,		0x00000200	},
	{0x17,	0x0,		0x80000000,	0x0,		0x0,		0x00000400	},
	{0x18,	0x00000004,	0x0,		0x0,		0x0,		0x0		},
	{0x19,	0x0,		0x00000008,	0x0,		0x0,		0x0		},
	{0x1A,	0x0,		0x0,		0x00000001,	0x0,		0x0		},
	{0x1B,	0x00000220,	0x0,		0x0,		0x0,		0x0		},
	{0x1C,	0x00000220,	0x0,		0x0,		0x0,		0x0		}
};

void iphone_clock_gate_switch(u32 gate, int on_off) {
	uint32_t gate_register;
	uint32_t gate_register_bits;
	uint32_t reg_num;
	for (reg_num=0; reg_num<NUM_CLOCK_GATE_REGISTERS; reg_num++) {
		switch (reg_num) {
			case 0:
				gate_register = CLOCK + CLOCK_GATES_0;
				gate_register_bits = ClockGateTable[gate].reg_0_bits;
				break;
			case 1:
				gate_register = CLOCK + CLOCK_GATES_1;
				gate_register_bits = ClockGateTable[gate].reg_1_bits;
				break;
			case 2:
				gate_register = CLOCK + CLOCK_GATES_2;
				gate_register_bits = ClockGateTable[gate].reg_2_bits;
				break;
			case 3:
				gate_register = CLOCK + CLOCK_GATES_3;
				gate_register_bits = ClockGateTable[gate].reg_3_bits;
				break;
			case 4:
				gate_register = CLOCK + CLOCK_GATES_4;
				gate_register_bits = ClockGateTable[gate].reg_4_bits;
				break;
			default:
				continue;
		}
		
		uint32_t gate_register_data = __raw_readl(IO_ADDRESS(gate_register));
		if (on_off) {
			gate_register_data &= ~gate_register_bits;
		} else {
			gate_register_data |= gate_register_bits;
		}
		__raw_writel(gate_register_data, IO_ADDRESS(gate_register));
	}
}
