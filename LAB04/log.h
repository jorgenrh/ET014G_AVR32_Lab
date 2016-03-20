/**
 * Name         : log.h
 * Author       : Jørgen Ryther Hoem
 * Lab          : Lab 4 (ET014G)
 * Description  : Log functions, uses FAT service and SD/MMC driver
 */
#ifndef LOG_H_
#define LOG_H_



/***** Log commands *****/

// Initiates log functionality 
bool log_init(uint8_t slot);

// Selects/create logfile
void log_set_file(char *filename);

// Opens logfile
bool log_start(void);

// Writes int value to logfile
void log_write_adc(uint32_t cy_count, uint16_t value);

// Closes logfile
void log_stop(void);



/***** FAT/drive utils *****/

// Formats drive
bool format_drive(void);

// Mounts drive with error messages
uint8_t mount_drive(void);



#endif /* LOG_H_ */