#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/SprdGpio.h>

#include "I2C.h"

SPRD_GPIO *gSprdGpio;

#define I2C_WAIT_INT                                                  \
{                                                                     \
    timetick = SYSTEM_CURRENT_CLOCK;                                  \
    while (g_wait_i2c_int_flag)                                       \
    {                                                                 \
        if ((SYSTEM_CURRENT_CLOCK - timetick) >= g_i2c_timeout)       \
        {                                                             \
            if(ERR_I2C_NONE == ret_value)                             \
            {                                                         \
                ret_value = ERR_I2C_INT_TIMEOUT;                      \
            }                                                         \
            break;                                                    \
        }                                                             \
    }                                                                 \
    g_wait_i2c_int_flag = 1;                                          \
}

#define I2C_WAIT_ACK                                                  \
{                                                                     \
    timetick = SYSTEM_CURRENT_CLOCK;                                  \
    while(ptr->cmd & I2CCMD_ACK)                                      \
    {                                                                 \
        if ((SYSTEM_CURRENT_CLOCK - timetick) >= g_i2c_timeout)       \
        {                                                             \
            if(ERR_I2C_NONE == ret_value)                             \
            {                                                         \
                ret_value = ERR_I2C_ACK_TIMEOUT;                      \
            }                                                         \
            break;                                                    \
        }                                                             \
    }                                                                 \
}

VOID MS_Delay(UINT32 ticks)
{
	int i;
	for(i=0; i<10*ticks; i++);
}	

volatile BOOLEAN    g_wait_i2c_int_flag;
volatile BOOLEAN    g_i2c_open_flag=FALSE;
volatile UINT32     g_i2c_timeout = 10; //unit is ms

VOID I2CHandler(UINT32 param)
{
    param = param; // avoid compiler warning

    /* set i2c flag  */    
    g_wait_i2c_int_flag = FALSE;
    
    while((*(volatile UINT32*)(I2C_CMD)) & I2CCMD_BUS);
    
    /* clear i2c int  */
    CHIP_REG_OR(I2C_CMD, I2CCMD_INT_ACK);    
}

UINT32 ChipGetAPBClk(VOID)
{
    return ARM_CLK_26M;
}

ERR_I2C_E I2C_SetSCLclk(UINT32 freq)
{
    UINT32 APB_clk,i2c_dvd;
    
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;

    ASSERT(freq > 0);
    ASSERT(g_i2c_open_flag);

    APB_clk= ChipGetAPBClk();
    i2c_dvd=APB_clk/(4*freq)-1;

    ptr->div0=(UINT16)(i2c_dvd & 0xffff);
    ptr->div1=(UINT16)(i2c_dvd>>16);

    g_i2c_timeout = I2C_TIMEOUT_FACTOR / (freq);
    
    if(g_i2c_timeout < 2)
        g_i2c_timeout = 2;
    //g_i2c_timeout will be changed according I2C frequency
    
    return ERR_I2C_NONE;
     
      
}

ERR_I2C_E I2C_Init(UINT32 freq)
{    
    /*SC8810 use IIC1 for sensor init, but befoe SC8810 all chip use
    IIC0 for sensor init. IIC1 use bit29 for Clock enable.
    */
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;

    freq*=1000;
    ASSERT (freq > 0);

    g_wait_i2c_int_flag = TRUE;
    g_i2c_open_flag=TRUE;

    ptr->rst = BIT_0;//why reset
    ptr->ctl &= ~(I2CCTL_EN);//you must first disable i2c module then change clock  
    ptr->ctl &= ~(I2CCTL_IE);
    ptr->ctl &= ~(I2CCTL_CMDBUF_EN);

    I2C_SetSCLclk(freq);

    CHIP_REG_OR(I2C_CTL, (I2CCTL_IE | I2CCTL_EN));

     //Clear I2C int
    CHIP_REG_OR(I2C_CMD, I2CCMD_INT_ACK); 
   
    return ERR_I2C_NONE; 
}

UINT32 I2C_GetSCLclk(VOID)
{
    UINT32 APB_clk,i2c_dvd,freq;
    
    volatile I2C_CTL_REG_T *ptr = (I2C_CTL_REG_T *)I2C_BASE;

    ASSERT(g_i2c_open_flag);

    APB_clk= ChipGetAPBClk();

    i2c_dvd=((ptr->div1)<<16)|(ptr->div0);

    freq=APB_clk/(4*(i2c_dvd+1));


    return freq;

}

