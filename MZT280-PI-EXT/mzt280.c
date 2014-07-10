#include "bcm2835.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#include "mzt280.h"

#define BCM2708SPI

#define XP    0x020
#define YP    0x021

#define XS    0x050
#define XE    0x051
#define YS    0x052
#define YE    0x053

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

typedef struct colorCoordStruct_s
{
    unsigned int start_x;
    unsigned int start_y;
    unsigned int end_x;
    unsigned int end_y;
    unsigned char start_value;
    unsigned char end_value;
    
} colorCoordStruct_t;

const colorCoordStruct_t RED = 
{
    0,       // Red start is top right corner
    MAX_Y,
    MAX_X,   // Red end is bottom left corner
    0,
    0x1f,    // Red is 5 bits
    0
};

const colorCoordStruct_t GREEN =
{
    MAX_X,   // Green start is bottom left corner
    0,
    0,       // Green end is top right corner
    MAX_Y,
    0x3F,    // Green is 6 bits
    0
};

const colorCoordStruct_t BLUE =
{
    0,       // Blue start is top left corner
    0,
    MAX_X,   // Blue end is bottom right corner
    MAX_Y,
    0x1F,    // Blue is 5 bits
    0
};

unsigned int pixelPart(colorCoordStruct_t color, unsigned int x, unsigned int y)
{
    unsigned int part1, part2;
    
    if ( color.start_x < color.end_x )
    {
        part1 = ( ( color.start_value * ( x - color.start_x ) ) + ( color.end_value * ( color.end_x - x ))) / MAX_X;
    }
    else
    {
        part1 = ( ( color.start_value * ( x - color.end_x ) ) + ( color.end_value * ( color.start_x - x ) ) ) / MAX_X;
        part1 = color.start_value - part1;
    }
    
    if ( color.start_y < color.end_y )
    {
        part2 = ( ( color.start_value * ( y - color.start_y ) ) + ( color.end_value * ( color.end_y - y ))) / MAX_Y;
    }
    else
    {
        part2 = ( ( color.start_value * (  y - color.end_y ) ) + ( color.end_value * ( color.start_y - y ) ) ) / MAX_Y;
        part2 = color.start_value - part2;
    }
    
    return ( part1 + part2 )/2;
}

unsigned int pixelValue(unsigned int x, unsigned int y)
{
    unsigned int outRed,outGreen,outBlue = 0;
    
    outRed = pixelPart(RED,x,y);
    outGreen = pixelPart(GREEN,x,y);
    outBlue = pixelPart(BLUE,x,y);
    
    return RGB565(outRed,outGreen,outBlue);
}



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
    int index;
    LCD_RST_CLR;
    delay (100);
    LCD_RST_SET;
    delay (100);

    LCD_WR_CMD(0x0015, 0x0030);      // Set the internal vcore voltage
    LCD_WR_CMD(0x009A, 0x0010);

    // *************Power On sequence ****************
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

    LCD_WR_CMD(YS,0);
    LCD_WR_CMD(YE,MAX_Y);
    LCD_WR_CMD(XS,0);
    LCD_WR_CMD(XE,MAX_X);

    LCD_WR_REG(0x22);
    LCD_CS_CLR;
    LCD_RS_SET;

    for(index=0;index<(320*240);index++)
    {
        LCD_WR_DATA(0x0000);
    }
    LCD_CS_SET;
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
        LCD_WR_CMD(YS,0); // Column address start
        LCD_WR_CMD(YE,MAX_Y); // Column address end
        LCD_WR_CMD(XS,0); // Row address start
        LCD_WR_CMD(XE,MAX_X); // Row address end

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
    
    printf("Filling with color gradient\n");
    
    LCD_WR_CMD(YS,0); // Column address start
    LCD_WR_CMD(YE,MAX_Y); // Column address end
    LCD_WR_CMD(XS,0); // Row address start
    LCD_WR_CMD(XE,MAX_X); // Row address end

    LCD_WR_REG(0x22);
    LCD_CS_CLR;
    LCD_RS_SET;
    
    for( testY = 0; testY <= MAX_Y; testY ++ )
    {
        for( testX = 0; testX <= MAX_X; testX++ )
        {
            LCD_WR_DATA(pixelValue(testX,testY));
        }
    }
    
    LCD_CS_SET;
    printf("Waiting 3 seconds...\n");

    nanosleep(&waitTime, NULL);
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
    int retval = 0;
    int index, yIndex, xIndex, sideBet;

    if( (startX+lenX) <= MAX_X && (startY+lenY) <= MAX_Y )
    {
        retval = 1;

        LCD_WR_CMD(XS,0);
        LCD_WR_CMD(XE,MAX_X);
        LCD_WR_CMD(YS,0);
        LCD_WR_CMD(YE,MAX_Y);

        LCD_WR_REG(0x22);
        LCD_CS_CLR;
        LCD_RS_SET;

        for(xIndex = startX; xIndex < (lenX+startX); xIndex++)
        {
            printf("Starting row %d\n",xIndex);
//            LCD_WR_CMD(XP,xIndex);
//            LCD_WR_CMD(YP,startY);

            for(yIndex = startY; yIndex < (lenY+startY); yIndex++)
            {
                //LCD_WR_DATA(part[index]);
                writeDot(xIndex,yIndex,part[index]);
                index++;
            }
        }
/*
        for(ndex = 0; index < (lenX+lenY); index++)
        {
            LCD_WR_DATA(part[index]);
        }
*/
        LCD_CS_SET;
    }

    return retval;
}

void writeFrame(int *frame)
{
    int index;

    LCD_WR_CMD(YS,0);     // Column address start
    LCD_WR_CMD(YE,MAX_Y); // Column address end
    LCD_WR_CMD(XS,0);     // Row address start
    LCD_WR_CMD(XE,MAX_X); // Row address end

    LCD_WR_REG(0x22);
    LCD_CS_CLR;
    LCD_RS_SET;

    for(index = 0; index < (MAX_Y*MAX_X); index++)
    {
        LCD_WR_DATA(frame[index]);
    }

    LCD_CS_SET;
}

