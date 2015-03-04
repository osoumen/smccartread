/*
 GBCartRead - Gameboy Cart Reader - Arduino Serial Reader
 Version: 1.4
 Author: Alex from insideGadgets (www.insidegadgets.com)
 Created: 21/07/2013
 Last Modified: 30/08/2013

 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"

int cport_nr = 0, // /dev/ttyS0 (COM1 on windows)
bdrate = 230400; // 57600 baud

// Read the config.ini file for the COM port to use
void read_config(void) {
	FILE* configfile = fopen ( "config.ini" , "rb" );
	char* buffer;
	if (configfile != NULL) {
		// Allocate memory 
		buffer = (char*) malloc (sizeof(char) * 2);
		
		// Copy the file into the buffer, we only read 2 characters
		fread (buffer, 1, 2, configfile);
		cport_nr = atoi(buffer);
		
		fclose (configfile);
		free (buffer);
	}
}

// Read one letter from stdin
int read_one_letter(void) {
	int c = getchar();
	while (getchar() != '\n' && getchar() != EOF);
	return c;
}

// Write serial data to file - used for ROM and RAM dumping
void write_to_file(char* filename, char* cmd) {
	// Create a new file
	FILE *pFile = fopen(filename, "wb");
	RS232_cputs(cport_nr, cmd);
	
	// Wait a little bit until we start gettings some data
	#ifdef _MSC_VER
	Sleep(500);
	#else
	usleep(500000); // Sleep for 500 milliseconds
	#endif
	
	int Kbytesread = 0;
	int uptoKbytes = 1;
	unsigned char buf[4097];
	int n = 0;
	while(1) {
		n = RS232_PollComport(cport_nr, buf, 4096);
		
		if (n > 0) {
			buf[n] = 0;
			fwrite((char *) buf, 1, n, pFile);
			printf("#");
			Kbytesread += n;
			if (Kbytesread / 32768 == uptoKbytes) {
				printf("%iK", (Kbytesread/32768) * 32);
				uptoKbytes++;
			}
			fflush(stdout);
		}
		else {
			break;
		}
		
		#ifdef _MSC_VER
		Sleep(100);
		#else
		usleep(50000); // Sleep for 50 milliseconds
		#endif
	}
	
	fclose(pFile);
}

// Read from file to serial - used writing to RAM
void read_from_file(char* filename, char* cmd) {
	// Load a new file
	FILE *pFile = fopen(filename, "rb");
	if (pFile == NULL) {
		return;
	} 
	RS232_cputs(cport_nr, cmd);
	
	// Wait a little bit until we start gettings some data
	#ifdef _MSC_VER
	Sleep(500);
	#else
	usleep(500000); // Sleep for 500 milliseconds
	#endif
	
	int Kbytesread = 0;
	int uptoKbytes = 1;
	unsigned char buf[4097];
	unsigned char readbuf[100];
	int n = 0;
	while(1) {
		n = RS232_PollComport(cport_nr, buf, 4096);
		
		if (n > 0) {
			buf[n] = 0;
			//printf("%s",buf);
			// Exit if save is finished
			if (strstr(buf, "END")) {
				break;
			}
			
			fread((char *) readbuf, 1, 64, pFile);
			readbuf[64] = 0;
			
			// Becuase Sendbuf doesn't work properly, so send one byte at a time
			/*
			int z = 0;
			for (z = 0; z < 64; z++) {
				RS232_SendByte(cport_nr, readbuf[z]);
#ifdef _MSC_VER
//				Sleep(1);
#else
//				usleep(1000);
#endif
			}
			*/
			RS232_SendBuf(cport_nr, readbuf, 64);
			
			printf("#");
			Kbytesread += n;
			if (Kbytesread / 32768 == uptoKbytes) {
				printf("%iK", (Kbytesread/32768) * 32);
				uptoKbytes++;
			}
			fflush(stdout);
		}
		else {
			//break;
		}
		
		#ifdef _MSC_VER
		Sleep(1);
		#else
		usleep(50000); // Sleep for 50 milliseconds
		#endif
	}
	
	fclose(pFile);
}

