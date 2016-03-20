/**
 * Name         : cli.h
 * Author       : Jørgen Ryther Hoem
 * Lab          : Lab 4 (ET014G)
 * Description  : Minimalistic Command Line Interface (CLI)
 */
#ifndef CLI_H_
#define CLI_H_

// Help text
#define CLI_HELP_TEXT "Available commands:\r\n\r\n" \
                      "  start            start logging\r\n" \
                      "  stop             stops logging\r\n" \
                      "  status           shows current status\r\n" \
                      "  format           formats active drive\r\n" \
                      "  file <filename>  select/create logfile\r\n" \
                      "  help             displays this message\r\n\r\n"

// CLI commands
typedef enum {
	CLI_CMD_NONE = 0,
	CLI_CMD_START,
	CLI_CMD_STOP,
	CLI_CMD_STATUS,
	CLI_CMD_FORMAT,
	CLI_CMD_FILE,
	CLI_CMD_HELP,
	CLI_CMD_ARGUMENT,
	CLI_CMD_UNKNOWN
} cli_command_t;


// Checks for commands
void cli_task(void);

// Returns last command
cli_command_t cli_get_command(void);

// Returns argument if exists
char* cli_get_argument(void);


#endif /* CLI_H_ */