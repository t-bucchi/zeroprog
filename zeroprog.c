/*
 Copyright (c) 2010 Haxx Enterprises <bushing@gmail.com>
 All rights reserved.

 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following 
 conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
  THE POSSIBILITY OF SUCH DAMAGE.
 
*/

#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "gl.h"

typedef struct {
	unsigned short pid;
	char model_name[64];
	} model_t;

model_t zeroplus_models[] = {
	{0x700B, "LAP-C(32128)"},
	{0x700C, "LAP-C(321000)"},
	{0x700D, "LAP-C(322000)"},
	{0x700A, "LAP-C(16128)"},
	{0x7009, "LAP-C(16064)"},
	{0x700E, "LAP-C(16032)"},
	{0x7016, "LAP-C(162000)"},
};

#define VERSION "1.0"
#define ZP_VID 0x0C12
#define GL_VID 0x05E3 // if you brick it!

#define EEPROM_SIZE 64 // in words
unsigned short eep[EEPROM_SIZE];

#define EEP_CS	1
#define EEP_CLK	2
#define EEP_DI	4
#define EEP_DO	8

#define OFFSET_VID			0
#define OFFSET_PID			1
#define OFFSET_CRC			2
#define OFFSET_MANUFACTURER	8
#define OFFSET_MODEL		0x20
#define OFFSET_SERIAL		0x38

#define LEN_MANUFACTURER	(OFFSET_MODEL - OFFSET_MANUFACTURER)
#define LEN_MODEL			(OFFSET_SERIAL - OFFSET_MODEL)
#define LEN_SERIAL			(EEPROM_SIZE - OFFSET_MODEL)

static void parse_eep(void) {
	printf("EEP: config info ");
	int i;
	for (i=0; i<7; i++) printf("%04hx ", eep[i]);

// dunno why these need to be byteswapped but the above don't:
	printf("\nEEP: Manufacturer: ");
	for (i=8; i<0x20; i++) printf("%02hhx%02hhx ", eep[i] & 0xFF, eep[i] >> 8);
	
	printf("\nEEP: Model: ");
	for (i=0x20; i<0x38; i++) printf("%02hhx%02hhx ", eep[i] & 0xFF, eep[i] >> 8);

	printf("\nEEP: Serial: ");
	for (i=0x38; i<0x40; i++) printf("%02hhx%02hhx ", eep[i] & 0xFF, eep[i] >> 8);
	printf("\n");
}

void eep_send_bit(int bit) {
	int flag = bit ? EEP_DI : 0;
	gl_gpio_write(EEP_CS | flag);
	gl_gpio_write(EEP_CS | EEP_CLK | flag);	
}

static unsigned int gl_eeprom_read_word(void) {
	int i;
	unsigned int retval=0;
	for (i=15; i>=0; i--) {
		eep_send_bit(0);
		if (gl_gpio_read() & EEP_DO) retval |= 1 << i;
	}
	return retval;
}

static void eep_send_ewen(void) {
	printf("sending EWEN\n");
	// start condition
	eep_send_bit(1);
	
	// EWEN command - 0, 0
	eep_send_bit(0);
	eep_send_bit(0);

	// address -- 1 1 1 1 1 1 
	int i;
	for (i=0; i<6; i++) {
		eep_send_bit(1);
	}	
	gl_gpio_write(0);	
}

static void eep_send_ewds(void) {
	printf("sending EWDS\n");
	// start condition
	eep_send_bit(1);
	
	// EWDS command - 0, 0
	eep_send_bit(0);
	eep_send_bit(0);

	// address -- 0 0 0 0 0 0
	int i;
	for (i=0; i<6; i++) {
		eep_send_bit(0);
	}	
	gl_gpio_write(0);	
}

