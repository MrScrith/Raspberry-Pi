#define RGB565(r,g,b)((r>>3)<<11 | (g >> 2) << 5 | (b >> 3))
#define MAX_X 239
#define MAX_Y 319

void writeDot(int dotX, int dotY, int color);
void lcdInit();
void lcdTest();
void initSpi();
void resetScreen();
int writePart(int startX, int startY, int lenX, int lenY, int *part);
void writeFrame(int *frame);
void backlightOn();
void backlightOff();
