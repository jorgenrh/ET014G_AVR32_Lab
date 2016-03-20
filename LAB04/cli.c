/**
 * Name         : cli.c
 * Author       : Jørgen Ryther Hoem
 * Lab          : Lab 4 (ET014G)
 * Description  : Minimalistic Command Line Interface (CLI)
 */
#include <asf.h>
#include <string.h>
#include "cli.h"


/*****  DECLARATIONS  *************************************************/

// Maximum command buffer size
#define CLI_BUFFER_SIZE 40



/*****  VARIABLES  ****************************************************/

// Active command
volatile cli_command_t cli_command = CLI_CMD_NONE;
volatile cli_command_t cli_arg_cmd = CLI_CMD_NONE;

// Command buffer and index counter
volatile char cmd_buffer[CLI_BUFFER_SIZE];
volatile uint8_t cmd_index = 0;

// Argument buffer and active flag
volatile char cmd_argument[CLI_BUFFER_SIZE];
volatile bool cmd_arg_active = false;



/*****  PRIVATE PROTOTYPES  *******************************************/

// Private prototypes
void cli_build_cmd(char ch);
void cli_parse_cmd(char *cmd);
void cli_clear_cmd(void);



/*****  FUNCTIONS  ****************************************************/

/*
 * CLI task 
 *
 *  This function checks the stdin buffer for 
 *  data and sends it to the build cmd function. 
 */
void cli_task(void)
{
	char ch;
	scanf("%c",&ch);

	if (ch) {
		cli_build_cmd(ch);
	}
}

/*
 * CLI get command
 *
 *  Returns and then resets the command placeholder
 */
cli_command_t cli_get_command(void)
{
	cli_command_t cmd = cli_command;

	cli_clear_cmd();

	return cmd;
}


/*
 * CLI get argument
 *
 *  Returns the argument if arguments are required
 */
char* cli_get_argument(void)
{
	return (char*)cmd_argument;
}


/*****  PRIVATE FUNCTIONS  ********************************************/

/*
 * CLI clear command
 *
 *  Resets command
 */
void cli_clear_cmd(void)
{
	cli_command = CLI_CMD_NONE;
}


/*
 * CLI parse command
 *
 *  This function checks the buffer for commands
 *  and arguments
 */
void cli_parse_cmd(char *cmd)
{
	if (!cmd_arg_active)
	{
		if (!strcmp((char*)cmd, "start")) cli_command = CLI_CMD_START;
		else if (!strcmp((char*)cmd, "stop")) cli_command = CLI_CMD_STOP;
		else if (!strcmp((char*)cmd, "status")) cli_command = CLI_CMD_STATUS;
		else if (!strcmp((char*)cmd, "format")) cli_command = CLI_CMD_FORMAT;
		else if (!strcmp((char*)cmd, "file")) cli_arg_cmd = CLI_CMD_FILE;
		else if (!strcmp((char*)cmd, "help")) cli_command = CLI_CMD_HELP;
		else cli_command = CLI_CMD_UNKNOWN;
	}
	else 
	{
		uint8_t i;
		for (i=0; i < strlen(cmd); i++) cmd_argument[i] = cmd[i];
		cmd_argument[i] = '\0';
		
		// Deactivate argument flag		
		cmd_arg_active = false;		
		cli_command = cli_arg_cmd;
	}
}

/*
 * CLI build command
 *
 *  Resets command
 */
void cli_build_cmd(char ch)
{
	if (ch == '\n' || ch == ' ')
	{
		// Insert hex 0 at the end of buffer
		cmd_buffer[cmd_index] = '\0';
		
		// Send buffer to parser
		cli_parse_cmd((char*)cmd_buffer);
		
		// If space is detected assume argument
		if (ch == ' ') cmd_arg_active = true;
		
		// Clear buffer and reset index counter
		memset((char*)cmd_buffer, 0, CLI_BUFFER_SIZE);
		cmd_index = 0;
	}
	else if (ch != '\r') // ignore XR
	{
		// Fill buffer
		cmd_buffer[cmd_index++] = ch;
		
		// Reset buffer if size exceeds buffer limit
		if (cmd_index >= CLI_BUFFER_SIZE) cmd_index = 0;
	}
}