static void eep_write_word(unsigned int addr, unsigned short val) {
	int i;
	printf("sending WRITE(%x, %x)\n", addr, val);

	// start condition
	eep_send_bit(1);
	
	// WRITE command - 0, 1
	eep_send_bit(0);
	eep_send_bit(1);

	for (i=5; i>=0; i--) {
		eep_send_bit(addr & (1 << i));
	}
	
	for (i=15; i>=0; i--) {
		eep_send_bit(val & (1 << i));
	}
	gl_gpio_write(0);
	gl_gpio_write(EEP_CS);
	printf("waiting...");
	fflush(stdout);
	int retries;
	int status=0;
	for(retries=0; (retries < 100) && (status==0); retries++) {
		status = gl_gpio_read() & EEP_DO;
		printf(".");
		fflush(stdout);
	}
	if (status) printf("OK!\n");
	else printf("FAIL :(\n");
}

static unsigned short eep_read_word(unsigned int addr) {
	int i;
//	printf("sending READ(%x)\n", addr);

	// start condition
	eep_send_bit(1);
	
	// READ command - 1, 0
	eep_send_bit(1);
	eep_send_bit(0);

	for (i=5; i>=0; i--) {
		eep_send_bit(addr & (1 << i));
	}
	
	unsigned short retval = gl_eeprom_read_word();
	gl_gpio_write(0);
	return retval;
}

int gl_read_eeprom(unsigned short *eep) {
	// start condition
	eep_send_bit(1);
	
	// read command - 1, 0
	eep_send_bit(1);
	eep_send_bit(0);

	// address -- 0 0 0 0 0 0 
	int i;
	for (i=0; i<6; i++) {
		eep_send_bit(0);
	}	
	
	for(i=0; i<64; i++) {
		eep[i]=gl_eeprom_read_word();
	}
	
	gl_gpio_write(0);
	return 0;
}

static void write_vid_pid(unsigned short vid, unsigned short pid) {
	unsigned short checksum = 0xFF00 | ((vid >> 8) + (vid & 0xFF) + (pid >> 8) + (pid & 0xFF) + 1);
	eep_send_ewen();
	eep_write_word(OFFSET_VID, vid);
	eep_write_word(OFFSET_PID, pid);
	eep_write_word(OFFSET_CRC, checksum);
	eep_send_ewds();
}

static void change_vid(void) {
	printf("%s()\n", __FUNCTION__);
	unsigned short pid = eep_read_word(OFFSET_PID);
	write_vid_pid(ZP_VID, pid);
}

static void change_pid(unsigned int pid) {
	printf("%s(%x)\n", __FUNCTION__, pid);
	write_vid_pid(ZP_VID, pid);
}

static void display_pascal_string(int offset, int len) {
	unsigned char buf[128];
	for(int i=0; i < len; i++) {
		unsigned short val = eep_read_word(offset + i);
		buf[i*2] = val & 0xFF;
		buf[i*2+1] = val >> 8;
	}
	if (buf[0] > (len*2 + 1)) {
		printf("Error: string len %d\n", buf[0]);
		return;
	}
	buf[buf[0]+1]='\0';
	printf("len=%d str='%s'\n", buf[0], buf+1);
}

static void write_pascal_string(int offset, unsigned int len, char *string) {
	char bytes[128];
	if (strlen(string) > (len*2-1)) {
		printf("Invalid string len (%d > %d)\n", (int)strlen(string), len*2-1);
		return;
	}
	
	bytes[0] = strlen(string);
	strncpy(bytes + 1, string, bytes[0]);
	eep_send_ewen();
	for(int i=0; i < (bytes[0]/2+1); i++) {
		eep_write_word(offset + i, bytes[i*2+1] << 8 | bytes[i*2]);
	}
	eep_send_ewds();
}

static void display_strings(void) {
	printf("Manufacturer: ");
	display_pascal_string(OFFSET_MANUFACTURER, LEN_MANUFACTURER);
	printf("Model: ");
	display_pascal_string(OFFSET_MODEL, LEN_MODEL);
	printf("Serial: ");
	display_pascal_string(OFFSET_SERIAL, LEN_SERIAL);
}

static void change_manufacturer(char *manufacturer_string) {
//	printf("%s(%s)\n", __FUNCTION__, manufacturer_string);
	write_pascal_string(OFFSET_MANUFACTURER, LEN_MANUFACTURER, manufacturer_string);
}

