#include "config.h"
#include "gpio.h"
#include "clock.h"
#include "global_config.h"
#include "jump_function.h"
#include "pwrmgr.h"
#include "mcu.h"
#include "gpio.h"
#include "log.h"
#include "utils/utils.h"
#include "rf_phy_driver.h"
#include "flash.h"
#include "display/display.h"

extern void init_config(void);
extern void app_osal_init(void);
extern void hal_rom_boot_init(void);

#define LARGE_HEAP_SIZE (4 * 1024)
ALIGN4_U8 g_largeHeap[LARGE_HEAP_SIZE];

volatile uint8 g_clk32K_config;
volatile sysclk_t g_spif_clk_config;

static void hal_low_power_io_init(void)
{
    //========= disable all gpio pullup/down to preserve juice
    const ioinit_cfg_t ioInit[] = {
        {GPIO_P00, GPIO_PULL_UP}, // Green LED
        {GPIO_P01, GPIO_PULL_DOWN},
        {GPIO_P02, GPIO_PULL_DOWN},
        {GPIO_P03, GPIO_FLOATING},
        {GPIO_P07, GPIO_FLOATING},
        {GPIO_P09, GPIO_PULL_UP},  // TX1
        {GPIO_P10, GPIO_PULL_UP},  // RX1
        {GPIO_P11, GPIO_FLOATING}, // Capacitive Key
        {GPIO_P14, GPIO_FLOATING}, // Maybe ADC Vbatt
        {GPIO_P15, GPIO_FLOATING},
        {GPIO_P16, GPIO_FLOATING},
        {GPIO_P17, GPIO_FLOATING},
        {GPIO_P18, GPIO_FLOATING},
        {GPIO_P20, GPIO_FLOATING},
        {GPIO_P23, GPIO_FLOATING},
        {GPIO_P24, GPIO_FLOATING}, // Display RESET
        {GPIO_P25, GPIO_FLOATING}, // Display RS
        {GPIO_P26, GPIO_FLOATING},
        {GPIO_P31, GPIO_FLOATING}, // Display CS
        {GPIO_P32, GPIO_FLOATING}, // Display SDA
        {GPIO_P33, GPIO_FLOATING},
        {GPIO_P34, GPIO_FLOATING},  // Display SCL
        {GPIO_P02, GPIO_FLOATING},
        {GPIO_P03, GPIO_FLOATING},
        {GPIO_P07, GPIO_FLOATING},
        {GPIO_P09, GPIO_FLOATING},
        {GPIO_P10, GPIO_FLOATING},
        {GPIO_P11, GPIO_FLOATING},
        {GPIO_P14, GPIO_FLOATING},
        {GPIO_P15, GPIO_FLOATING},
        {GPIO_P18, GPIO_FLOATING},
        {GPIO_P20, GPIO_FLOATING},
        {GPIO_P34, GPIO_FLOATING},
    };

    for (uint8_t i = 0; i < sizeof(ioInit) / sizeof(ioinit_cfg_t); i++)
        hal_gpio_pull_set(ioInit[i].pin, ioInit[i].type);

    DCDC_CONFIG_SETTING(0x0a);
    DCDC_REF_CLK_SETTING(1);
    DIG_LDO_CURRENT_SETTING(1);
#if defined(__GNUC__)
#if 1
	hal_pwrmgr_RAM_retention(RET_SRAM0 | RET_SRAM1 | RET_SRAM2);
#else
    extern uint32 g_irqstack_top;
    // Check IRQ STACK (1KB) location

    /*
        if ((uint32_t) &g_irqstack_top > 0x1fffc000) {
            hal_pwrmgr_RAM_retention(RET_SRAM0 | RET_SRAM1 | RET_SRAM2);
        } else
    */
    if ((uint32_t)&g_irqstack_top > 0x1fff8000)
    {
        hal_pwrmgr_RAM_retention(RET_SRAM0 | RET_SRAM1);
    }
    else
    {
        hal_pwrmgr_RAM_retention(RET_SRAM0); // RET_SRAM0|RET_SRAM1|RET_SRAM2
    }
#endif
#else
#if DEBUG_INFO || SDK_VER_RELEASE_ID != 0x03010102
    hal_pwrmgr_RAM_retention(RET_SRAM0 | RET_SRAM1); // RET_SRAM0|RET_SRAM1|RET_SRAM2
#else
    hal_pwrmgr_RAM_retention(RET_SRAM0 | RET_SRAM1); // RET_SRAM0|RET_SRAM1|RET_SRAM2
#endif
#endif
    hal_pwrmgr_RAM_retention_set();
    subWriteReg(0x4000f014, 26, 26, 1); // hal_pwrmgr_LowCurrentLdo_enable();
                                        // hal_pwrmgr_LowCurrentLdo_disable();
}

static void hal_rfphy_init(void)
{
    // Watchdog_Init(NULL);
    //============config the txPower
    g_rfPhyTxPower = RF_PHY_TX_POWER_0DBM;
    //============config BLE_PHY TYPE
    g_rfPhyPktFmt = PKT_FMT_BLE1M;
    //============config RF Frequency Offset
    g_rfPhyFreqOffSet = RF_PHY_FREQ_FOFF_00KHZ; //	hal_rfPhyFreqOff_Set();
    //============config xtal 16M cap
    XTAL16M_CAP_SETTING(0x09); //	hal_xtal16m_cap_Set();
    XTAL16M_CURRENT_SETTING(0x01);

    hal_rc32k_clk_tracking_init();
    { /* hal_rom_boot_init() */
        extern void efuse_init(void);
        efuse_init();
        typedef void (*my_function)(void);
        my_function pFunc = (my_function)(0xa2e1);
        // ble_main();
        pFunc();
    }
    NVIC_SetPriority((IRQn_Type)BB_IRQn, IRQ_PRIO_REALTIME);
    NVIC_SetPriority((IRQn_Type)TIM1_IRQn, IRQ_PRIO_HIGH); // ll_EVT
    NVIC_SetPriority((IRQn_Type)TIM2_IRQn, IRQ_PRIO_HIGH); // OSAL_TICK
    NVIC_SetPriority((IRQn_Type)TIM4_IRQn, IRQ_PRIO_HIGH); // LL_EXA_ADV
}

static void hal_init(void)
{
    hal_low_power_io_init();
    clk_init(g_system_clk); // system init
    hal_rtc_clock_config((CLK32K_e)g_clk32K_config);
    hal_pwrmgr_init();
    // g_system_clk, SYS_CLK_DLL_64M, SYS_CLK_RC_32M / XFRD_FCMD_READ_QUAD, XFRD_FCMD_READ_DUAL
    hal_spif_cache_init(SYS_CLK_DLL_64M, XFRD_FCMD_READ_DUAL);
    hal_gpio_init();
    LOG_INIT();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(void)
{
    g_system_clk = SYS_CLK_XTAL_16M; // SYS_CLK_XTAL_16M, SYS_CLK_DBL_32M, SYS_CLK_DLL_64M
    g_clk32K_config = CLK_32K_RCOSC; // CLK_32K_XTAL, CLK_32K_RCOSC

    drv_irq_init();
    init_config();

#if (HOST_CONFIG & OBSERVER_CFG)
    extern void ll_patch_advscan(void);
#else
    extern void ll_patch_slave(void);
    ll_patch_slave();
#endif

    hal_rfphy_init();
    hal_init();

#if 0 // def STACK_MAX_SRAM
        extern uint32 g_stack;
    __set_MSP((uint32_t)(&g_stack));
#endif

    device_init();
    display_init();
    app_osal_init();

    return 0;
}