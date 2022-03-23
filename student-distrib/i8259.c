/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void i8259_init(void) {
    // give the two PICs initialize command
    outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

    // ICW2 - give PICs their vector offsets
    outb(ICW2_MASTER, MASTER_8259_IMR);
    outb(ICW2_SLAVE, SLAVE_8259_IMR);

    // ICW3 - master/slave wiring
    outb(ICW3_MASTER, MASTER_8259_IMR);
    outb(ICW3_SLAVE, SLAVE_8259_IMR);

    // ICW4 - info about what mode the 8259 runs in
    outb(ICW4, MASTER_8259_IMR);
    outb(ICW4, SLAVE_8259_IMR);

    // mask all interrupts on PIC
    master_mask = 0xFF;
    outb(master_mask, MASTER_8259_IMR);    /* 1 = inhibited, 0 = enabled,
                                            * so inhibiting all -> 0xFF    */

    enable_irq(SLAVE_IRQ);
}

/* Enable (unmask) the specified IRQ 
 *      INPUTS: irq_num - IRQ number to enable, range 0-15
 *      OUTPUTS: sets bit (irq_num) of master_mask or slave_mask low
 *      RETURN VALUE: none
 *      SIDE EFFECTS: changes master_mask, enables an interrupt
 */
void enable_irq(uint32_t irq_num) {
    uint8_t mask;
    if(irq_num > MAX_INTERRUPT){
        return;
    }

    if(irq_num < PIC_SIZE){
        mask = ~(1 << irq_num);
        master_mask &= mask;
        outb(master_mask, MASTER_8259_IMR);
    }else{
        irq_num -= PIC_SIZE;
        mask = ~(1 << irq_num);
        slave_mask &= mask;
        outb(slave_mask, SLAVE_8259_IMR);
    }

}

/* Disable (mask) the specified IRQ 
 *      INPUTS: irq_num - IRQ number to disable, range 0-15
 *      OUTPUTS: sets bit (irq_num) of master_mask or slave_mask high
 *      RETURN VALUE: none
 *      SIDE EFFECTS: changes master_mask
 */
void disable_irq(uint32_t irq_num) {
    uint8_t mask;
    if(irq_num > MAX_INTERRUPT){
        return;
    }

    if(irq_num < PIC_SIZE){
        mask = (1 << irq_num);
        master_mask |= mask;
        outb(master_mask, MASTER_8259_IMR);
    }else{
        irq_num -= PIC_SIZE;
        mask = (1 << irq_num);
        slave_mask |= mask;
        outb(slave_mask, SLAVE_8259_IMR);
    }
    
}

/* Send end-of-interrupt signal for the specified IRQ 
 *      INPUTS: irq_num - IRQ number to send EOI to, range 0-15
 *      OUTPUTS: none
 *      RETURN VALUE: none
 *      SIDE EFFECTS: sends EOI, tells device we are done servicing interrupt
 */
void send_eoi(uint32_t irq_num) {
    if(irq_num > MAX_INTERRUPT){
        return;
    }

    if(irq_num >= PIC_SIZE){   // send to both master and slave
        irq_num -= PIC_SIZE;
        outb((EOI | irq_num), SLAVE_8259_PORT);
        outb((EOI | SLAVE_IRQ), MASTER_8259_PORT);
    }else{
        outb((EOI | irq_num), MASTER_8259_PORT);
    }
}