static void change_model(char *model_string) {
//	printf("%s(%s)\n", __FUNCTION__, model_string);
	write_pascal_string(OFFSET_MODEL, LEN_MODEL, model_string);
}

static void change_serial(char *serial_string) {
//	printf("%s(%s)\n", __FUNCTION__, serial_string);
	write_pascal_string(OFFSET_SERIAL, LEN_SERIAL, serial_string);
}

static void backup_eeprom(char *filename) {
	printf("%s(%s)\n", __FUNCTION__, filename);
	gl_read_eeprom(eep);
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		perror("Unable to open output file: ");
		exit(1);
	}
	for (unsigned int i=0; i < sizeof(eep)/sizeof(eep[0]); i++) {
		fputc(eep[i]>>8, fp);
		fputc(eep[i] & 0xFF, fp);
	}
	fclose(fp);
	exit(0);
}

static void restore_eeprom(char *filename) {
	printf("%s(%s)\n", __FUNCTION__, filename);
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("Unable to open input file: ");
		exit(1);
	}
	fclose(fp);
	printf("// not yet implemented, sorry\n");
}


static const struct option opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "vendor", no_argument, 0, 'v' },
	{ "product", required_argument, 0, 'v' },
	{ "manufacturer", required_argument, 0, 'm' },
	{ "model", required_argument, 0, 'o' },
	{ "serial", required_argument, 0, 's' },
	{ "backup", required_argument, 0, 'b' },
	{ "restore", required_argument, 0, 'r' },
	{ 0, 0, 0, 0 }
};

static void show_help(char *progname) {
	printf("Usage: %s <command> <argument>\n", progname);
	printf("where <command> is one of the following:\n");
	printf("  -v, --vendor\tchange the vendor ID to the default 0x%04x\n", ZP_VID);
	printf("  -p, --product <PID>\tchange the product ID\n");
	for (unsigned int i=0; i < sizeof(zeroplus_models)/sizeof(zeroplus_models[0]); i++)
		printf("\t0x%04x %s\n", zeroplus_models[i].pid, zeroplus_models[i].model_name);
	printf("  -m, --manufacturer <string>\tchange the manufacturer string\n");
	printf("  -o, --model <string>\tchange the model name string\n");
	printf("  -s, --serial <string>\tchange the serial string\n");
	printf("  -b, --backup <filename>\tdump the entire EEPROM to file\n");
	printf("  -r, --restore <filename>\trestore the entire EEPROM from a file\n");
	printf("If no command is given, the current contents of the EEPROM will be displayed.\n");
}

int main(int argc, char *argv[])
{
	int r = 0;
	
	printf("%s " VERSION "\n", argv[0]);
	
	int c, i;
	c = 0;
	r = gl_open(GL_VID);

	if (r < 0) {
		r = gl_open(ZP_VID);
	}

	if (r < 0) {
		fprintf(stderr, "Zeroplus device not found!\n");
		return -1;
	}

	gl_gpio_oe(EEP_CS | EEP_CLK | EEP_DI);
	gl_gpio_write(0);

	while (argc > 1 && c >= 0) {
		c = getopt_long(argc, argv, "vp:m:o:s:b:r:h", opts, &i);
		switch (c) {
			case 'v':
				change_vid();
				return 0;
			case 'p':
				change_pid(strtoul(optarg, NULL, 16));
				return 0;
			case 'm':
				change_manufacturer(optarg);
				return 0;
			case 'o':
				change_model(optarg);
				return 0;
			case 's':
				change_serial(optarg);
				return 0;
			case 'b':
				backup_eeprom(optarg);
				return 0;
			case 'r':
				restore_eeprom(optarg);
				return 0;
			case '?':
			case 'h':
			default:
				show_help(argv[0]);
				return 0;
		}
		argv++; argc--;
	}

	gl_read_eeprom(eep);
	parse_eep();
	display_strings();
}
