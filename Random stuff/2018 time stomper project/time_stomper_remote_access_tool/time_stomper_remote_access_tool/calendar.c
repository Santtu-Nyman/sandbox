#include "calendar.h"
//#include <stdio.h>

#define DECEMBER 12
#define NOVEMBER 11
#define OCTOBER 10
#define SEPTEMBER 9
#define AUGUST 8
#define JULY 7
#define JUNE 6
#define MAY 5
#define APRIL 4
#define MARCH 3
#define FEBRUARY 2
#define JANUARY 1

#define DAYS_PER_WEEK 7

const char * const 		WEEK_DAY[]							= {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const unsigned short  END_OF_MONTH[] 					= {31, 61, 92, 122, 153, 184,	214, 245,	275, 306,	337, 366}; // March..February
const unsigned short  FIFTY_THIRD_WEEK[] 			= {261, 574, 887, 1148, 1461}; // nth weeks have week number 53
const unsigned int 		SECONDS_PER_DAY 				= 86400;
const unsigned int 		SECONDS_PER_HOUR 				= 3600; 
const unsigned short 	EPOCH_YEAR 							= 2016;
const unsigned short 	DAYS_PER_BUNDLE 				= 1461;
const unsigned short 	DAYS_PER_BLOCK_NORMAL 	= 1095;
const unsigned short 	DAYS_PER_YEAR_LEAP 			= 366;
const unsigned short 	DAYS_PER_YEAR_NORMAL 		= 365;
const unsigned char 	SECONDS_PER_MINUTE 			= 60;
const unsigned char 	MINUTES_PER_HOUR 				= 60;
const unsigned char 	WEEKS_PER_YEAR 					= 52;
const unsigned char 	HOURS_PER_DAY 					= 24;
const unsigned char 	MONTHS_PER_YEAR 				= 12;
const unsigned char 	EPOCH_WEEK_NUMBER 			= 9;
const unsigned char 	END_OF_DECEMBER					= 9;
const unsigned char 	END_OF_SEPTEMBER				= 6;
const unsigned char 	LAST_WEEK_DAY						= 6;
const unsigned char 	YEARS_PER_BUNDLE 				= 4;
const unsigned char 	YEARS_PER_BLOCK_NORMAL 	= 3;
const unsigned char 	EPOCH_MONTH 						= 3;
const unsigned char 	MONTH_OFFSET 						= 3;
const unsigned char   DAYLIGHT_START_HOUR			= 3;
const unsigned char		EPOCH_WEEK_DAY					= 1;
const unsigned char		ZEROTH_DAY						 	= 1;

static YearMonthDay getYearMonthDay(unsigned short days);
static Week getWeek(unsigned short days);
static unsigned short convertYMDtoDays(unsigned short year, unsigned char month, unsigned char day);
static unsigned short convertYearsToDays(unsigned short year, unsigned char month);
static unsigned short convertMonthToDays(unsigned char month);
static unsigned char handleMarch(Date date, unsigned char forward_march);
static unsigned char handleOctober(Date date, unsigned int seconds_since, unsigned char backward_october);
static unsigned char getForwardClock(Date date, unsigned int seconds_since);
static unsigned char getWeekNumber(unsigned short weeks);
static unsigned char getSummerTimeBorderDay(unsigned short year, unsigned char month);

/*******************getYearMonthDay*******************************************************************
General:	Calculates the year, the month and the day of month since epoch 2016/03/01 up to 2100/02/28. 
					(YYYY/MM/DD)

Accepts:	days:	The number of days since epoch.

Returns:  A <YearMonthDay> structure with year, month and day.
*****************************************************************************************************/
static YearMonthDay getYearMonthDay(unsigned short days){
	YearMonthDay ymd;
	unsigned short remainder; 																										// 0..365
	unsigned short stub = days % DAYS_PER_BUNDLE; 																// 0..1460
	unsigned short base = EPOCH_YEAR + days / DAYS_PER_BUNDLE * YEARS_PER_BUNDLE; // 2016..2100
	
	if(stub == 0){
		ymd.year = base;
		remainder = 0;
	}

	else if(stub <= DAYS_PER_BLOCK_NORMAL){ 				 // 0..1095
		ymd.year = base + stub / DAYS_PER_YEAR_NORMAL; // 2016..2100 + 0..3
		remainder = stub % DAYS_PER_YEAR_NORMAL; 			 // 0..364
	}

	else if(stub > DAYS_PER_BLOCK_NORMAL){														 // 1096..1460
		ymd.year = base + YEARS_PER_BLOCK_NORMAL;												 // 2016..2100 + 3
		remainder = (stub - DAYS_PER_BLOCK_NORMAL) % DAYS_PER_YEAR_LEAP; // 1..365
	}

	if(remainder + ZEROTH_DAY > END_OF_MONTH[END_OF_DECEMBER]){		
		ymd.year++;
	}
	
	for(unsigned char i = 0; i < MONTHS_PER_YEAR; ++i){
		if(ZEROTH_DAY + remainder <= END_OF_MONTH[i]){
			ymd.month = i + 3; 
			if(ymd.month > MONTHS_PER_YEAR){
				ymd.month %= MONTHS_PER_YEAR;
			}			
			if(i == 0){
				ymd.day = ZEROTH_DAY + remainder;
			}
			else {
				ymd.day = ZEROTH_DAY + remainder - END_OF_MONTH[i - 1];
			}
			break;
		}
	}	

	return ymd;
}

/***********getWeek**********************************************************************
General: Calculates the weekday and fetches the week number by calling the getWeekNumber.				 

Accepts: Number of days since epoch.

Returns: A <Week> structure with weekday and week number.
************getWeek*********************************************************************/
static Week getWeek(unsigned short days){
	unsigned short weeks = days / DAYS_PER_WEEK;
	unsigned char week_day = days % DAYS_PER_WEEK;
	unsigned short epoch_adjusted = weeks + EPOCH_WEEK_NUMBER;
	unsigned char week_number;

	if(week_day + EPOCH_WEEK_DAY > LAST_WEEK_DAY)
		epoch_adjusted++;	
	
	week_number = getWeekNumber(epoch_adjusted);
	Week week;
	week.number = week_number;
	week.day = (days + EPOCH_WEEK_DAY) % DAYS_PER_WEEK; // week day 0..6

	return week;
}

/*********************convertYMDtoDays**********************************************************
General:	Calculates the total number of days from a given year, a month, and the day of month.

Accepts:	year:  The year of the inspected date (2016-2100)
					month: The month of the inspected date(1-12)
					day:   The day of month of the inspected date (1-31)

Returns:	The total number of days from the given inputs. (1-7670)
**********************convertYMDtoDays********************************************************/
static unsigned short convertYMDtoDays(unsigned short year, unsigned char month, unsigned char day){
	unsigned short month_to_days = convertMonthToDays(month);
	unsigned short years_to_days = convertYearsToDays(year, month);
	unsigned short days_since = month_to_days + years_to_days + day - ZEROTH_DAY;
	return days_since;
}

/*********************convertYearsToDays***************************************************
General:	Calculates the amount of days for full years. Takes into account leap years.

Used by: 	convertYMDtoDays, a function that converts a <Date> structure into days.

Accepts:	year:  The year of the inspected date. (2016-2100)
					month: The month of the inspected date. (1-12) Month is required because the epoch
								 date begins on March the 1st 2016 meaning the first year is a stub year.

Returns:	The total number of days for full years since epoch date 2016 March the 1st.
**********************convertYearsToDays******************************************************/
static unsigned short convertYearsToDays(unsigned short year, unsigned char month){	
	unsigned char delta = year - EPOCH_YEAR;
	if(delta > 0){
		if(month < MARCH){			
			if(delta - 1 == 0)
				return 0;
			return (delta - 1) * DAYS_PER_YEAR_NORMAL + (delta - 1) / YEARS_PER_BUNDLE;
		}
		if(month >= MARCH)
			return delta * DAYS_PER_YEAR_NORMAL + delta / YEARS_PER_BUNDLE;
	}
	return 0;
}

/*********************convertMonthToDays*******************************************************
General:	Calculates the amount of days since the beginning of March 1st up to the beginning
					of the given month. E.g. month=4 (April) returns 31, because March has 31 days that
					is already past.

Used by: convertYMDtoDays, a function that converts a <Date> structure into days.

Accepts: month: A month between 1-12. (from January to December)

Returns: The cumulative number of days from prior months up to the beginning of given month.
				 A value in the range of 31-337. (from January to November)
**********************convertMonthToDays******************************************************/
static unsigned short convertMonthToDays(unsigned char month){
	if(month == MARCH) return 0;	
	return END_OF_MONTH[(month + MONTHS_PER_YEAR - MONTH_OFFSET - 1) % MONTHS_PER_YEAR];
}


/********************getWeekNumber***********************
General: Calculates the week number for getWeek.

Accepts: Number of 7-day periods (weeks) since epoch.

Returns: Calendar week number. 
*********************getWeekNumber**********************/
static unsigned char getWeekNumber(unsigned short weeks){
	unsigned char week_number;
	unsigned char k;
	weeks %= FIFTY_THIRD_WEEK[4];

	for(unsigned char i = 0, j = 5; i < j; ++i)
	{
		if(weeks == FIFTY_THIRD_WEEK[i])
			return 53;
		
		if(weeks < FIFTY_THIRD_WEEK[i])
		{
			k = i;
			break;
		}
	}

	weeks -= k;
	week_number = weeks % 52;

	if(week_number == 0)
		return 52;
	
	return week_number;
}

/********************getForwardClock********************************************************************
General:	Determines if the date.hms.hour field of <Date> structure should be forwarded by 1 hour due to
					daylight savings.

Accepts:	date: 					A copy of the inspected <Date> structure.
					seconds_since:	Seconds since epoch is required by another function this function calls.

Returns:	Returns either 0 or 1, indicating if the clock should be forwarded by 1 hour or not.
*********************getForwardClock*******************************************************************/
static unsigned char getForwardClock(Date date, unsigned int seconds_since){
	unsigned char forward_march	= getSummerTimeBorderDay(date.ymd.year, MARCH);
	unsigned char backward_october	= getSummerTimeBorderDay(date.ymd.year, OCTOBER);

	if(date.ymd.month > 3 && date.ymd.month < 10)
		return 1;

	if(date.ymd.month == 3)
		return handleMarch(date, forward_march);

	if(date.ymd.month == 10)
		return handleOctober(date, seconds_since, backward_october);
	
	return 0;
}

/********************handleMarch*************************************************************************
General:	Figures out if the date and its hour is past the summer time starting boundary i.e. at 03:00 am
					on last Sunday in March. This function is called by getForwardClock.

Accepts: 	date:					 A <Date> structure for the inspected date.
					forward_march: The day of March on which the summer time starting boundary is potentially being 
												 crossed. forward_march can be fetched by calling getSummerTimeBorderDay.

Returns:	Returns either 0 or 1 indicating if the clock should be forwarded due to boundary crossing.
					0="no forward", 1="forward".
********************************************************************************************************/
static unsigned char handleMarch(Date date, unsigned char forward_march){	
	if(date.ymd.day < forward_march)
		return 0;

	if(date.ymd.day == forward_march && date.hms.hour < 3)
		return 0;	

	return 1;
}

/********************handleOctober***************************************************************************
General:	Same as handleMarch except figures out the date and its hour past the summer time ending boundary
					which happens at 04:00 am on last Sunday in October. This function is called by getForwardClock.


Accepts:	date: 						A copy of the inspected <Date> structure.
					seconds_since: 		Seconds since epoch is required because we do not know if the date.hms.hour of
														<Date> structure is already being forwarded by 1 hour or not.
					backward_october:	The day of October on which the summer time ending boundary is potentially being
														crossed. backward_march can be fetched by calling getSummerTimeBorderDay.

Returns:  Returns either 0 or 1, indicating if the clock should be forwarded due to staying inside the
					boundaries of summer time. 0="no forward", 1="forward"
*********************handleOctober**************************************************************************/
static unsigned char handleOctober(Date date, unsigned int seconds_since, unsigned char backward_october){	
	if(date.ymd.day < backward_october)
		return 1;
	
	if(date.ymd.day == backward_october){
		if(seconds_since % SECONDS_PER_DAY / SECONDS_PER_HOUR < 3)
			return 1;		
	}
	return 0;
}

/********************getSummerTimeBorderDay********************************************
General:	
Accepts:	
Returns:  
*********************getSummerTimeBorderDay*******************************************/
static unsigned char getSummerTimeBorderDay(unsigned short year, unsigned char month){	
	unsigned char LAST_SUNDAY[DAYS_PER_WEEK];
	unsigned char delta = year - EPOCH_YEAR;
	unsigned char length = DAYS_PER_WEEK;

 	if(month == MARCH){
		LAST_SUNDAY[0] = 27;
		LAST_SUNDAY[1] = 26;
		LAST_SUNDAY[2] = 25;
		LAST_SUNDAY[3] = 31;
		LAST_SUNDAY[4] = 30;
		LAST_SUNDAY[5] = 29;
		LAST_SUNDAY[6] = 28;

 	}
	else if(month == OCTOBER){
		LAST_SUNDAY[0] = 30;
		LAST_SUNDAY[1] = 29;
		LAST_SUNDAY[2] = 28;
		LAST_SUNDAY[3] = 27;
		LAST_SUNDAY[4] = 26;
		LAST_SUNDAY[5] = 25;
		LAST_SUNDAY[6] = 31;
	}
	else 
		return 0;	

	unsigned char double_shifts = delta / YEARS_PER_BUNDLE;
	unsigned char shifts 				= delta - double_shifts;
	unsigned char long_index 		= double_shifts * 2 + shifts;
	unsigned char index 				= long_index % length;

	return LAST_SUNDAY[index];
}


/****getDate**********************************************************************
General: Converts seconds since epoch into meaningful <Date> structure.

Accepts: A number of seconds since epoch. Either user input or counted by a timer.

Returns:	A <Date> structure representing the time since Epoch 2016 March the 1st. 
*****getDate*********************************************************************/
Date getDate(unsigned int seconds_since){	
	unsigned int minutes_since = seconds_since / SECONDS_PER_MINUTE;
	unsigned int hours_since = minutes_since / MINUTES_PER_HOUR;
	unsigned short days_since = hours_since / HOURS_PER_DAY;
	unsigned char forward_clock;

	HourMinuteSecond hms;
	hms.hour = hours_since % HOURS_PER_DAY;
	hms.minute = minutes_since % MINUTES_PER_HOUR;
	hms.second = seconds_since % SECONDS_PER_MINUTE;

	Date date;
	date.ymd = getYearMonthDay(days_since); 
	date.week = getWeek(days_since); 				
	date.hms = hms;	

	forward_clock	= getForwardClock(date, seconds_since);

	if(forward_clock){
		hours_since++;
		if(hours_since % HOURS_PER_DAY == 0){
			days_since++;
			date.ymd = getYearMonthDay(days_since);
			date.week = getWeek(days_since);
		}
		date.hms.hour = hours_since % HOURS_PER_DAY;
	}
	return date;
}

/****makeDate**********************************************************
General:	Converts a user input date information into <Date> structure.

Accepts:  year:   A year from 2016 to 2100.
					month:  A month from 1 to 12.
					day:    A day of month from 1 - 31.
					hour:   An hour of a day from 0 to 23.
					minute: A minute of an hour from 0 to 59
					second: A second of a minute from 0 to 59 

NOTE: 		Forces all invalid inputs into valid ones, silently.

Returns: A valid <Date> structure.
*****makeDate**********************************************************/
Date makeDate(unsigned short year, unsigned char month, unsigned char day, 
							unsigned char hour, unsigned char minute, unsigned char second){
	Date date;
	Week week;

	if(year < 2016) year = 2016;	
	if(year > 2100) year = 2100;	

	if(year == 2016 && month < 3) month = 3;
	if(year == 2100 && month > 2) month = 2;
	if(month < 1) month = 1;	
	if(month > 12) month = 12;

	if(day < 1) day = 1;	
	if(day > 28) day = daysPerMonth(year, month);	

	unsigned short days_since = convertYMDtoDays(year, month, day);
	week = getWeek(days_since);
	
	if(hour > 23) hour = 23;	
	if(minute > 59) minute = 59;
	if(second > 59) second = 59;	
	
	date.ymd.year = year;
	date.ymd.month = month;
	date.ymd.day = day;
	date.hms.hour = hour;
	date.hms.minute = minute;
	date.hms.second = second;
	date.week = week;
	return date;
}

/****viewDate*****************************************************************************************************
General:	Shows the date information with printf -function calls in format YYYY.MM.DD Weekday hh:mm:ss WeekNumber.
					Intended for testing purposes where printf is available.

Accepts:  A <Date> structure.

Returns:  void
*****viewDate****************************************************************************************************/
/*
void viewDate(Date date){
	printf("%hu/", date.ymd.year);

	if(date.ymd.month < 10) printf("0");
	printf("%hhu/", date.ymd.month);

	if(date.ymd.day < 10) printf("0");
	printf("%hhu", date.ymd.day);

	printf(" %s ", WEEK_DAY[date.week.day]);
	
	if(date.hms.hour < 10) printf("0");	
	printf("%hhu:", date.hms.hour);	

	if(date.hms.minute < 10) printf("0");	
	printf("%hhu:", date.hms.minute);

	if(date.hms.second < 10) printf("0");	
	printf("%hhu Week ", date.hms.second);	

	if(date.week.number < 10) printf("0");
	printf("%hhu\n", date.week.number);
}
*/

/************convertDateToSeconds*****************************
General: Converts a <Date> structure into seconds since epoch.

Accepts: date: A <Date> structure.

Returns: The total number of seconds since epoch.
*************convertDateToSeconds****************************/
unsigned int convertDateToSeconds(Date date){	
	unsigned int seconds_since_0 = exactConvert(date, 0);
	unsigned int seconds_since_1 = exactConvert(date, 1);
	Date date_0 = getDate(seconds_since_0);
	Date date_1 = getDate(seconds_since_1);
	return date.hms.hour == date_0.hms.hour ? exactConvert(date_0, 0) : exactConvert(date_1, 1);	
}

/************exactConvert******************************************
General: 	

Accepts:

Returns:
*************exactConvert*****************************************/
unsigned int exactConvert(Date date, unsigned char repeating_hour){	
	unsigned short days_since = convertYMDtoDays(date.ymd.year, date.ymd.month, date.ymd.day);
	unsigned int hours_since = days_since * HOURS_PER_DAY + date.hms.hour;	
	if(repeating_hour){
		hours_since--;
	}	
	return (unsigned int)hours_since * SECONDS_PER_HOUR + date.hms.minute * SECONDS_PER_MINUTE + date.hms.second;
}

/*************daysPerMonth*********************************************************************
General:	Calculates the maximum amount of days a given month has, depending on the year.

Accepts:	year:  A year between 2016-2100.
					month: A month between 1-12. (from January to December)

Returns:	The maximum number of days in the given month.
**************daysPerMonth********************************************************************/
unsigned char daysPerMonth(unsigned short year, unsigned char month){
	switch(month){
		case JANUARY:
		case MARCH:
		case MAY:
		case JULY:
		case AUGUST:
		case OCTOBER:
		case DECEMBER:
		return 31;		

		case FEBRUARY:
		return year % YEARS_PER_BUNDLE == 0 ? 29 : 28;
		
		case APRIL:
		case JUNE:
		case SEPTEMBER:
		case NOVEMBER:
		return 30;
	}
	return 0;
}