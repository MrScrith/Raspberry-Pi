#include <stdio.h>
#include "bcm2835.h"
#include "mzt280.h"

int main(void)
{
    if(!bcm2835_init())
    {
        printf("bcm2835 init error!\n");
        return 1;
    }

    // Setup the SPI bus
    initSpi();

    backlightOn();

    printf("Initializing the LCD Screen");

    lcdInit();

    printf("Running LCD Test");

    lcdTest();

    backlightOff();
 
    return 0;
}