ERR_I2C_E I2C_WriteCmd(UINT8 addr,UINT8 command, BOOLEAN ack_en)
{
    volatile UINT32 timetick = 0; 
    volatile UINT32 cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = ERR_I2C_NONE;

    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);
    
    cmd = ((UINT32)addr)<<8;
    cmd = cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->cmd = cmd; 

    I2C_WAIT_INT
    
    I2C_CLEAR_INT
    
    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    cmd = ((UINT32)command)<<8;
    cmd = cmd | I2CCMD_WRITE | I2CCMD_STOP;//send command
    ptr->cmd = cmd; 

    I2C_WAIT_INT
       
    I2C_CLEAR_INT  

    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    return ERR_I2C_NONE;
            
}

ERR_I2C_E I2C_WriteData(UINT8 addr,UINT8 data, BOOLEAN ack_en)
{
    volatile UINT32 timetick = 0; 
    volatile UINT32 cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = ERR_I2C_NONE;

    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);
    
    cmd = ((UINT32)addr)<<8;
    cmd = cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->cmd = cmd; 

    I2C_WAIT_INT
    
    I2C_CLEAR_INT
    
    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    data = ((UINT32)data)<<8;
    data = data | I2CCMD_WRITE | I2CCMD_STOP;//send command
    ptr->cmd = data; 

    I2C_WAIT_INT
       
    I2C_CLEAR_INT  

    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    return ERR_I2C_NONE;
            
}

ERR_I2C_E I2C_ReadCmd(UINT8 addr,UINT8 *pCmd, BOOLEAN ack_en)
{
    volatile UINT32 timetick = 0; 
    volatile UINT32 cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = ERR_I2C_NONE;
    
    ASSERT(NULL != pCmd);
    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);

    cmd = ((UINT32)(addr|I2C_READ_BIT))<<8;
    cmd = cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->cmd = cmd; 

    I2C_WAIT_INT
           
    I2C_CLEAR_INT  

    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    cmd = I2CCMD_READ | I2CCMD_STOP | I2CCMD_TX_ACK;
    ptr->cmd = cmd;
     
    I2C_WAIT_INT
       
    I2C_CLEAR_INT  


    *pCmd=(UINT8)((ptr->cmd)>>8);

    return ERR_I2C_NONE;
}

UINT32   ret_value = ERR_I2C_NONE;
volatile UINT32 timetick = 0; 

ERR_I2C_E I2C_WriteCmdArr(UINT8 addr, UINT8 *pCmd, UINT32 len, BOOLEAN ack_en)
{
    volatile UINT32 curtime = 0; 	
    volatile UINT32 i = 0;
    volatile UINT32 cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    
    ASSERT(NULL != pCmd);
    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);
    
    cmd = ((UINT32)addr)<<8;
    cmd = cmd | I2CCMD_START | I2CCMD_WRITE ;//send device address 0x9824
    ptr->cmd = cmd; 

    I2C_WAIT_INT		
    
    I2C_CLEAR_INT   
    
    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    for(i=0;i<len;i++)
    {
        cmd = ((UINT32)pCmd[i])<<8;
        if(i== (len-1))     
            cmd = cmd | I2CCMD_WRITE | I2CCMD_STOP;//send command
        else
            cmd = cmd | I2CCMD_WRITE ;

        ptr->cmd = cmd; 

        I2C_WAIT_INT
           
        I2C_CLEAR_INT    

        //check ACK
        if(ack_en)
        {
            I2C_WAIT_ACK
        }

    }

    return ret_value;
}

ERR_I2C_E I2C_ReadCmdArr(UINT8 addr, UINT8 *pCmd, UINT32 len,BOOLEAN ack_en )
{
    volatile UINT32 timetick = 0; 
    volatile UINT32 i = 0;
    volatile UINT32 cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = ERR_I2C_NONE;
    
    ASSERT(NULL !=pCmd );
    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);

    cmd = ((UINT32)(addr|I2C_READ_BIT))<<8;
    cmd = cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->cmd = cmd; 

    I2C_WAIT_INT
           
    I2C_CLEAR_INT
    
    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    for(i=0;i<len;i++)
    {
        if(i<len-1)
            cmd = I2CCMD_READ;  // I2CCMD_READ|I2CCMD_TX_ACK;
        else
            cmd = I2CCMD_READ|I2CCMD_STOP|I2CCMD_TX_ACK;

        ptr->cmd = cmd;

        I2C_WAIT_INT
                   
        I2C_CLEAR_INT   

        pCmd[i] = (UINT8)((ptr->cmd)>>8);
    }

    return ERR_I2C_NONE;
}

