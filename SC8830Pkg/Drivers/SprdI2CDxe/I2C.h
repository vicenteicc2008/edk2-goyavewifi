#ifndef __SPRD_I2C_H__
#define __SPRD_I2C_H__

#define SPRD_I2C_CTL_ID	(6)
#define I2C_CTL	0x0000
#define I2C_CMD	0x0004
#define I2C_CLKD0	0x0008
#define I2C_CLKD1	0x000C
#define I2C_RST	0x0010
#define I2C_CMD_BUF	0x0014
#define I2C_CMD_BUF_CTL	0x0018

#define I2CCTL_INT                      (1 << 0)        //I2c interrupt
#define I2CCTL_ACK                      (1 << 1)        //I2c received ack value
#define I2CCTL_BUSY                     (1 << 2)        //I2c data line value
#define I2CCTL_IE                       (1 << 3)        //I2c interrupt enable
#define I2CCTL_EN                       (1 << 4)        //I2c module enable
#define I2CCTL_CMDBUF_EN                (1 << 5)        //Enable the cmd buffer mode
#define I2CCTL_CMDBUF_EXEC              (1 << 6)        //Start to exec the cmd in the cmd buffer
#define I2CCTL_ST_CMDBUF                (7 << 7)        //The state of  I2c cmd buffer state machine.
#define I2CCTL_CMDBUF_WPTR              (7 << 7)        //I2c command buffer write pointer

/*The corresponding bit of I2C_CMD register*/
#define I2C_CMD_INT_ACK	(1 << 0)	/* I2c interrupt clear bit */
#define I2C_CMD_TX_ACK	(1 << 1)	/* I2c transmit ack that need to be send */
#define I2C_CMD_WRITE	(1 << 2)	/* I2c write command */
#define I2C_CMD_READ	(1 << 3)	/* I2c read command */
#define I2C_CMD_STOP	(1 << 4)	/* I2c stop command */
#define I2C_CMD_START	(1 << 5)	/* I2c start command */
#define I2C_CMD_ACK	(1 << 6)	/* I2c received ack  value */
#define I2C_CMD_BUSY	(1 << 7)	/* I2c busy in exec commands */
#define I2C_CMD_DATA	0xFF00	/* I2c data received or data need to be transmitted */

/*The corresponding bit of I2C_RST register*/
#define I2C_RST_RST	(1 << 0)	/* I2c reset bit */

/*The corresponding bit of I2C_CMD_BUF_CTL register*/
#define I2C_CTL_CMDBUF_EN	(1 << 0)	/* Enable the cmd buffer mode */
#define I2C_CTL_CMDBUF_EXEC	(1 << 1)	/* Start to exec the cmd in the cmd buffer */

#define ARM_CLK_13M         13000000
#define ARM_CLK_24M         24000000
#define ARM_CLK_26M         26000000

#define I2C_READ_BIT        0x1

#define I2C_CLEAR_INT

#define I2C_TIMEOUT_FACTOR  500000  //the critical value is 10000
#define I2CCMD_WRITE                    (1 << 2)        //I2c write command
#define I2CCMD_READ                     (1 << 3)        //I2c read command
#define I2CCMD_STOP                     (1 << 4)        //I2c stop command
#define I2CCMD_START                    (1 << 5)        //I2c start command
#define I2CCMD_INT_ACK                  (1 << 0)        //I2c interrupt clear bit
#define I2CCMD_TX_ACK                   (1 << 1)        //I2c transmit ack that need to be send
#define I2CCMD_ACK                      (1 << 6)        //I2c received ack  value
#define I2CCMD_BUS                      (1 << 7)        //I2c busy in exec commands
#define I2CCMD_DATA                     0xFF00          //I2c data received or data need to be transmitted

#define CTL_BASE_IIC0        0x70500000
#define I2C_BASE             CTL_BASE_IIC0

