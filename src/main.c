/*
	iClass Dump
	Original code by Brad Antoniewicz
	Modified code by Sharan ***REMOVED***

	When referring to FT2232H, I am referring to FT2232H Mini Module
	Power the reader from 12V. Make sure all grounds (GND) are connected.

	HID iClass ICSP Pinout From left to right

	Pin 1 of iClass - GND (ground, 0V) 											- GND Pin of FT2232H
	Pin 2 of iClass - VDD (5V)															- No Connect
	Pin 3 of iClass - VPP (programming mode) 								- 12V Source
	Pin 4 of iclass - PGD (ICSP data)												- Pin CN2-7 of FT2232H
	Pin 5 of iClass - PGC (ICSP clock)											- Pin CN2-10 of FT2232H
	Pin 6 of iClass - PGM (low voltage programming enable) 	- Pin CN2-9 of FT2232H

	The FT2232H has two channels. The program will only use the first channel.
	The first channel will be Channel A.

	The program is set up in synchronous bit-bang mode. The ADBUS will be used.

*/

#define PIN_PGD (1<<0)
#define PIN_PGC (1<<1)
#define PIN_PGM (1<<2)

#define PIN_OUT (PIN_PGD|PIN_PGC|PIN_PGM)

#define PGM_CORE_INST                    0	// 0b0000
#define PGM_TABLAT_OUT                   2	// 0b0010
#define PGM_TABLE_READ                   8	// 0b1000
#define PGM_TABLE_READ_POST_INC          9	// 0b1001
#define PGM_TABLE_READ_POST_DEC         10	// 0b1010
#define PGM_TABLE_READ_PRE_INC          11	// 0b1011
#define PGM_TABLE_WRITE                 12	// 0b1100
#define PGM_TABLE_WRITE_POST_INC2       13	// 0b1101
#define PGM_TABLE_WRITE_POST_INC2_PGM   14	// 0b1110
#define PGM_TABLE_WRITE_PGM             15	// 0b1111

#define BAUD_RATE	1000000

// Number of registers to read
#define REGS 1536

#include <stdio.h>
#include <ftd2xx.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

FT_HANDLE ftHandle;

void cleanup() {
	printf("Closing FTDI Device.\n");
	FT_Close(ftHandle);
	ftHandle = NULL;
}

void report(FT_STATUS ftStatus, char *fnName, char *notes) {
	if( ftStatus == FT_OK ) {
		return;
	} else {
		printf("FTDI function failed: %s\n", fnName);
		printf("%s\n", notes);
		cleanup();
		exit(-1);
	}
}

void tick_tx(UCHAR tick) {
	UCHAR data;
	DWORD count;
	FT_STATUS ftStatus;

	ftStatus = FT_Write(ftHandle, &tick, sizeof(tick), &count);
	report(ftStatus, "FT_Write", "");

 	ftStatus = FT_Read(ftHandle, &data, sizeof(data), &count);
	report(ftStatus, "FT_Read", "");
}

void ICD_Write(UCHAR cmd, USHORT data)
{
	int i;
	UCHAR tx[(4 + 16) * 2 + 1], *p, out;
	DWORD count;
	FT_STATUS ftStatus;

  p = tx;

  // transmit CMD
  for( i = 0; i < 4; i++ )
  {
    // keep reset high
    out = PIN_PGM | PIN_PGC;

    // get CMD LSB first
    if( cmd & 1 )
			out |= PIN_PGD;
    cmd >>= 1;

    // shift out PGD data + PGC
    *p++ = out;

    // shift out PGD only - no PGC
    *p++ = out ^ PIN_PGC;
  }

  // transmit payload data
  for (i = 0; i < 16; i++)
  {
    // keep reset high + PGC
    out = PIN_PGM | PIN_PGC;
    // get DATA LSB first
    if (data & 1)
			out |= PIN_PGD;
    data >>= 1;

    // shift out PGD data + PGC
    *p++ = out;

    // shift out PGD only - no PGC
    *p++ = out ^ PIN_PGC;
  }

  // all lines to GND except of reset line
  *p++ = PIN_PGM;

  ftStatus = FT_Write(ftHandle, &tx, sizeof (tx), &count);
	report(ftStatus, "FT_Write", "");

	ftStatus = FT_Read(ftHandle, &tx, sizeof (tx), &count);
	report(ftStatus, "FT_Read", "");
}

