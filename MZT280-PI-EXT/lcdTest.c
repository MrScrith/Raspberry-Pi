#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include "bcm2835.h"
#include "mzt280.h"
#include "charmap.h"

#define RED_V1 0
#define RED_V2 0x1F

#define GREEN_V1 0
#define GREEN_V2 0x3F

#define BLUE_V1 0
#define BLUE_V2 0x1F

#define VALUE_1_X 0
#define VALUE_1_Y 0

#define VALUE_2_X MAX_X
#define VALUE_2_Y MAX_Y

int rgbAtPoint(px,py)
{
    int redP, greenP, blueP;

    redP = (RED_V2 * (px - VALUE_1_X) + RED_V1 * (VALUE_2_X-px)) / (VALUE_2_X-VALUE_1_X);
    greenP = (GREEN_V2 * (px - VALUE_1_X) + GREEN_V1 * (VALUE_2_X-px)) / (VALUE_2_X-VALUE_1_X);
    blueP = (BLUE_V1 * (px - VALUE_1_X) + BLUE_V1 * (VALUE_2_X-px)) / (VALUE_2_X-VALUE_1_X);

    return RGB565(redP,greenP,blueP);
}

int main(void)
{
    int lx,ly;
    struct timespec waitTime;

    waitTime.tv_sec = 10;
    waitTime.tv_nsec = 0;

    if(!bcm2835_init())
    {
        printf("bcm2835 init error!\n");
        return 1;
    }

    // Setup the SPI bus
    initSpi();

    backlightOn();

    printf("Initializing the LCD Screen\n");

    lcdInit();

    printf("Running LCD Test");

    lcdTest();

    nanosleep(&waitTime,NULL);

    int cell[78];

    int line1,line2,lindex,lintmp,linchar = 0;

    for(line1 = 0; line1 < 13; line1++)
    {
        lintmp = characters[1][line1];
        printf("line value 0x%X | ",lintmp);
        for(line2 = 0; line2 < 6; line2++)
        {
            if ( lintmp & 0x1 == 1 )
            {
                cell[lindex] = 0xFFFF;
            }
            else
            {
                cell[lindex] = 0x0000;
            }
            lintmp >> 1;
            printf("-0x%X",cell[lindex]);
            lindex++;
        }
        printf("\n");
    }

    printf("about to write spot at 100x100\n");
    //writePart(100,100,13,6,&cell[0]);
    printf("spot written...\n");
    int spotx,spoty,index = 0;

    nanosleep(&waitTime,NULL);

    backlightOff();
 
    return 0;
}