int main() {
	int i;
	
	read_config();
	
  printf("SMCCartRead by osoumen based on\n");
	printf("GBCartRead by insideGadgets\n");
	printf("################################\n\n");
	
	printf("Opening COM PORT %d...\n\n", cport_nr+1);
	
	// Open COM port
	if(RS232_OpenComport(cport_nr, bdrate)) {
		printf("Can not open comport\n");
		return(0);
	}
	
	// Read smc cart header
	while (1) {
		#ifdef _MSC_VER
		Sleep(2000);
		#else
		usleep(2000000); // Sleep for 2 seconds
		#endif
		
		RS232_cputs(cport_nr, "HEADER\n");
		
		unsigned char buf[4096];
		unsigned char header[50];
		int headersize = 0;
		int n = 0;
		int waitingforheader = 0;
		while (1) {
			n = RS232_PollComport(cport_nr, buf, 4095);
			if (n > 0) {
				buf[n] = 0;
				if (headersize == 0) {
					strncpy((char *) header, (char *) buf, n);
				}
				else {
					strncat((char *) header, (char *) buf, n);
				}
				headersize += n;
			}
			waitingforheader++;
			if (waitingforheader >= 10) {
				break;
			}
			
			#ifdef _MSC_VER
			Sleep(100);
			#else
			usleep(100000); // Sleep for 2 seconds
			#endif
		}
		header[headersize] = '\0';
		
		// Split the string
		int headercounter = 0;
		char gametitle[80];
    int romLayout = 255;
		int cartridgeType = 255;
		int romSize = 255;
		int ramSize = 255;
		
		char* tokstr;
		tokstr = strtok ((char *) header, "\r\n");
		while (tokstr != NULL) {
			if (headercounter == 0) {
				strncpy(gametitle, tokstr, 30);
				for (i=20; i>=0; i--) {
					if (gametitle[i] != 0x20) {
						gametitle[i+1] = '\0';
						break;
					}
				}
				printf ("Game title... ");
				printf ("%s\n", gametitle);
			}
      if (headercounter == 1) {
        romLayout = atoi(tokstr);
        if ((romLayout & 0x0f) == 0) {
          printf ("LoROM");
        }
        else if ((romLayout & 0x0f) == 1) {
          printf ("HiROM");
        }
				else {
					printf ("Unknown %x\n", romLayout);
				}
        if ((romLayout & 0xf0) == 0x20) {
          printf (", not fast\n");
        }
        else if ((romLayout & 0xf0) == 0x30) {
          printf (", fast \n");
        }
				else {
					printf ("Unknown %x\n", romLayout);
				}
      }
			if (headercounter == 2) {
				printf ("Cartridge type... ");
				cartridgeType = atoi(tokstr);
				
				switch (cartridgeType) {
					case 0: printf ("ROM ONLY\n"); break;
					case 1: printf ("ROM and RAM but no battery\n"); break;
					case 2: printf ("ROM and save-RAM (with a battery)\n"); break;
					case 0x13: printf ("SuperFX with no battery\n"); break;
					case 0x14: printf ("SuperFX with no battery\n"); break;
					case 0x15: printf ("SuperFX with save-RAM\n"); break;
					case 0x1a: printf ("SuperFX with save-RAM\n"); break;
					case 0x34: printf ("SA-1\n"); break;
					case 0x35: printf ("SA-1\n"); break;
					default: printf ("Unknown\n");
				}
			}
			else if (headercounter == 3) {
				printf ("ROM size... ");
				romSize = 0x400 << atoi(tokstr);
        printf ("%dKbytes\n", romSize / 1024);
			}
			else if (headercounter == 4) {
				printf ("RAM size... ");
				ramSize = 0x400 << atoi(tokstr);
        printf ("%dKbytes\n", ramSize / 1024);
			}
			
			tokstr = strtok (NULL, "\r\n");
			headercounter++;
		}
		
		printf ("\nSelect an option below\n1. Dump ROM\n2. Save RAM\n3. Write RAM\n4. Exit\n");
		printf (">");
		
		char userInput = read_one_letter();
		
		char filename[30];
		if (userInput == '1') {    
			printf ("\nDumping ROM to %s.smc... ", gametitle);
			strncpy(filename, gametitle, 20);
			strcat(filename, ".smc");
			write_to_file(filename, "READROM\n");
		}
		else if (userInput == '2') {    
			printf ("\nDumping RAM to %s.srm... ", gametitle);
			strncpy(filename, gametitle, 20);
			strcat(filename, ".srm");
			write_to_file(filename, "READRAM\n");
		}
		else if (userInput == '3') { 
			printf ("\nGoing to write to RAM from %s.srm...", gametitle);
			printf ("\n*** This will erase the save game from your SNES Cartridge ***");
			printf ("\nPress y to continue or any other key to abort.");
			printf ("\n>");
			
			char userInputConfirm = read_one_letter();	
			
			if (userInputConfirm == 'y') {
				printf ("\nWriting to RAM from %s.srm... ", gametitle);
				strncpy(filename, gametitle, 20);
				strcat(filename, ".srm");
				read_from_file(filename, "WRITERAM\n");
			}
			else {
				printf ("Aborted.\n");
			}
		}
		else {  
			RS232_CloseComport(cport_nr);
			return(0);
		}
		
		printf("\n\n");
	}
	
	return(0);
}