#define BIT_0                                          0x01
#define BIT_1                                          0x02
#define BIT_2                                          0x04
#define BIT_3                                          0x08
#define BIT_4                                          0x10
#define BIT_5                                          0x20
#define BIT_6                                          0x40
#define BIT_7                                          0x80
#define BIT_8                                          0x0100
#define BIT_9                                          0x0200
#define BIT_10                                         0x0400
#define BIT_11                                         0x0800
#define BIT_12                                         0x1000
#define BIT_13                                         0x2000
#define BIT_14                                         0x4000
#define BIT_15                                         0x8000
#define BIT_16                                         0x010000
#define BIT_17                                         0x020000
#define BIT_18                                         0x040000
#define BIT_19                                         0x080000
#define BIT_20                                         0x100000
#define BIT_21                                         0x200000
#define BIT_22                                         0x400000
#define BIT_23                                         0x800000
#define BIT_24                                         0x01000000
#define BIT_25                                         0x02000000
#define BIT_26                                         0x04000000
#define BIT_27                                         0x08000000
#define BIT_28                                         0x10000000
#define BIT_29                                         0x20000000
#define BIT_30                                         0x40000000
#define BIT_31                                         0x80000000

typedef enum
{
	ERR_I2C_NONE = 0,				// Success,no error
	ERR_I2C_ACK_TIMEOUT,			// I2C wait ACK timeout
	ERR_I2C_INT_TIMEOUT,			// I2C wait INT timeout
	ERR_I2C_BUSY_TIMEOUT,			// I2C wait BUSY timeout
	ERR_I2C_DEVICE_NOT_FOUND		// I2C device not found
} ERR_I2C_E;

typedef ERR_I2C_E (*_init) (UINT32 phy_id, UINT32 freq, UINT32 port);
typedef ERR_I2C_E (*_start) (UINT32 phy_id, UINT8 addr, BOOLEAN rw, BOOLEAN ack_en);
typedef ERR_I2C_E (*_write) (UINT32 phy_id, UINT8 *pCmd, UINT32 len, BOOLEAN ack_en, BOOLEAN no_stop);
typedef ERR_I2C_E (*_read) (UINT32 phy_id, UINT8 *pCmd, UINT32 len, BOOLEAN ack_en);
typedef ERR_I2C_E (*_stop) (UINT32 phy_id);
typedef ERR_I2C_E (*_sendack) (UINT32 phy_id);
typedef ERR_I2C_E (*_getack) (UINT32 phy_id);

typedef struct
{
    _init init;
    _start start;
    _write write;
    _read read;
    _stop stop;
    _sendack sendack;
    _getack getack;
} I2C_PHY_FUN;

VOID I2CHandler(UINT32 param);

typedef struct i2c_tag
{
    volatile UINT32 ctl;
    volatile UINT32 Cmd;
    volatile UINT32 div0;
    volatile UINT32 div1;
    volatile UINT32 rst;
    volatile UINT32 cmd_buf;
} I2C_CTL_REG_T;

#define CTL_BASE_INT             0x40200000
#define CTL_BASE_AP_TMR0         0x40220000
#define CTL_BASE_AP_SYS_TMR      0x40230000
#define SYSTIMER_BASE        CTL_BASE_AP_SYS_TMR  //System timer

#define SYS_ALM                         (SYSTIMER_BASE + 0x0000)
#define SYS_CNT0                        (SYSTIMER_BASE + 0x0004)
#define SYS_CTL                         (SYSTIMER_BASE + 0x0008)

#define SYSTEM_CURRENT_CLOCK (*((volatile UINT32 *)SYS_CNT0) & 0xFFFFFFFF)
#define CHIP_REG_OR(reg_addr, value)    (*(volatile UINT32 *)(reg_addr) |= (UINT32)(value))

#define I2C_BUS_MAX 6
#define I2C_ID_MAX 6

#define I2C_PORT_NUM 0

typedef struct
{
    UINT32 phy_id;
    UINT32 base_addr;
} I2C_BASE_INFO;

typedef struct
{
    UINT32 phy_id;
    UINT32 sda_pin;
    UINT32 scl_pin;
} I2C_GPIO_INFO;

typedef struct
{
    UINT32 logic_id;
    UINT32 phy_id;
    UINT32 port_id;
    I2C_PHY_FUN *phy_fun;
} I2C_PHY_CFG;

#define GR_GEN0                         (0x71300000 + 0x0000)
#define AON_APB_EB0                     (0X402E0000 + 0X0000)
#define GEN0_I2C0_EN                     BIT_8
#define GEN0_I2C1_EN                     BIT_9
#define GEN0_I2C2_EN                     BIT_10
#define GEN0_I2C3_EN                     BIT_11
#define GEN0_I2C4_EN                     BIT_12
#define AON_I2C_EN                       BIT_31

#endif