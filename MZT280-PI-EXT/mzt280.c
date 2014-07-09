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
#define LCD_PWM_CLR bcm2835_gpio_clr(LCDPWM)

#define LCD_CS_SET  bcm2835_gpio_set(SPICS)
#define LCD_RS_SET  bcm2835_gpio_set(SPIRS)
#define LCD_RST_SET bcm2835_gpio_set(SPIRST)
#define LCD_SCL_SET bcm2835_gpio_set(SPISCL)
#define LCD_SCI_SET bcm2835_gpio_set(SPISCI)
#define LCD_PWM_SET bcm2835_gpio_set(LCDPWM)

int color[]={0xf800,0x07e0,0x001f,0xffe0,0x0000,0xffff,0x07ff,0xf81f};

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

int compare(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
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

void inline LCD_WR_Data(int val)
{
    bcm2835_spi_transfer(val>>8);
    bcm2835_spi_transfer(val);
}


void write_dot(int dx,int dy,int color)
{
    LCD_WR_CMD(XS,0x0000); // Column address start2
    LCD_WR_CMD(XE,MAX_X); // Column address end2
    LCD_WR_CMD(YS,0x0000); // Row address start2
    LCD_WR_CMD(YE,MAX_Y); // Row address end2

    LCD_WR_CMD(XP,dy); // Column address start
    LCD_WR_CMD(YP,dx); // Row address start
        
    LCD_WR_CMD(0x22,color);
}

void loadFrameBuffer_320x240()
{
    FILE *infile=fopen("/dev/fb0","rb");
    int  xsize=320, ysize=240;
    int i,j;
    int p;
    int r1,g1,b1;
    int r,g,b;
    int drawmap[ysize][xsize];
    int diffmap[ysize][xsize];
    int diffsx, diffsy, diffex, diffey;
    unsigned long offset=0;
    unsigned char *buffer = (unsigned char *) malloc(xsize * ysize * 2);

    for(i=0; i< ysize; i++)
    {
        for(j=0; j<xsize;j++)
        {
            diffmap[i][j] = 1;
        }
    }
    
    while(1)
    {
        fseek(infile, 0, 0);
    
        if (fread (buffer, xsize * ysize *2, sizeof(unsigned char), infile) != 1)
        {
            printf ("Read < %d chars when loading file %s\n", xsize*ysize*3, "ss");
            printf ("config.txt framebuffer setting error") ;
            return;
        }  

        struct timespec ts1, ts2;
        long elapsed;

        clock_gettime(CLOCK_MONOTONIC, &ts1);

        diffex  = 0;
        diffey  = 0;
        diffsx  = 65535;
        diffsy  = 65535;

        for (i=0; i < ysize; i++)
        {
            for (j=0; j < xsize; j++) 
            {
                offset =  (i * xsize+ j)*2;
                p=(buffer[offset+1] << 8) | buffer[offset];
                r = (p & RGB565_MASK_RED) >> 11;
                g = (p & RGB565_MASK_GREEN) >> 5;
                b = (p & RGB565_MASK_BLUE);
                
                r <<= 1;
                b <<= 1;
                
                offset = ( (i+1) * xsize +j )*2;
                p=(buffer[offset+1] << 8) | buffer[offset];
                r1 = (p & RGB565_MASK_RED) >> 11;
                g1 = (p & RGB565_MASK_GREEN) >> 5;
                b1 = (p & RGB565_MASK_BLUE);
                
                r += r1<<1;
                g += g1;
                b += b1 <<1;
              
                offset = ( i*xsize + j+1)*2;
                p=(buffer[offset+1] << 8) | buffer[offset];
                r1 = (p & RGB565_MASK_RED) >> 11;
                g1 = (p & RGB565_MASK_GREEN) >> 5;
                b1 = (p & RGB565_MASK_BLUE);
                
                r += r1<<1;
                g += g1;
                b += b1 <<1;
                
                offset=((i+1)*xsize + j+1)*2;
                p=(buffer[offset+1] << 8) | buffer[offset];
                r1 = (p & RGB565_MASK_RED) >> 11;
                g1 = (p & RGB565_MASK_GREEN) >> 5;
                b1 = (p & RGB565_MASK_BLUE);
                
                r += r1<<1;
                g += g1;
                b += b1 <<1;
                
                p = RGB565(r, g, b);
            
                drawmap[i][j] = p;
  
                if ((i>>1) < diffsx)
                    diffsx = i>>1;
                if ((i>>1) > diffex)
                    diffex = i >> 1;
                if ((j>>1)< diffsy)
                    diffsy=j>>1;
                if ((j>>1)>diffey)
                    diffey = j >>1;
            }
        }

        LCD_WR_CMD(YS,diffsx); // Column address start2
        LCD_WR_CMD(YE,diffex); // Column address end2
        LCD_WR_CMD(XS,diffsy); // Row address start2
        LCD_WR_CMD(XE,diffey); // Row address end2
        LCD_WR_REG(0x22);
        LCD_CS_CLR;
        LCD_RS_SET;

        for (i=0; i < 240; i++)
        {
            for (j=0; j < 320; j++) 
            {
                if ( diffmap[i][j] != drawmap[i][j])
                {
                    write_dot(i,j,drawmap[i][j]);
                }
                diffmap[i][j] = drawmap[i][j];
            }
        }
        
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        elapsed = ts2.tv_nsec - ts1.tv_nsec;
        elapsed += (ts2.tv_sec - ts1.tv_sec) * 1000000000;

        if (elapsed < 333333333) 
        {
            struct timespec et;
            et.tv_sec = 0;
            et.tv_nsec = 333333333 - elapsed;
            nanosleep(&et, NULL);
        }
    }
}

void LCD_Init()
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


void LCD_test()
{
	int temp,num,i;
	char n;
    
	LCD_WR_CMD(XS,0x0000); // Column address start2
	LCD_WR_CMD(XS+1,0x0000); // Column address start1
	LCD_WR_CMD(XE,0x0000); // Column address end2
	LCD_WR_CMD(XE+1,0x00EF); // Column address end1
	LCD_WR_CMD(YS,0x0000); // Row address start2
	LCD_WR_CMD(YS+1,0x0000); // Row address start1
	LCD_WR_CMD(YE,0x0001); // Row address end2
	LCD_WR_CMD(YE+1,0x003F); // Row address end1
    
	LCD_WR_REG(0x22);
	LCD_CS_CLR;
	LCD_RS_SET;
	printf("Running first test loop\n");
	for(n=0;n<8;n++)
	{
	    temp=color[n];
	    printf("filling with color %X\n",temp);
		for(num=40*240;num>0;num--)
		{
			LCD_WR_Data(temp);
		}
	}
	printf("Running second test loop\n");
	for(n=0;n<8;n++)
	{
		LCD_WR_CMD(XS,0x0000); // Column address start2
		LCD_WR_CMD(XS+1,0x0000); // Column address start1
		LCD_WR_CMD(XE,0x0000); // Column address end2
		LCD_WR_CMD(XE+1,0x00EF); // Column address end1
		LCD_WR_CMD(YS,0x0000); // Row address start2
		LCD_WR_CMD(YS+1,0x0000); // Row address start1
		LCD_WR_CMD(YE,0x0001); // Row address end2
		LCD_WR_CMD(YE+1,0x003F); // Row address end1
		
		LCD_WR_REG(0x22);
		LCD_CS_CLR;
		LCD_RS_SET;
		temp=color[n];
		printf("filling with color %X\n",temp);
		for(i=0;i<240;i++)
		{
			for(num=0;num<320;num++)
			{
				LCD_WR_Data(temp);
			}
		}
		printf("Waiting 3 seconds...\n");
		struct timespec et;
		et.tv_sec = 3;
		et.tv_nsec = 0;
		nanosleep(&et, NULL);
	}
	LCD_CS_SET;
}


int main (void)
{
    if (!bcm2835_init())
    {
        printf("bcm2835 init error\n");
        return 1;
    }

    bcm2835_gpio_fsel(SPICS,  BCM2835_GPIO_FSEL_OUTP) ;
    bcm2835_gpio_fsel(SPIRS,  BCM2835_GPIO_FSEL_OUTP) ;
    bcm2835_gpio_fsel(SPIRST, BCM2835_GPIO_FSEL_OUTP) ;
    bcm2835_gpio_fsel(LCDPWM, BCM2835_GPIO_FSEL_OUTP) ;
    
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_2);
    
    LCD_PWM_CLR;

    printf("Initializing TFT Screen\n");
    
    LCD_Init();
    printf("Initialization complete\n");
    printf("Resetting screen\n");
    LCD_WR_CMD(XS, 0x0000); // Column address start2
    LCD_WR_CMD(XE, MAX_X ); // Column address end2
    LCD_WR_CMD(YS, 0x0000); // Row address start2
    LCD_WR_CMD(YE, MAX_Y ); // Row address end2
        
    LCD_WR_REG(0x22);
    LCD_CS_CLR;
    LCD_RS_SET;
	printf("running LCD Test\n");
	LCD_test();
    //loadFrameBuffer_320x240();
    //loadFrameBuffer_640x480();

    return 0 ;
}
