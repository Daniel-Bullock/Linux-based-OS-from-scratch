/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 * vim:ts=4 noexpandtab
 */

#ifndef _I8259_H
#define _I8259_H

#include "types.h"

/* Ports that each PIC sits on */
#define MASTER_8259_PORT    0x20
#define MASTER_8259_IMR     0x21
#define SLAVE_8259_PORT     0xA0
#define SLAVE_8259_IMR      0xA1
#define MAX_INTERRUPT       15
#define SLAVE_IRQ           2
#define PIC_SIZE            8

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning
 * of each word */
#define ICW1                0x11
#define ICW2_MASTER         0x20
#define ICW2_SLAVE          0x28
#define ICW3_MASTER         0x04
#define ICW3_SLAVE          0x02
#define ICW4                0x01

/* End-of-interrupt byte.  This gets OR'd with
 * the interrupt number and sent out to the PIC
 * to declare the interrupt finished */
#define EOI                 0x60

/* Externally-visible functions */

/* Enables both PICs (note: interrupts should be disabled before calling)
 *      INPUTS: none
 *      OUTPUTS: sets master_mask to 0xFF (fully masked)
 *      RETURN VALUE: none
 *      SIDE EFFECTS: none
 */
void i8259_init(void);

/* Enable (unmask) the specified IRQ 
 *      INPUTS: irq_num - IRQ number to enable, range 0-15
 *      OUTPUTS: sets bit (irq_num) of master_mask or slave_mask low
 *      RETURN VALUE: none
 *      SIDE EFFECTS: changes master_mask
 */
void enable_irq(uint32_t irq_num);

/* Disable (mask) the specified IRQ 
 *      INPUTS: irq_num - IRQ number to disable, range 0-15
 *      OUTPUTS: sets bit (irq_num) of master_mask or slave_mask high
 *      RETURN VALUE: none
 *      SIDE EFFECTS: changes master_mask
 */
void disable_irq(uint32_t irq_num);

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);

#endif /* _I8259_H */
