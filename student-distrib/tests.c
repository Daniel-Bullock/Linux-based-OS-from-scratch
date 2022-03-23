#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "fs.h"
#include "syscalls.h"
#include "terminal_driver.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* Paging Test
 * 
 * Asserts that physical addresses can be mapped to multiple virtual addresses
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Writes to 0x1000
 * Coverage: Enable paging, initialize paging
 * Files: paging.c/h
 */
int page_test(){
	TEST_HEADER;
	int result = FAIL;
	int canary = 1234;

	// Create two pointers mapped to same physical memory
	volatile int *mem1 = (volatile int*) 0x1000;
	volatile int *mem2 = (volatile int*) 0x2000;

	*mem2 = 0;
	*mem1 = canary;
	if (*mem2 == canary) result = PASS;

	return result;
}


// add more tests here

/* DE_test - tests Divide Error exception
 * Expectation: prints Divide Error
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prevents further interrupts
 *  * Coverage: Load IDT, IDT definition
 * Files: init_idc.c/h, exc_handler.c/h
 */
int DE_test(){
	TEST_HEADER;
	int result = PASS;
	int dividebyzero = 0;
	int dbz = 20/dividebyzero;
	dbz++; 
	result = FAIL;
	return result;
}

/* system_call_test - tests system calls
 * Expectation: prints system call
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: prevents further interrupts
 *  * Coverage: Load IDT, IDT definition
 * Files: init_idc.c/h, exc_handler.c/h
 */
int system_call_test() {
    TEST_HEADER;
	asm("int    $0x80");
    return FAIL; 
}


/* NMI Test - tests for NMI interrupt
 * Expectation: prints NMI interrupt
 * Inputs: None
 * Outputs: FAIL
 * Side Effects: prevents further interrupts
 * Coverage: Load IDT, IDT definition
 * Files: init_idc.c/h, exc_handler.c/h
 */
int nmi_test() {
	TEST_HEADER;
	asm("int $2");
	return FAIL;
}

/* Pagefault Test - Tests for interrupt on dereferencing unmapped memory
 * Expectation: prints page fault interrupt
 * Inputs: None
 * Outputs: FAIL
 * Side Effects: prevents further interrupts
 * Coverage: Load IDT, IDT definition, enable paging
 * Files: paging.c/h, init_idc.c/h, exc_handler.c/h
 */

int pagefault_test(){
	TEST_HEADER;
	int result = FAIL;
	int* ptr = NULL;
	// dereference a null pointer
	*(ptr) += 1;
	return result;
}
// add more tests here

/* Checkpoint 2 tests */

/* RTC Test - Tests setting RTC at different frequencies
 * Expectation: RTC runs at various frequencies for 2 seconds each
 * Inputs: None
 * Outputs: PASS
 * Side Effects: takes some time to run
 * Coverage: rtc_write, rtc_read, rtc_init
 * Files: rtc.c/h
 */
int rtc_test(){
	TEST_HEADER;
	int i;
	int buf;
	rtc_open(NULL, 0);
	for(buf = 1; buf <= RTC_RATE; buf*=2){
		printf("\n%d Hz\n", buf);
		rtc_write(0, &buf, 4);
		for(i = 0; i < 2*buf; i++){
			// receive 2*freq interrupts, shuld take 2 sec
			rtc_read(0, 0, 0);
		}
	}
	printf("\n");
	buf = 2;
	rtc_write(0, &buf, 4);
	return PASS;
}

/* File information test - Lists files using read_dentry_by_index()
 * Expectation: Prints the files in the root directory,
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: read_dentry_by_index, fs_init
 * Files: fs.c/h
 */

int filestat_test(){
	TEST_HEADER;
	int result = PASS;
	
	int i, c;

	dentry_t d;

	for (i=0; i<64; i++) {
		int32_t err = read_dentry_by_index(i, &d);

		if (err) break;

		printf("#%d type: %d inode: 0x%x name: ", i, d.type, d.inode);

		for (c=0; c<32; c++) {
			if (d.name[c] == '\0') break;
			putc(d.name[c]);
		}

		printf("\n");
	}

	

	return result;
}

