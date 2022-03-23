/* keyboard.c - Functions to interact with the keyboard */

#include "keyboard.h"
#include "i8259.h"
#include "lib.h"
#include "terminal_driver.h"

/*
// 0x3A size because all scan codes we want to print for checkpoint 1 are < 0x3A
static char scan_code_1[0x3A] = { 0 ,  0 , '1', '2',        // covers letters, numbers, and a few symbols
                                 '3', '4', '5', '6',
                                 '7', '8', '9', '0',
                                 '-', '=', '0', '0',
                                 'q', 'w', 'e', 'r',
                                 't', 'y', 'u', 'i',
                                 'o', 'p', '[', ']',
                                 '\n', 0 , 'a', 's',
                                 'd', 'f', 'g', 'h',
                                 'j', 'k', 'l', ';',
                                 '\'','`',  0 , '\\',
                                 'z', 'x', 'c', 'v',
                                 'b', 'n', 'm', ',',
                                 '.', '/',  0 ,  0 ,
                                  0 , ' '
                                 };
*/

static uint8_t scan_code2[MODS][KEYS] = {
    //normal
    {0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l' , ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v','b', 'n', 'm',',', '.', '/', 0, '*', 0, ' ', 0}, 

    // shift
    {0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0, 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L' , ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0},

    // caps (index is 2 which is why CAPS_PRESSED is 2)
    {0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0, 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L' , ';', '\'', '`', 0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',',', '.', '/', 0, '*', 0, ' ', 0},

    /// shift and caps
    {0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,'q','w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', 0, 0, 'a', 's',
	 'd', 'f', 'g', 'h', 'j', 'k', 'l' , ':', '"', '~', 0, '|', 'z', 'x', 'c', 'v','b', 'n', 'm', '<', '>', '?', 0, '*', 0, ' ', 0}
};

//Flags for modifers
uint8_t shift_flag  = 0;
uint8_t control_flag   = 0;    //Can be up to two (for the right and left)
uint8_t caps_flag   = 0;
uint8_t alt_flag   = 0;

int halt_flag = 0;

/* void keyboard_init()
 * Initializes keyboard to use scan code 1 and enables the
 * associated IRQ on the PIC 
 *      INPUTS: none
 *      OUTPUTS: none
 *      RETURN VALUE: none
 *      SIDE EFFECTS: enables interrupts from the keyboard
 */
void keyboard_init(){
    //outb(0xF0, KB_CMD_PORT);  // set keyboard scan code
    //outb(0x01, KB_DATA_PORT);  // scan code set 1
    //outb(0xF4, KB_CMD_PORT);  // enable keyboard scanning
    enable_irq(KB_IRQ);
}






/*  For checkpoint 1, only handles single key presses and alphanumeric keys
    with some symbols.   */
/* Handles interrupts generated by the keyboard
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Prints the character typed onto the screen
 */
void keyboard_handler(){
    /* Keyboard handler from checkpoint 1
    uint8_t scan_code = inb(KB_DATA_PORT);
    if(scan_code < 0x3A && scan_code_1[scan_code] != 0){
        printf("%c", scan_code_1[scan_code]);
    }
    send_eoi(KB_IRQ);
    */

    cli();

    uint8_t prev_terminal = active_terminal;
    active_terminal = visible_terminal;

    uint8_t scan_code = inb(KB_DATA_PORT);
    int isSpecialChar = special_chars(scan_code);

    int32_t status = 0;
    
    if (!isSpecialChar && (scan_code < KEYS) && !control_flag  && (scan_code2[shift_flag + caps_flag][scan_code] != 0)) {
        //printf('%d', shift_flag);
        status = add_char_to_tbuff(scan_code2[shift_flag + caps_flag][scan_code]);
        if (status) putc(scan_code2[shift_flag + caps_flag][scan_code]);
    }

    active_terminal = prev_terminal;

    sti();
    send_eoi(KB_IRQ);
}


uint8_t special_chars(uint8_t scan_code){
    int32_t status;

    switch(scan_code) {
        case ALT_PRESS:
            alt_flag = 1;
            return 1;
        case ALT_RELEASE:
            alt_flag = 0;
            return 1;
        case L_SHIFT_PRESS: 
            shift_flag = 1; 
            return 1;
        case L_SHIFT_RELEASE:
            shift_flag = 0;
            return 1; 
        case R_SHIFT_PRESS:
            shift_flag = 1; 
            return 1;
        case R_SHIFT_RELEASE:
            shift_flag = 0; 
            return 1;
        case CAPS_LOCK: 
            if (caps_flag) {
                caps_flag = 0;
                return 1;
            }
            else {
                caps_flag = CAPS_PRESSED;
                return 1;
            }   
        case CTRL_PRESS:
            control_flag = 1;
            return 1;
        case CTRL_RELEASE:
            control_flag = 0; 
            return 1;
        case SCANCODE_C: // C
            if(control_flag) {
                terminal_data[visible_terminal].halt_flag = 1;
                return 1;
            }
            else {
                return 0;
            }
        case SCANCODE_L:
            if(control_flag) {
                clear_reset_cursor();
                return 1;
            }
            else {
                return 0;
            }
        case ENTER:
            status = add_char_to_tbuff('\n');
            if (status) putc('\n');
            return 1;
            
        case BACKSPACE:
            status = add_char_to_tbuff(BACKSPACE);
            if (status) putc(BACKSPACE);
            return 1;
        case TAB:
            status = add_char_to_tbuff(' ');
            if (status) putc(' ');
        case F1_PRESS:
            if(alt_flag){
                switch_visible_terminal(0);
            }
            return 1;
        case F2_PRESS:
            if(alt_flag){
                switch_visible_terminal(1);
            }
            return 1;
        case F3_PRESS:
            if(alt_flag){
                switch_visible_terminal(2);
            }
            return 1;        
        default:
            return 0;
    }
}
