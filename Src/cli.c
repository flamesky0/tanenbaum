#include "main.h"
#include "ctype.h"

#define BUFLEN 256

static void print_help(void)
{
	char *help = "supported commands:\r\n"
		     "pin set [0|1] -- sets/resets LED1 on the board\r\n"
		     "time [set HH:MM:SS|get]\r\n"
		     "date [set YY::MM:DD |get]\r\n"
		     "i2c [read|write str] -- access to i2c flash\r\n"
		     "TBD\r\n";
	puts(help);
}

static int match_pin_cmd(const char *buf, int len)
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

/* function reads two symbols from str, checks validity and returns num if success */
static int get_num_from_str(const char *buf)
{
	int num;
	if (!isdigit((int) buf[0]) || !isdigit((int) buf[1]))
		return -1;
	num = (buf[0] - '0') * 10 + buf[1] - '0';
	return num;
}

static int set_time_from_str(const char *buf)
{
	int i = 0;
	int time, hours, minutes, seconds;
	hours = get_num_from_str(buf + i);
	if (hours < 0 || hours > 23)
		return 0;
	i = i + 2;
	if(buf[i] != ':')
		return 0;
	i = i + 1;

	minutes = get_num_from_str(buf + i);
	if (minutes < 0 || minutes > 59)
		return 0;
	i = i + 2;
	if(buf[i] != ':')
		return 0;
	i = i + 1;

	seconds = get_num_from_str(buf + i);
	if (seconds < 0 || seconds > 59)
		return 0;
	printf("Time is succesfully set %d:%d:%d!\r\n", hours, minutes, seconds);

	hours = __LL_RTC_CONVERT_BIN2BCD(hours);
	minutes = __LL_RTC_CONVERT_BIN2BCD(minutes);
	seconds = __LL_RTC_CONVERT_BIN2BCD(seconds);
	/* these magic masks are taken from reference manual (just offsets) */
	time = (hours << 16 & 0x003f0000) | (minutes << 8 & 0x00007f00) | (seconds & 0x0000007f);
	if (LL_RTC_EnterInitMode(RTC) == ERROR)
		return 0;
	/* because writing to rtc register individually doesn't work!!! */
	RTC->TR = time;
	LL_RTC_ExitInitMode(RTC);
	return 1;

}

static int match_time_cmd(const char *buf, int len)
{
	int i = 0;
	int time, hours, minutes, seconds;
	while (isblank(buf[i]))
		++i;
	if (!strncmp(buf + i, "time", 4))
		i = i + 4;
	else
		return 0;

	while (isblank(buf[i]))
		++i;
	if (!strncmp(buf + i, "set", 3)) {
		i = i + 3;
		while (isblank(buf[i]))
			++i;
		return set_time_from_str(buf + i);

	} else if (!strncmp(buf + i, "get", 3)) {
		time = LL_RTC_TIME_Get(RTC);
		hours = __LL_RTC_GET_HOUR(time);
		hours = __LL_RTC_CONVERT_BCD2BIN(hours);
		minutes = __LL_RTC_GET_MINUTE(time);
		minutes = __LL_RTC_CONVERT_BCD2BIN(minutes);
		seconds = __LL_RTC_GET_SECOND(time);
		seconds = __LL_RTC_CONVERT_BCD2BIN(seconds);
		printf("time is %02d:%02d:%02d\r\n", hours, minutes, seconds);
		return 1;
	} else {
		/* not matched */
		return 0;
	}
}

static int set_date_from_str(const char *buf)
{
	int i = 0;
	int date, year, month, day;
	year = get_num_from_str(buf + i);
	if (year < 0)
		return 0;
	i = i + 2;
	if(buf[i] != ':')
		return 0;
	i = i + 1;

	month = get_num_from_str(buf + i);
	if (month < 1 || month > 12)
		return 0;
	i = i + 2;
	if(buf[i] != ':')
		return 0;
	i = i + 1;

	day = get_num_from_str(buf + i);
	if (day < 1 || day > 31)
		return 0;

	month = __LL_RTC_CONVERT_BIN2BCD(month);
	year = __LL_RTC_CONVERT_BIN2BCD(year);
	day = __LL_RTC_CONVERT_BIN2BCD(day);
	/* + 0x20 because always is Monday */
	date = (year << 16 & 0x00ff0000) | ((month + 0x20) << 8 & 0x00001f00) | (day & 0x0000003f);
	if (LL_RTC_EnterInitMode(RTC) == ERROR)
		return 0;
	RTC->DR = date;
	LL_RTC_ExitInitMode(RTC);
	printf("Date is succesfully set!\r\n");
	return 1;

}

static int match_date_cmd(const char *buf, int len)
{
	int i = 0;
	int date, year, month, day;
	while (isblank(buf[i]))
		++i;
	if (!strncmp(buf + i, "date", 4))
		i = i + 4;
	else
		return 0;

	while (isblank(buf[i]))
		++i;
	if (!strncmp(buf + i, "set", 3)) {
		i = i + 3;
		while (isblank(buf[i]))
			++i;
		return set_date_from_str(buf + i);
	} else if (!strncmp(buf + i, "get", 3)) {
		date = LL_RTC_DATE_Get(RTC);
		year = __LL_RTC_GET_YEAR(date);
		year = __LL_RTC_CONVERT_BCD2BIN(year);
		month = __LL_RTC_GET_MONTH(date);
		month = __LL_RTC_CONVERT_BCD2BIN(month);
		day = __LL_RTC_GET_DAY(date);
		day = __LL_RTC_CONVERT_BCD2BIN(day);
		/* its 21st century today, babe! */
		printf("date is 20%02d:%02d:%02d\r\n", year, month, day);
		return 1;
	} else {
		/* not matched */
		return 0;
	}
}
static int match_i2c_cmd(const char *buf, int len)
{
	/* not matched */
	return 0;
}

int parse_string(char *buf, int len)
{
	if (match_pin_cmd(buf, len))
		return 1;
	if (match_i2c_cmd(buf, len))
		return 1;
	if (match_time_cmd(buf, len))
		return 1;
	if (match_date_cmd(buf, len))
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

