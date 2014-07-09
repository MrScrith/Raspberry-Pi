void writeDot(int dotX, int dotY, int color);
void lcdInit();
void lcdTest();
void initSpi();
void resetScreen();
int writePart(int startX, int startY, int lenX, int lenY, int *part);
void writeFrame(int *frame);
void backlightOn();
void backlightOff();