/* Directory read test - Reads from a directory using open() and read()
 * Expectation: Prints the filenames in a directory
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Opens and closes a file descriptor
 * Coverage: open, dir_open, read, dir_read, read_dentry_by_name()
 * Files: fs.c/h, syscalls.c/h
 */
int dirread_test(){
	TEST_HEADER;
	int result = PASS;
	
	int c;

	int32_t fd = open((uint8_t*)".");

	if (fd < 0) return FAIL;

	uint8_t buf[33];

	while (1) {
		// Clear buffer
		for (c=0; c<33; c++) buf[c] = 0;

		int count = read(fd, buf, 33);

		if (count < 0) return FAIL;
		if (count == 0) break;
		
		printf("%s;\n", buf);
	}
	
	close(fd);

	return result;
}

/* File read test - Reads from a file using open() and read()
 * Expectation: Prints the contents of the file
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Opens and closes a file descriptor
 * Coverage: open, file_open, read, file_read, read_data
 * Files: fs.c/h, syscalls.c/h
 */

int fileread_test(){
	TEST_HEADER;
	int result = PASS;
	
	int c;

	int32_t fd = open((uint8_t*)"created.txt");

	if (fd < 0) return FAIL;

	uint8_t buf[256];

	while (1) {
		// Clear buffer
		for (c=0; c<256; c++) buf[c] = 0;

		int count = read(fd, buf, 256);
		if (count < 0) return FAIL;
		if (count == 0) break;
		
		for (c=0; c<256; c++) {
			if (buf[c] == '\0') break;
			putc(buf[c]);
		}
		printf("[%d]", count);
	}
	
	printf("\n");

	close(fd);

	return result;
}

/* terminal_driver_test Test - Tests for interrupt on dereferencing unmapped memory
 * Expectation: prints page fault interrupt
 * Inputs: None
 * Outputs: None
 * Side Effects: prevents further interrupts
 * Coverage: checks the terminal driver functions
 * Files: terminal.c/h, keyboard.c/h
 */
int terminal_driver_test(){
    TEST_HEADER;
    int nbytes;
    char buf[1024];
    while(1){
        nbytes = terminal_read(0,buf, 8);
        terminal_write(0,buf, nbytes);
    }
    return PASS;
}

/* Checkpoint 3 tests */

/* test_system_call_open Test - Tests for open in system call
 * Expectation: 
 * Inputs: None
 * Outputs: None
 * Side Effects: 
 * Coverage: 
 * Files:syscalls.c/h
 */
int test_system_call_open(){
	int i;
	uint8_t fn[] = "testprint";
	i = open(fn);
	if (i != -1){
		return PASS;
	}
	return FAIL;
}
/* test_system_call_close - close in system call
 * Expectation: 
 * Inputs: None
 * Outputs: None
 * Side Effects: 
 * Coverage: 
 * Files:syscalls.c/h
 */
int test_system_call_close(){
	int i;
	i = close(0);
	if (i == 0){
		return PASS;
	}
	return FAIL;
}


/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	//TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
	// TEST_OUTPUT("filestat_test", filestat_test());
	//TEST_OUTPUT("dirread_test", dirread_test());
	//TEST_OUTPUT("fileread_test", fileread_test());
	//TEST_OUTPUT("page_test", page_test());
	//TEST_OUTPUT("DE_test", DE_test());
	//TEST_OUTPUT("system_call_test", system_call_test());
	//TEST_OUTPUT("nmi_test", nmi_test());
	//TEST_OUTPUT("pagefault_test", pagefault_test());
	//TEST_OUTPUT("rtc_test", rtc_test());
	//TEST_OUTPUT("terminal_driver_test", terminal_driver_test());


	clear_reset_cursor(); //clear screen
	/*while(1){
		execute((const uint8_t*)"ece391shell");
	}*/

	// not working cuz of new pcb
	//TEST_OUTPUT("system call open test",test_system_call_open());
	//TEST_OUTPUT("system call close test",test_system_call_close());
}

