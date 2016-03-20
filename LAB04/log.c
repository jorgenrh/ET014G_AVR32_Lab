/**
 * Name         : log.c
 * Author       : Jørgen Ryther Hoem
 * Lab          : Lab 4 (ET014G)
 * Description  : Log functions, uses FAT service and SD/MMC driver
 */
#include <asf.h>
#include <string.h>
#include "log.h"



/*****  VARIABLES  ****************************************************/

// Sets the active memory slot
volatile uint8_t active_drive = 0;

// Active logfile that is used
volatile char logfile[MAX_FILE_PATH_LENGTH];

// Tells whether the logfile is open or not
volatile bool logfile_open = false;



/*****  PRIVATE PROTOTYPES  *******************************************/

// Reset navigator priv prototype
void reset_navigator(void);



/*****  FUNCTIONS  ****************************************************/

/*
 * Log initialization 
 *
 *  This function checks if any drives exists
 *  and tries to mount it. Returns true if successful
 *  or false otherwise. 
 */
bool log_init(uint8_t slot)
{
	// Set drive/disk slot
	active_drive = slot;
	
	// Check if drive(s) exists
	if (!nav_drive_nb() || slot > nav_drive_nb()-1) {
		printf("\r\nMemory slots not found (%d)\r\n", active_drive);
		return false;
	}
	
	// Try to mount drive if possible
	printf("\r\nTrying to mount drive %d\r\n", active_drive);
	if (mount_drive()) {
		printf("Try to format the drive using 'format'\r\n");
		return false;
	}
	
	// Return true if ok	
	return true;
}


/*
 * Log set file 
 *
 *  Selects (creates if necessary) logfile
 */
void log_set_file(char *filename)
{
	uint8_t i;
	uint8_t len = strlen(filename);
	
	// Make sure length of filename does not exceed limit
	if (len > MAX_FILE_PATH_LENGTH-1) len = MAX_FILE_PATH_LENGTH-1;
	
	// Copy filename to memory
	for (i=0; i < len; i++)
	{
		logfile[i] = filename[i];
	}
	logfile[i] = '\0';

	// Create new logfile
	reset_navigator();
	nav_file_create((FS_STRING)logfile);
}


/*
 * Log Start 
 *
 *  Tries to open logfile in append mode for writing
 */
bool log_start(void)
{
	logfile_open = false;
	
	// Reset navigator just in case
	reset_navigator();
	
	// Try to navigate to logfile
	if (!nav_setcwd((FS_STRING)logfile, true, true))
	{
		printf("Error: Could not open logfile (err: %d)\r\n", fs_g_status);
	}
	else
	{
		// Open logfile
		file_open(FOPEN_MODE_APPEND);
		logfile_open = true;
	}
	
	return logfile_open;
}



/*
 * Log write ADC
 *
 *  This function writes the system cycle count and
 *  ADC value (uint16 argument) to the logfile as comma separated 
 *  values if the file is open.
 */
void log_write_adc(uint32_t cy_count, uint16_t value)
{
	if (logfile_open)
	{
		// Convert data to string
		char buffer[24];
		sprintf(buffer, "%lu, %u", cy_count, value);
		
		// Write data to file
		file_write_buf((uint8_t*)buffer, strlen(buffer));
		file_putc('\r');
		file_putc('\n');
	}
	else printf("Error: Logfile not open\r\n");
}


/*
 * Log Stop 
 *
 *  Closes the logfile
 */
void log_stop(void)
{
	// Close logfile
	file_close();
	logfile_open = false;
}


/*
 * Format drive 
 *
 *  Format current drive to FAT16
 *  Returns true if successful
 */
bool format_drive(void)
{
	// Reset navigator
	reset_navigator();
	
	// Format drive to FAT16
	if (!nav_drive_format(FS_FORMAT_FAT))
	{
		return false;
	}

	return true;
}


/*
 * Mount drive
 *
 *  Drive/disk mount with user friendly error messages.
 *  Returns a FAT error or 0 if mounting is successful.
 */
uint8_t mount_drive(void)
{
	// Reset navigator
	reset_navigator();

	// Try to mount, print error message if not
	if (!nav_partition_mount())
	{
		printf("Error: ");
		
		switch (fs_g_status)
		{
			case FS_ERR_HW_NO_PRESENT:
			printf("Disk not present\r\n");
			break;
			case FS_ERR_HW:
			printf("Disk access error\r\n");
			break;
			case FS_ERR_NO_FORMAT:
			printf("Disk not formated\r\n");
			break;
			case FS_ERR_NO_PART:
			printf("No partition available on disk\r\n");
			break;
			case FS_ERR_NO_SUPPORT_PART:
			printf("Partition not supported\r\n");
			break;
			default :
			printf("Unknown system error\r\n");
			break;
		}
		
		return fs_g_status;
	}
	else printf("Partition mounted\r\n");
	
	return 0;
}


/*
 * Reset FAT navigator 
 *
 *  This function resets the FAT navigator and 
 *  set the drive/disk active
 */
void reset_navigator(void)
{
	// Reset navigator
	nav_reset();
	
	// Set drive
	nav_drive_set(active_drive);
}