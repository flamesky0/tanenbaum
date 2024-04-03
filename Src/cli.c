#include "main.h"
#include "ctype.h"

#define BUFLEN 256

static void print_help(void)
{
	char *help = "supported commands:\r\n"
		     "pin set [0|1] -- sets/resets LED1 on the board\r\n"
		     "i2c [read|write str] -- access to i2c flash\r\n"
		     "TBD\r\n";
	puts(help);
}

int match_pin_cmd(char *buf, int len)
{
	/* dont wanna check buf borders here */
	int i = 0;
	while (isblank(buf[i]))
		++i;
	if (!strncmp(buf + i, "pin", 3))
		i = i + 3;
	else
		return 0;

	while (isblank(buf[i]))
		++i;
	if (!strncmp(buf + i, "set", 3))
		i = i + 3;
	else
		return 0;

	while (isblank(buf[i]))
		++i;
	if (buf[i] == '0') {
		/* we do set pin when 0 because LED is pulled up on the board */
		LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_13);
		return 1;
	}
	if (buf[i] == '1') {
		LL_GPIO_ResetOutputPin(GPIOE, LL_GPIO_PIN_13);
		return 1;
	}
	/* not matched */
	return 0;
}

int match_i2c_cmd(char *buf, int len)
{
	while (isblank)
	/* not matched */
	return 0;
}

int parse_string(char *buf, int len)
{
	if (match_pin_cmd(buf, len))
		return 1;
	if (match_i2c_cmd(buf, len))
		return 1;
	return 0;
}

static void cli_task(void *pvParameters)
{
	char buf[BUFLEN];
	while (1) {
		printf("misha>>> ");
		fflush(stdout);
		fgets(buf, BUFLEN, stdin);
		if (!parse_string(buf, BUFLEN)) {
			printf("syntax error!\r\n");
			print_help();
		}
		fflush(stdin);
	}
}

void cli_setup()
{
	xTaskCreate(cli_task, "CLI", 256, NULL, CLI_TASK_PRIORITY, NULL);
}

