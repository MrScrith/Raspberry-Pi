#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include "bcm2835.h"
#include "mzt280.h"
#include "charmap.h"

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

    int cell[6*13];

    int line1,line2,lindex,lintmp,linchar = 0;

    for(line1 = 0; line1 < 13; line1++)
    {
        lintmp = characters[1][line1];
        printf("line value 0x%0.2X | ",lintmp);
        for(line2 = 0; line2 < 6; line2++)
        {
            if ( lintmp & 0x1 == 1 )
            {
                cell[lindex] = 0xFFFF;
                printf("##");
            }
            else
            {
                cell[lindex] = 0x0000;
                printf("..");
            }
            lintmp >>= 1;
            //printf("-0x%X",cell[lindex]);
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
