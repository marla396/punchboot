#include <board.h>
#include <io.h>
#include <plat/imx6ul/gpt.h>

static struct gp_timer platform_timer;

uint32_t plat_get_us_tick(void) 
{
    return gp_timer_get_tick(&platform_timer);
}

uint32_t plat_early_init(void)
{
    uint32_t reg;

    platform_timer.base = GP_TIMER1_BASE;
    gp_timer_init(&platform_timer);


    /**
     * TODO: Some imx6ul can run at 696 MHz and some others at 528 MHz
     *   implement handeling of that.
     *
     */

    /*** Configure ARM Clock ***/
    reg = pb_readl(0x020C400C);
    /* Select step clock, so we can change arm PLL */
    pb_writel(reg | (1<<2), 0x020C400C);


    /* Power down */
    pb_writel((1<<12) , 0x020C8000);

    /* Configure divider and enable */
    /* f_CPU = 24 MHz * 88 / 4 = 528 MHz */
    pb_writel((1<<13) | 88, 0x020C8000);


    /* Wait for PLL to lock */
    while (!(pb_readl(0x020C8000) & (1<<31)))
        asm("nop");

    /* Select re-connect ARM PLL */
    pb_writel(reg & ~(1<<2), 0x020C400C);
    
    /*** End of ARM Clock config ***/


    /* Ungate all clocks */
    pb_writel(0xFFFFFFFF, 0x020C4000+0x68); /* Ungate usdhc clk*/
    pb_writel(0xFFFFFFFF, 0x020C4000+0x6C); /* Ungate usdhc clk*/
    pb_writel(0xFFFFFFFF, 0x020C4000+0x70); /* Ungate usdhc clk*/
    pb_writel(0xFFFFFFFF, 0x020C4000+0x74); /* Ungate usdhc clk*/
    pb_writel(0xFFFFFFFF, 0x020C4000+0x78); /* Ungate usdhc clk*/
    pb_writel(0xFFFFFFFF, 0x020C4000+0x7C); /* Ungate usdhc clk*/
    pb_writel(0xFFFFFFFF, 0x020C4000+0x80); /* Ungate usdhc clk*/

    return PB_OK;
}

