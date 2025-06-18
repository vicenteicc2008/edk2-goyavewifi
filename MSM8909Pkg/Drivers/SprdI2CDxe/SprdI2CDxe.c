#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseMemoryLib.h>

#include "I2C.h"

#include <Protocol/SprdGpio.h>
#include <Protocol/SprdI2C.h>

SPRD_GPIO *gSprdGpio;

#define I2C_WAIT_INT                                                  \
{                                                                     \
    Timetick = SYSTEM_CURRENT_CLOCK;                                  \
    while (g_wait_i2c_int_flag)                                       \
    {                                                                 \
        if ((SYSTEM_CURRENT_CLOCK - Timetick) >= g_i2c_timeout)       \
        {                                                             \
            if(EFI_SUCCESS == ret_value)                             \
            {                                                         \
                ret_value = EFI_TIMEOUT;                      \
            }                                                         \
            break;                                                    \
        }                                                             \
    }                                                                 \
    g_wait_i2c_int_flag = 1;                                          \
}

#define I2C_WAIT_ACK                                                  \
{                                                                     \
    Timetick = SYSTEM_CURRENT_CLOCK;                                  \
    while(ptr->Cmd & I2CCMD_ACK)                                      \
    {                                                                 \
        if ((SYSTEM_CURRENT_CLOCK - Timetick) >= g_i2c_timeout)       \
        {                                                             \
            if(EFI_SUCCESS == ret_value)                             \
            {                                                         \
                ret_value = EFI_TIMEOUT;                      \
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

EFI_STATUS
I2C_SetSCLclk (
  IN UINT32 Frequency
  )
{
  volatile I2C_CTL_REG_T *I2cRegs;
  UINT32 ApbClk;
  UINT32 Divider;

  if (Frequency == 0 || !g_i2c_open_flag) {
    return EFI_INVALID_PARAMETER;
  }

  I2cRegs = (volatile I2C_CTL_REG_T *)I2C_BASE;
  ApbClk  = ChipGetAPBClk();

  if (ApbClk == 0 || Frequency > (ApbClk / 4)) {
    return EFI_UNSUPPORTED;
  }

  Divider = ApbClk / (4 * Frequency);
  if (Divider > 0) {
    Divider -= 1;
  }

  I2cRegs->div0 = (UINT16)(Divider & 0xFFFF);
  I2cRegs->div1 = (UINT16)(Divider >> 16);

  g_i2c_timeout = I2C_TIMEOUT_FACTOR / Frequency;
  if (g_i2c_timeout < 2) {
    g_i2c_timeout = 2;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
I2C_Init (
  IN UINT32 FrequencyKhz
  )
{
  volatile I2C_CTL_REG_T *I2cRegs;
  UINT32 FrequencyHz;

  if (FrequencyKhz == 0) {
    return EFI_INVALID_PARAMETER;
  }

  I2cRegs = (volatile I2C_CTL_REG_T *)I2C_BASE;
  FrequencyHz = FrequencyKhz * 1000;

  g_wait_i2c_int_flag = TRUE;
  g_i2c_open_flag = TRUE;

  // Reset I2C module
  I2cRegs->rst = BIT_0;

  // Disable I2C module before reconfiguration
  I2cRegs->ctl &= ~(I2CCTL_EN | I2CCTL_IE | I2CCTL_CMDBUF_EN);

  // Configure clock
  I2C_SetSCLclk(FrequencyHz);

  // Enable I2C and Interrupt
  MmioOr32(I2C_CTL, (I2CCTL_IE | I2CCTL_EN));

  // Acknowledge and clear any pending interrupt
  MmioOr32(I2C_CMD, I2CCMD_INT_ACK);

  return EFI_SUCCESS;
}

UINT32
I2C_GetSCLclk (
  VOID
  )
{
  volatile I2C_CTL_REG_T *I2cRegs;
  UINT32 ApbClk;
  UINT32 Divider;
  UINT32 Frequency;

  if (!g_i2c_open_flag) {
    return 0; // O podrías retornar un valor reservado para error si lo defines
  }

  I2cRegs = (volatile I2C_CTL_REG_T *)I2C_BASE;
  ApbClk  = ChipGetAPBClk();

  Divider = ((UINT32)(I2cRegs->div1) << 16) | I2cRegs->div0;

  if (Divider == 0xFFFFFFFF || Divider == 0) {
    return 0;
  }

  Frequency = ApbClk / (4 * (Divider + 1));

  return Frequency;
}

EFI_STATUS
I2C_WriteCmd(UINT8 Addr, UINT8 Command, BOOLEAN Ack_en)
{
    volatile UINT32 Timetick = 0; 
    volatile UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = EFI_SUCCESS;

    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);
    
    Cmd = ((UINT32)Addr)<<8;
    Cmd = Cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->Cmd = Cmd; 

    I2C_WAIT_INT
    
    I2C_CLEAR_INT
    
    //check ACK
    if(Ack_en)
    {
        I2C_WAIT_ACK
    }

    Cmd = ((UINT32)Command)<<8;
    Cmd = Cmd | I2CCMD_WRITE | I2CCMD_STOP;//send command
    ptr->Cmd = Cmd; 

    I2C_WAIT_INT
       
    I2C_CLEAR_INT  

    //check ACK
    if(Ack_en)
    {
        I2C_WAIT_ACK
    }

    return EFI_SUCCESS;
            
}

ERR_I2C_E I2C_WriteData(UINT8 addr,UINT8 data, BOOLEAN ack_en)
{
    volatile UINT32 Timetick = 0; 
    volatile UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = EFI_SUCCESS;

    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);
    
    Cmd = ((UINT32)addr)<<8;
    Cmd = Cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->Cmd = Cmd; 

    I2C_WAIT_INT
    
    I2C_CLEAR_INT
    
    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    data = ((UINT32)data)<<8;
    data = data | I2CCMD_WRITE | I2CCMD_STOP;//send command
    ptr->Cmd = data; 

    I2C_WAIT_INT
       
    I2C_CLEAR_INT  

    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    return EFI_SUCCESS;
            
}

ERR_I2C_E I2C_ReadCmd(UINT8 addr,UINT8 *pCmd, BOOLEAN ack_en)
{
    volatile UINT32 Timetick = 0; 
    volatile UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = EFI_SUCCESS;
    
    ASSERT(NULL != pCmd);
    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);

    Cmd = ((UINT32)(addr|I2C_READ_BIT))<<8;
    Cmd = Cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->Cmd = Cmd; 

    I2C_WAIT_INT
           
    I2C_CLEAR_INT  

    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    Cmd = I2CCMD_READ | I2CCMD_STOP | I2CCMD_TX_ACK;
    ptr->Cmd = Cmd;
     
    I2C_WAIT_INT
       
    I2C_CLEAR_INT  


    *pCmd=(UINT8)((ptr->Cmd)>>8);

    return EFI_SUCCESS;
}

UINT32   ret_value = EFI_SUCCESS;
volatile UINT32 Timetick = 0; 

ERR_I2C_E I2C_WriteCmdArr(UINT8 addr, UINT8 *pCmd, UINT32 len, BOOLEAN ack_en)
{
    volatile UINT32 curtime = 0; 	
    volatile UINT32 i = 0;
    volatile UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    
    ASSERT(NULL != pCmd);
    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);
    
    Cmd = ((UINT32)addr)<<8;
    Cmd = Cmd | I2CCMD_START | I2CCMD_WRITE ;//send device address 0x9824
    ptr->Cmd = Cmd; 

    I2C_WAIT_INT		
    
    I2C_CLEAR_INT   
    
    //check ACK
    if(ack_en)
    {
        I2C_WAIT_ACK
    }

    for(i=0;i<len;i++)
    {
        Cmd = ((UINT32)pCmd[i])<<8;
        if(i== (len-1))     
            Cmd = Cmd | I2CCMD_WRITE | I2CCMD_STOP;//send command
        else
            Cmd = Cmd | I2CCMD_WRITE ;

        ptr->Cmd = Cmd; 

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
    volatile UINT32 Timetick = 0; 
    volatile UINT32 i = 0;
    volatile UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T * ptr = (volatile I2C_CTL_REG_T *)I2C_BASE;
    UINT32   ret_value = EFI_SUCCESS;
    
    ASSERT(NULL !=pCmd );
    ASSERT(g_i2c_open_flag);
    ASSERT(g_i2c_timeout > 0);

    Cmd = ((UINT32)(addr|I2C_READ_BIT))<<8;
    Cmd = Cmd | I2CCMD_START | I2CCMD_WRITE;//send device address
    ptr->Cmd = Cmd; 

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
            Cmd = I2CCMD_READ;  // I2CCMD_READ|I2CCMD_TX_ACK;
        else
            Cmd = I2CCMD_READ|I2CCMD_STOP|I2CCMD_TX_ACK;

        ptr->Cmd = Cmd;

        I2C_WAIT_INT
                   
        I2C_CLEAR_INT   

        pCmd[i] = (UINT8)((ptr->Cmd)>>8);
    }

    return EFI_SUCCESS;
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

    return EFI_SUCCESS;
}

ERR_I2C_E __I2C_PHY_SetPort (UINT32 port)
{
    return EFI_SUCCESS;
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
    ptr->Cmd &= ~ (I2CCMD_INT_ACK);
        
    DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_ControlInit_V0: freq=%d, port=%d", freq, port));
    return EFI_SUCCESS;
}

ERR_I2C_E I2C_PHY_StartBus_V0 (UINT32 phy_id, UINT8 addr, BOOLEAN rw, BOOLEAN ack_en)
{
    UINT32 Timetick = 0;
    UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = EFI_SUCCESS;

    if (rw)
    {
        /*read cmd*/
        Cmd = ( (UINT32) (addr |0x1)) <<8;
    }
    else
    {
        /*write cmd*/
        Cmd = ( (UINT32) addr) <<8;
    }

    Cmd = Cmd | I2CCMD_START | I2CCMD_WRITE;
    DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_StartBus_V0: Cmd=%x", Cmd));
    ptr->Cmd = Cmd;
    I2C_WAIT_INT
    I2C_CLEAR_INT

    //check ACK
    if (ack_en)
    {
        I2C_WAIT_ACK
    }

    return EFI_SUCCESS;
}

ERR_I2C_E I2C_PHY_WriteBytes_V0 (UINT32 phy_id, UINT8 *pCmd, UINT32 len, BOOLEAN ack_en, BOOLEAN no_stop)
{
    UINT32 Timetick = 0;
    UINT32 i = 0;
    UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = EFI_SUCCESS;

    for (i=0; i<len; i++)
    {
        Cmd = ( (UINT32) pCmd[i]) <<8;
        Cmd = Cmd | I2CCMD_WRITE ;

        if ( (i== (len-1)) && (!no_stop))
        {
            Cmd = Cmd | I2CCMD_STOP;
        }

        ptr->Cmd = Cmd;
        DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_WriteBytes_V0: Cmd=%x", Cmd));
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
    UINT32 Timetick = 0;
    UINT32 i = 0;
    UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = EFI_SUCCESS;

    for (i=0; i<len; i++)
    {
        Cmd = I2CCMD_READ; /*FIXME |I2CCMD_TX_ACK;*/

        if (i== (len-1))
        {
            Cmd = Cmd |I2CCMD_STOP |I2CCMD_TX_ACK;
        }

        ptr->Cmd = Cmd;
        DEBUG((EFI_D_ERROR, "[IIC DRV:]I2C_PHY_ReadBytes_V0: Cmd=%x", Cmd));
        I2C_WAIT_INT
        I2C_CLEAR_INT
        pCmd[i] = (UINT8) ( (ptr->Cmd) >>8);
    }

    return ret_value;
}

ERR_I2C_E I2C_PHY_StopBus_V0 (UINT32 phy_id)
{
    UINT32 Timetick = 0;
    UINT32 Cmd = 0;
    volatile I2C_CTL_REG_T *ptr = (volatile I2C_CTL_REG_T *) __I2C_PHY_GetBase (phy_id);
    UINT32   ret_value = EFI_SUCCESS;
    Cmd = I2CCMD_STOP;
    ptr->Cmd = Cmd;
    I2C_WAIT_INT
    I2C_CLEAR_INT
    return EFI_SUCCESS;
}

ERR_I2C_E I2C_PHY_SendACK_V0 (UINT32 phy_id)
{
    return EFI_SUCCESS;
}

ERR_I2C_E I2C_PHY_GetACK_V0 (UINT32 phy_id)
{
    return EFI_SUCCESS;
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
InitI2CDriver (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_HANDLE      Handle = NULL;
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gSprdGpioProtocolGuid, NULL, (VOID *)&gSprdGpio);

  I2C_Init(0);

  Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gSprdI2cProtocolGuid, NULL);
  ASSERT_EFI_ERROR(Status);

  DEBUG((EFI_D_INFO, "Initializing Spreadtrum I2C\n"));
  return EFI_SUCCESS;
}