extern const I2C_BASE_INFO __i2c_base_info[I2C_BUS_MAX];

UINT32 __I2C_PHY_GetBase (UINT32 phy_id)
{
    INT32 i;
    UINT32 ret = 0;

    for (i = 0; i < I2C_BUS_MAX; i++)
    {
        if (phy_id == (UINT32) __i2c_base_info[i].phy_id)
        {
            ret = (UINT32) __i2c_base_info[i].base_addr;
            
            switch(phy_id)
            {
		case 0:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C0_EN));
			break;	
		case 1:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C1_EN));
			break;	
		case 2:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C2_EN));
			break;
		case 3:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C3_EN));
			break;	
		case 4:
			CHIP_REG_OR (GR_GEN0, (GEN0_I2C4_EN));
			break;
                case 5:
                        CHIP_REG_OR (AON_APB_EB0, (AON_I2C_EN));
                        break;
	    }
            break;
        }
    }

    return ret;
}

ERR_I2C_E __I2C_PHY_SetSCL (UINT32 phy_id, UINT32 freq)
{
    UINT32 APB_clk,i2c_dvd;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    APB_clk= 26*1000*1000;//CHIP_GetAPBClk();
    i2c_dvd=APB_clk/ (4*freq)-1;
    ptr->div0= (UINT16) (i2c_dvd & 0xffff);
    ptr->div1= (UINT16) (i2c_dvd>>16);
    g_i2c_timeout = I2C_TIMEOUT_FACTOR / (freq);

    if (g_i2c_timeout < 2)
    {
        g_i2c_timeout = 2;
    }

    return ERR_I2C_NONE;
}

ERR_I2C_E __I2C_PHY_SetPort (UINT32 port)
{
    return ERR_I2C_NONE;
}


ERR_I2C_E I2C_PHY_ControlInit_V0 (UINT32 phy_id, UINT32 freq, UINT32 port)
{
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);

    ptr->rst = BIT_0;//why reset
    ptr->ctl &= ~ (I2CCTL_EN); //you must first disable i2c module then change clock
    ptr->ctl &= ~ (I2CCTL_IE);
    __I2C_PHY_SetSCL (phy_id, freq);

    if (I2C_PORT_NUM < port)
    {
        //ASSERT (0);/*assert to do*/
    }

    __I2C_PHY_SetPort (port);

    ptr->ctl |=  (I2CCTL_IE | I2CCTL_EN);
    //Clear I2C int
    ptr->cmd &= ~ (I2CCMD_INT_ACK);
        
    DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_ControlInit_V0: freq=%d, port=%d", freq, port));
    return ERR_I2C_NONE;
}

ERR_I2C_E I2C_PHY_StartBus_V0 (UINT32 phy_id, UINT8 addr, BOOLEAN rw, BOOLEAN ack_en)
{
    UINT32 timetick = 0;
    UINT32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = ERR_I2C_NONE;

    if (rw)
    {
        /*read cmd*/
        cmd = ( (UINT32) (addr |0x1)) <<8;
    }
    else
    {
        /*write cmd*/
        cmd = ( (UINT32) addr) <<8;
    }

    cmd = cmd | I2CCMD_START | I2CCMD_WRITE;
    DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_StartBus_V0: cmd=%x", cmd));
    ptr->cmd = cmd;
    I2C_WAIT_INT
    I2C_CLEAR_INT

    //check ACK
    if (ack_en)
    {
        I2C_WAIT_ACK
    }

    return ERR_I2C_NONE;
}

