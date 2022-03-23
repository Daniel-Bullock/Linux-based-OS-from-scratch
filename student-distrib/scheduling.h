#ifndef SCHEDULING_H
#define SCHEDULING_H


#define PIT_IRQ_VECTOR 0
#define PIT_MODE		0x36   //mode 3
#define PIT_CMD        0x43    //command register port
#define DIV_HZ	11932    //1193180 / 100 HZ
#define PIT_DATAREG     0x40


void pit_init();
void schedule_process();

#endif