int TABLAT_Read()
{
	int i;
	UCHAR tx[(4 + 16) * 2 + 1], *p, out, cmd;
	DWORD count;
	FT_STATUS ftStatus;

  p = tx;
  cmd = PGM_TABLAT_OUT;

	// transmit CMD
  for( i = 0; i < (4 + 16); i++ )
  {
    // keep reset high
    out = PIN_PGM | PIN_PGC;

		// get CMD LSB first
    if( cmd & 1 )
			out |= PIN_PGD;
    cmd >>= 1;

		// shift out PGD data + PGC
    *p++ = out;

		// shift out PGD only - no PGC
    *p++ = out ^ PIN_PGC;
  }

  *p++ = PIN_PGM;

  ftStatus = FT_Write(ftHandle, &tx, sizeof (tx), &count);
	report(ftStatus, "FT_Write", "");

  ftStatus = FT_Read(ftHandle, &tx, sizeof (tx), &count);
	report(ftStatus, "FT_Read", "");

  out = 0;
  for (i = 0; i < 8; i++) {
    out = (out >> 1) | ((tx[i * 2 + (1 + 2 * 12)] & PIN_PGD) ? 0x80 : 0);
	}

  return out;
}

int main(int argc, char *argv[]) {
	int i = 0;
	UCHAR ucMask;
	FT_STATUS ftStatus;

	uint8_t eeprom_data[REGS];

	printf("iCTs EEPROM Dumper\n");
	printf("Original code by: brad.antoniewicz@foundstone.com\n");
	printf("Modified code by: ***REMOVED***\n");
	printf("------------------------------------------------\n");

	printf("Connecting to FTDI Device...\n");

	char warnings[] = "Try unloading FTDI drivers: rmmod ftdi_sio usbserial\n";

	ftStatus = FT_Open(0, &ftHandle);
	report(ftStatus, "FT_Open", warnings);

	ftStatus = FT_SetBitMode(ftHandle, PIN_OUT, 0x04);
	report(ftStatus, "FT_SetBitMode", "");

	ftStatus = FT_SetBaudRate(ftHandle, BAUD_RATE);
	report(ftStatus, "FT_SetBaudRate", "");

	tick_tx(0x00);

	ftStatus = FT_GetBitMode(ftHandle, &ucMask);
	report(ftStatus, "FT_GetBitMode", "");

	printf("BitMode is currently: 0x%02x\n", ucMask);

	printf("Make sure at least one card has been read by the reader, then\n");
	printf("connect your FTDI Device to the reader's ICSP port\n");
	printf("and then introduce the VPP power\n");
	printf("\n");

	for( i = 20; i > 0; i-- ) {
		printf("\033[K\r");
		printf("Sleeping for %d Seconds while you do so....",i);
		fflush(stdout);
		sleep(1);
	}

	printf("Starting EEPROM Dump\n");

	// Set FSR0H to 0

	// Copy 0 to the working register (MOV literal)
	printf("MOVLW <ADDR> - Writing PGM_CORE_INST 0x0E00\n");
	ICD_Write(PGM_CORE_INST, 0x0E00);

	// Copy working register to FSR0H (MOV WREG)
  printf("MOVWF FSR0H - Writing PGM_CORE_INST 0x6EEA\n");
  ICD_Write(PGM_CORE_INST, 0x6EEA);

	// Set FSR0L to 0, increments FSR0H automatically

	// Copy 0 to working register
	printf("MOVLW <ADDR> - Writing PGM_CORE_INST 0x0E00\n");
  ICD_Write(PGM_CORE_INST, 0x0E00 );

	// Copy working register to FSR0L
	printf("MOVWF FSR0L - Writing PGM_CORE_INST 0x6EE9\n");
  ICD_Write(PGM_CORE_INST, 0x6EE9);

	printf("Dumping... (takes ~10 seconds)\n");

	// Increment FSR0
	for( i = 1; i < REGS; i++ ) {
    ICD_Write(PGM_CORE_INST, 0x50EE);
    ICD_Write(PGM_CORE_INST, 0x6EF5);

		eeprom_data[i] = TABLAT_Read();
	}

  printf("Full EEPROM Dump:\n");
  printf("-------------------------------------------------------\n");

	for( i = 1; i < REGS; i++ ) {
		printf("%02x ", eeprom_data[i]);
		if( i != 0 && i % 16 == 0 )
			printf("\n");
	}

	printf("\n");
	printf("-------------------------------------------------------\n");
	printf("\n");

	cleanup();
	printf("Program has terminated gracefully.\n");
	return 0;
}
