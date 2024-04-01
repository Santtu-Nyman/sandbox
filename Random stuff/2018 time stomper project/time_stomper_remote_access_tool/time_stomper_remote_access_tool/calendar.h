#ifndef IG_CALENDAR_H
#define IG_CALENDAR_H
//#include <stdio.h>

extern const char * const WEEK_DAY[];
extern const unsigned short END_OF_MONTH[];
extern const unsigned short FIFTY_THIRD_WEEK[];
extern const unsigned int 	SECONDS_PER_DAY;
extern const unsigned int 	SECONDS_PER_HOUR; 
extern const unsigned short EPOCH_YEAR;
extern const unsigned short DAYS_PER_BUNDLE;
extern const unsigned short DAYS_PER_BLOCK_NORMAL; 
extern const unsigned short DAYS_PER_YEAR_LEAP;
extern const unsigned short DAYS_PER_YEAR_NORMAL;
extern const unsigned char 	MINUTES_PER_HOUR;
extern const unsigned char 	SECONDS_PER_MINUTE;
extern const unsigned char 	WEEKS_PER_YEAR;
extern const unsigned char 	HOURS_PER_DAY;
extern const unsigned char 	MONTHS_PER_YEAR;
extern const unsigned char 	EPOCH_WEEK_NUMBER;
extern const unsigned char  END_OF_DECEMBER;
extern const unsigned char	END_OF_SEPTEMBER;
extern const unsigned char 	LAST_WEEK_DAY;
extern const unsigned char 	YEARS_PER_BUNDLE;
extern const unsigned char 	YEARS_PER_BLOCK_NORMAL;
extern const unsigned char	EPOCH_MONTH;
extern const unsigned char 	MONTH_OFFSET;
extern const unsigned char  DAYLIGHT_START_HOUR;
extern const unsigned char	EPOCH_WEEK_DAY;
extern const unsigned char	ZEROTH_DAY;

typedef struct {
	unsigned short year;
	unsigned char month;
	unsigned char day;
} YearMonthDay;

typedef struct {
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
} HourMinuteSecond;

typedef struct {
	unsigned char number;
	unsigned char day;
} Week;

typedef struct {
	YearMonthDay ymd;
	HourMinuteSecond hms;
	Week week;
} Date;

Date getDate(unsigned int seconds_since);
Date makeDate(unsigned short year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second);
//void viewDate(Date date);
unsigned int convertDateToSeconds(Date date);
unsigned int exactConvert(Date date, unsigned char repeating_hour);
unsigned char daysPerMonth(unsigned short year, unsigned char month);
#endif