#include "bcm2835.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>

#define RGB565(r,g,b)((r >> 3) << 11 | (g >> 2) << 5 | ( b >> 3))
#define BCM2708SPI
#define ROTATE90

#define RGB565_MASK_RED      0xF800
#define RGB565_MASK_GREEN    0x07E0
#define RGB565_MASK_BLUE     0x001F

#ifdef    ROTATE90
    #define XP    0x021
    #define YP    0x020

    #define XS    0x052
    #define XE    0x053
    #define YS    0x050
    #define YE    0x051

    #define MAX_X    319
    #define MAX_Y    239
#else
    #define XP    0x020
    #define YP    0x021

    #define XS    0x050
    #define XE    0x051
    #define YS    0x052
    #define YE    0x053

    #define MAX_X    239
    #define MAX_Y    319
#endif

#define SPICS  RPI_GPIO_P1_24  //GPIO08
#define SPIRS  RPI_GPIO_P1_22  //GPIO25
#define SPIRST RPI_GPIO_P1_10  //GPIO15
#define SPISCL RPI_GPIO_P1_23  //GPIO11
#define SPISCI RPI_GPIO_P1_19  //GPIO10
#define LCDPWM RPI_GPIO_P1_12  //GPIO18

#define LCD_CS_CLR  bcm2835_gpio_clr(SPICS)
#define LCD_RS_CLR  bcm2835_gpio_clr(SPIRS)
#define LCD_RST_CLR bcm2835_gpio_clr(SPIRST)
#define LCD_SCL_CLR bcm2835_gpio_clr(SPISCL)
#define LCD_SCI_CLR bcm2835_gpio_clr(SPISCI)
#define BACKLIGHT_ON bcm2835_gpio_clr(LCDPWM)

#define LCD_CS_SET  bcm2835_gpio_set(SPICS)
#define LCD_RS_SET  bcm2835_gpio_set(SPIRS)
#define LCD_RST_SET bcm2835_gpio_set(SPIRST)
#define LCD_SCL_SET bcm2835_gpio_set(SPISCL)
#define LCD_SCI_SET bcm2835_gpio_set(SPISCI)
#define BACKLIGHT_OFF bcm2835_gpio_set(LCDPWM)

int color[]={0xf800,0x07e0,0x001f,0xffe0,0xf81f,0xffff,0x07ff,0x0000};

/* Image part */
char *value=NULL;
int hsize=0, vsize=0;

void LCD_WR_REG(int index)
{
    LCD_CS_CLR;
    LCD_RS_CLR;

    bcm2835_spi_transfer(index>>8);
    bcm2835_spi_transfer(index);

    LCD_CS_SET;
}

void LCD_WR_CMD(int index,int val)
{
    LCD_CS_CLR;
    LCD_RS_CLR;

    bcm2835_spi_transfer(index>>8);
    bcm2835_spi_transfer(index);

    LCD_RS_SET;

    bcm2835_spi_transfer(val>>8);
    bcm2835_spi_transfer(val);

    LCD_CS_SET;
}

void inline LCD_WR_DATA(int val)
{
    bcm2835_spi_transfer(val>>8);
    bcm2835_spi_transfer(val);
}

void backlightOn()
{
    BACKLIGHT_ON;
}

void backlightOff()
{
    BACKLIGHT_OFF;
}

void writeDot(int dotX,int dotY,int color)
{
    LCD_WR_CMD(XS,0x0000); // Column address start2
    LCD_WR_CMD(XE,MAX_X); // Column address end2
    LCD_WR_CMD(YS,0x0000); // Row address start2
    LCD_WR_CMD(YE,MAX_Y); // Row address end2

    LCD_WR_CMD(XP,dotX); // Column address start
    LCD_WR_CMD(YP,dotY); // Row address start
        
    LCD_WR_CMD(0x22,color);
}