ERR_I2C_E I2C_PHY_WriteBytes_V0 (UINT32 phy_id, UINT8 *pCmd, UINT32 len, BOOLEAN ack_en, BOOLEAN no_stop)
{
    UINT32 timetick = 0;
    UINT32 i = 0;
    UINT32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = ERR_I2C_NONE;

    for (i=0; i<len; i++)
    {
        cmd = ( (UINT32) pCmd[i]) <<8;
        cmd = cmd | I2CCMD_WRITE ;

        if ( (i== (len-1)) && (!no_stop))
        {
            cmd = cmd | I2CCMD_STOP;
        }

        ptr->cmd = cmd;
        DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_WriteBytes_V0: cmd=%x", cmd));
        I2C_WAIT_INT
        I2C_CLEAR_INT

        //check ACK
        if (ack_en)
        {
            I2C_WAIT_ACK
        }
    }

    return ret_value;
}


ERR_I2C_E I2C_PHY_ReadBytes_V0 (UINT32 phy_id, UINT8 *pCmd, UINT32 len, BOOLEAN ack_en)
{
    UINT32 timetick = 0;
    UINT32 i = 0;
    UINT32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = ERR_I2C_NONE;

    for (i=0; i<len; i++)
    {
        cmd = I2CCMD_READ; /*FIXME |I2CCMD_TX_ACK;*/

        if (i== (len-1))
        {
            cmd = cmd |I2CCMD_STOP |I2CCMD_TX_ACK;
        }

        ptr->cmd = cmd;
        DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_ReadBytes_V0: cmd=%x", cmd));
        I2C_WAIT_INT
        I2C_CLEAR_INT
        pCmd[i] = (UINT8) ( (ptr->cmd) >>8);
    }

    return ret_value;
}

ERR_I2C_E I2C_PHY_StopBus_V0 (UINT32 phy_id)
{
    UINT32 timetick = 0;
    UINT32 cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = ERR_I2C_NONE;
    cmd = I2CCMD_STOP;
    ptr->cmd = cmd;
    I2C_WAIT_INT
    I2C_CLEAR_INT
    return ERR_I2C_NONE;
}

ERR_I2C_E I2C_PHY_SendACK_V0 (UINT32 phy_id)
{
    return ERR_I2C_NONE;
}

ERR_I2C_E I2C_PHY_GetACK_V0 (UINT32 phy_id)
{
    return ERR_I2C_NONE;
}

I2C_PHY_FUN phy_fun_v0 = {
        .init = I2C_PHY_ControlInit_V0,
        .start = I2C_PHY_StartBus_V0,
        .stop = I2C_PHY_StopBus_V0,
        .read = I2C_PHY_ReadBytes_V0,
        .write = I2C_PHY_WriteBytes_V0,
        .sendack = I2C_PHY_SendACK_V0,
        .getack = I2C_PHY_GetACK_V0,
};

extern I2C_PHY_FUN phy_fun_v0;

const I2C_PHY_CFG __i2c_phy_cfg[I2C_ID_MAX] =
{
    /*Note: Only port 1 is pulled up internal, other port should be pulled up external*/
    /*logic id, controller id, port id, method*/
    {0, 0, 1, &phy_fun_v0}, /*hw i2c controller0*/
    {1, 1, 1, &phy_fun_v0}, /*hw i2c controller1*/
    {2, 2, 1, &phy_fun_v0}, /*hw i2c controller2*/
    {3, 3, 1, &phy_fun_v0}, /*hw i2c controller3*/
    {4, 4, 1, &phy_fun_v0}, /*hw i2c controller4*/
    {5, 5, 1, &phy_fun_v0}, /*hw i2c controller5*/
    //{4, 1, 1, &phy_fun_v1} /*sw simulation i2c controller1, port 1*/
};

const I2C_BASE_INFO __i2c_base_info[I2C_BUS_MAX] =
{
    /*hw controller id, base address*/
    {0, 0x70500000},/*hw i2c controller0, register base*/
    {1, 0x70600000},/*hw i2c controller1, register base*/
    {2, 0x70700000},/*hw i2c controller2, register base*/
    {3, 0x70800000},/*hw i2c controller3, register base*/
    {4, 0x70900000},/*hw i2c controller4, register base*/
    {5, 0x40080000},/*hw i2c controller5, register base*/

	//{1, 0} /*sw i2c controller1, no register base*/
};

EFI_STATUS
EFIAPI
I2CInit (VOID)
{
  DEBUG((EFI_D_INFO, "Initializing I2C\n"));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
InitI2CDriver (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gSprdGpioProtocolGuid, NULL, (VOID *)&gSprdGpio);

  I2CInit();

  DEBUG((EFI_D_INFO, "Initializing Spreadtrum I2C\n"));
  return EFI_SUCCESS;
}