void lcdInit()
{
    LCD_RST_CLR;
    delay (100);
    LCD_RST_SET;
    delay (100);
    
    LCD_WR_CMD(0x0015, 0x0030);      // Set the internal vcore voltage
    LCD_WR_CMD(0x009A, 0x0010);

    //*************Power On sequence ****************
    LCD_WR_CMD(0x0011, 0x0020);         // DC1[2:0], DC0[2:0], VC[2:0]
    LCD_WR_CMD(0x0010, 0x3428);         // SAP, BT[3:0], AP, DSTB, SLP, STB    
    LCD_WR_CMD(0x0012, 0x0002);         // VREG1OUT voltage
    LCD_WR_CMD(0x0013, 0x1038);         // VDV[4:0] for VCOM amplitude
    delay (4);
    LCD_WR_CMD(0x0012, 0x0012);         // VREG1OUT voltage
    delay (4);
    LCD_WR_CMD(0x0010, 0x3420);         // SAP, BT[3:0], AP, DSTB, SLP, STB    
    LCD_WR_CMD(0x0013, 0x3038);         // VDV[4:0] for VCOM amplitude
    delay (7);
 
    //----------2.8" Gamma  Curve table 2 ----------
    LCD_WR_CMD(0x30, 0x0000);
    LCD_WR_CMD(0x31, 0x0402);
    LCD_WR_CMD(0x32, 0x0307);
    LCD_WR_CMD(0x33, 0x0304);
    LCD_WR_CMD(0x34, 0x0004);
    LCD_WR_CMD(0x35, 0x0401);
    LCD_WR_CMD(0x36, 0x0707);
    LCD_WR_CMD(0x37, 0x0305);
    LCD_WR_CMD(0x38, 0x0610);
    LCD_WR_CMD(0x39, 0x0610);

    LCD_WR_CMD(0x0001, 0x0000);     // set SS and SM bit
    LCD_WR_CMD(0x0002, 0x0300);     // set 1 line inveLCD_RSion
    LCD_WR_CMD(0x0003, 0x1038);     // set GRAM write direction and BGR=1.
    LCD_WR_CMD(0x0008, 0x0808);     // set the back porch and front porch
    LCD_WR_CMD(0x000A, 0x0008);     // FMARK function

    LCD_WR_CMD(0x0060, 0x2700);        // Gate Scan Line
    LCD_WR_CMD(0x0061, 0x0001);        // NDL,VLE, REV

    LCD_WR_CMD(0x0090, 0x013E);
    LCD_WR_CMD(0x0092, 0x0100);
    LCD_WR_CMD(0x0093, 0x0100);

    LCD_WR_CMD(0x00A0, 0x3000);
    LCD_WR_CMD(0x00A3, 0x0010);

    //-------------- Panel Control -------------------//

    LCD_WR_CMD(0x0007, 0x0173);        // 262K color and display ON
}


void lcdTest()
{
    int testColor,testY,testX;
    char colorIndex;
    struct timespec waitTime;
    waitTime.tv_sec = 3;
    waitTime.tv_nsec = 0;
    

    printf("Running test loop\n");
    for(colorIndex=0;colorIndex<8;colorIndex++)
    {
        LCD_WR_CMD(YS,0); // Column address start2
        LCD_WR_CMD(YE,MAX_Y); // Column address end2
        LCD_WR_CMD(XS,0); // Row address start2
        LCD_WR_CMD(XE,MAX_X); // Row address end2
        
        LCD_WR_REG(0x22);
        LCD_CS_CLR;
        LCD_RS_SET;
        
        testColor=color[colorIndex];
        printf("filling with color %X\n",testColor);
        for(testY=0;testY<240;testY++)
        {
            for(testX=0;testX<320;testX++)
            {
                LCD_WR_DATA(testColor);
            }
        }
        
        LCD_CS_SET;
        printf("Waiting 3 seconds...\n");
        
        nanosleep(&waitTime, NULL);
    }
}


void initSpi (void)
{
    bcm2835_gpio_fsel(SPICS,  BCM2835_GPIO_FSEL_OUTP) ;
    bcm2835_gpio_fsel(SPIRS,  BCM2835_GPIO_FSEL_OUTP) ;
    bcm2835_gpio_fsel(SPIRST, BCM2835_GPIO_FSEL_OUTP) ;
    bcm2835_gpio_fsel(LCDPWM, BCM2835_GPIO_FSEL_OUTP) ;

    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_2);
}

void resetScreen(void)
{
    LCD_WR_CMD(XS, 0x0000); // Column address start2
    LCD_WR_CMD(XE, MAX_X ); // Column address end2
    LCD_WR_CMD(YS, 0x0000); // Row address start2
    LCD_WR_CMD(YE, MAX_Y ); // Row address end2

    LCD_WR_REG(0x22);
    LCD_CS_CLR;
    LCD_RS_SET;
}

int writePart(int startX, int startY, int lenX, int lenY, int *part)
{
    return 0;
}

void writeFrame(int *frame)
{
    return;
}

