/*----------------------------------------------------------------------------*/
/* Hobbit monitor library.                                                    */
/*                                                                            */
/* This is a library module, part of libbbgen.                                */
/* It contains routines for timehandling.                                     */
/*                                                                            */
/* Copyright (C) 2002-2005 Henrik Storner <henrik@storner.dk>                 */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: timefunc.c,v 1.18 2005-03-27 06:56:44 henrik Exp $";

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "libbbgen.h"

char *timestamp = NULL;
void init_timestamp(void)
{
	time_t	now;

	if (timestamp == NULL) timestamp = (char *)malloc(30);

        now = time(NULL);
        strcpy(timestamp, ctime(&now));
        timestamp[strlen(timestamp)-1] = '\0';

}


char *weekday_text(char *dayspec)
{
	static char *result = NULL;
	static char *dayname[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	char *p;

	if (result == NULL) result = (char *)malloc(80);

	if (strcmp(dayspec, "*") == 0) {
		strcpy(result, "All days");
		return result;
	}

	result[0] = '\0';
	for (p=dayspec; (*p); p++) {
		switch (*p) {
			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6':
				strcat(result, dayname[(*p)-'0']);
				break;
			case '-':
				strcat(result, "-");
				break;
			case ' ':
			case ',':
				strcat(result, ",");
				break;
		}
	}
	return result;
}


char *time_text(char *timespec)
{
	static char *result = NULL;

	if (result == NULL) result = (char *)malloc(80);

	if (strcmp(timespec, "*") == 0) {
		strcpy(result, "0000-2359");
	}
	else {
		strcpy(result, timespec);
	}

	return result;
}


struct timeval *tvdiff(struct timeval *tstart, struct timeval *tend, struct timeval *result)
{
	static struct timeval resbuf;

	if (result == NULL) result = &resbuf;

	result->tv_sec = tend->tv_sec;
	result->tv_usec = tend->tv_usec;
	if (result->tv_usec < tstart->tv_usec) {
		result->tv_sec--;
		result->tv_usec += 1000000;
	}
	result->tv_sec  -= tstart->tv_sec;
	result->tv_usec -= tstart->tv_usec;

	return result;
}


static int minutes(char *p)
{
	/* Converts string HHMM to number indicating minutes since midnight (0-1440) */
	if (isdigit((int)*(p+0)) && isdigit((int)*(p+1)) && isdigit((int)*(p+2)) && isdigit((int)*(p+3))) {
		return (10*(*(p+0)-'0')+(*(p+1)-'0'))*60 + (10*(*(p+2)-'0')+(*(p+3)-'0'));
	}
	else {
		errprintf("Invalid timespec - expected 4 digits, got: '%s'\n", p);
		return 0;
	}
}

int within_sla(char *timespec, int defresult)
{
	/*
	 *    timespec is of the form W:HHMM:HHMM[,W:HHMM:HHMM]*
	 *    "W" = weekday : '*' = all, 'W' = Monday-Friday, '0'..'6' = Sunday ..Saturday
	 */

	int found = 0;
	time_t tnow;
	struct tm *now;
	int curtime;
	char *onesla;

	if (!timespec) return defresult;

	tnow = time(NULL);
	now = localtime(&tnow);
	curtime = now->tm_hour*60+now->tm_min;

	onesla = timespec;
	while (!found && onesla) {

		char *wday;
		int validday, wdaymatch = 0;
		char *endsla, *starttimep, *endtimep;
		int starttime, endtime;

		endsla = strchr(onesla, ','); if (endsla) *endsla = '\0';

		for (wday = onesla, validday=1; (validday && !wdaymatch); wday++) {
			switch (*wday) {
			  case '*':
				wdaymatch = 1;
				break;

			  case 'W':
			  case 'w':
				if ((now->tm_wday >= 1) && (now->tm_wday <=5)) wdaymatch = 1;
				break;

			  case '0': case '1': case '2': case '3': case '4': case '5': case '6':
				if (*wday == (now->tm_wday+'0')) wdaymatch = 1;
				break;

			  case ':':
				/* End of weekday spec. is OK */
				validday = 0;
				break;

			  default:
				errprintf("Bad timespec (missing colon or wrong weekdays): %s\n", onesla);
				validday = 0;
				break;
			}
		}

		if (wdaymatch) {
			/* Weekday matches */
			starttimep = strchr(onesla, ':');
			if (starttimep) {
				starttime = minutes(starttimep+1);
				endtimep = strchr(starttimep+1, ':');
				if (endtimep) {
					endtime = minutes(endtimep+1);
					if (endtime > starttime) {
						/* *:0200:0400 */
						found = ((curtime >= starttime) && (curtime <= endtime));
					}
					else {
						/* The period crosses over midnight: *:2330:0400 */
						found = ((curtime >= starttime) || (curtime <= endtime));
					}
					dprintf("\tstart,end,current time = %d, %d, %d - found=%d\n", 
						starttime,endtime,curtime,found);
				}
				else errprintf("Bad timespec (missing colon or no endtime): %s\n", onesla);
			}
			else errprintf("Bad timespec (missing colon or no starttime): %s\n", onesla);
		}
		else {
			dprintf("\tWeekday does not match\n");
		}

		/* Go to next SLA spec. */
		if (endsla) *endsla = ',';
		onesla = (endsla ? (endsla + 1) : NULL);
	}

	return found;
}


int periodcoversnow(char *tag)
{
	/*
	 * Tag format: "-DAY-HHMM-HHMM:"
	 * DAY = 0-6 (Sun .. Mon), or W (1..5)
	 */

	time_t tnow;
	struct tm *now;

        int result = 1;
        char *dayspec, *starttime, *endtime;
	unsigned int istart, iend, inow;
	char *p;

        if ((tag == NULL) || (*tag != '-')) return 1;

	dayspec = (char *) malloc(strlen(tag)+1+12); /* Leave room for expanding 'W' and '*' */
	starttime = (char *) malloc(strlen(tag)+1); 
	endtime = (char *) malloc(strlen(tag)+1); 

	strcpy(dayspec, (tag+1));
	for (p=dayspec; ((*p == 'W') || (*p == '*') || ((*p >= '0') && (*p <= '6'))); p++) ;
	if (*p != '-') {
		xfree(endtime); xfree(starttime); xfree(dayspec); return 1;
	}
	*p = '\0';

	p++;
	strcpy(starttime, p); p = starttime;
	if ( (strlen(starttime) < 4) || 
	     !isdigit((int) *p)            || 
	     !isdigit((int) *(p+1))        ||
	     !isdigit((int) *(p+2))        ||
	     !isdigit((int) *(p+3))        ||
	     !(*(p+4) == '-') )          goto out;
	else *(starttime+4) = '\0';

	p+=5;
	strcpy(endtime, p); p = endtime;
	if ( (strlen(endtime) < 4) || 
	     !isdigit((int) *p)          || 
	     !isdigit((int) *(p+1))      ||
	     !isdigit((int) *(p+2))      ||
	     !isdigit((int) *(p+3))      ||
	     !(*(p+4) == ':') )          goto out;
	else *(endtime+4) = '\0';

	tnow = time(NULL);
	now = localtime(&tnow);


	/* We have a timespec. So default to "not included" */
	result = 0;

	/* Check day-spec */
	if (strchr(dayspec, 'W')) strcat(dayspec, "12345");
	if (strchr(dayspec, '*')) strcat(dayspec, "0123456");
	if (strchr(dayspec, ('0' + now->tm_wday)) == NULL) goto out;

	/* Calculate minutes since midnight for start, end and now */
	istart = (600 * (starttime[0]-'0'))   +
		 (60  * (starttime[1]-'0'))   +
		 (10  * (starttime[2]-'0'))   +
		 (1   * (starttime[3]-'0'));
	iend   = (600 * (endtime[0]-'0'))     +
		 (60  * (endtime[1]-'0'))     +
		 (10  * (endtime[2]-'0'))     +
		 (1   * (endtime[3]-'0'));
	inow   = 60*now->tm_hour + now->tm_min;

	if ((inow < istart) || (inow > iend)) goto out;

	result = 1;
out:
	xfree(endtime); xfree(starttime); xfree(dayspec); 
	return result;
}

char *histlogtime(time_t histtime)
{
	static char *result = NULL;
	char d1[40],d2[3],d3[40];

	if (result == NULL) result = (char *)malloc(30);
	MEMDEFINE(d1); MEMDEFINE(d2); MEMDEFINE(d3);

	/*
	 * Historical logs use a filename like "Fri_Nov_7_16:01:08_2002 
	 * But apparently there is no simple way to generate a day-of-month 
	 * with no leading 0.
	 */

        strftime(d1, sizeof(d1), "%a_%b_", localtime(&histtime));
        strftime(d2, sizeof(d2), "%d", localtime(&histtime));
	if (d2[0] == '0') { d2[0] = d2[1]; d2[1] = '\0'; }
        strftime(d3, sizeof(d3), "_%H:%M:%S_%Y", localtime(&histtime));

	snprintf(result, 29, "%s%s%s", d1, d2, d3);

	MEMUNDEFINE(d1); MEMUNDEFINE(d2); MEMUNDEFINE(d3);

	return result;
}


int durationvalue(char *dur)
{
	/* 
	 * Calculate a duration, taking special modifiers into consideration.
	 * Return the duration as number of minutes.
	 */

	int result = 0;
	char *p;
	char modifier;

	p = dur + strspn(dur, "0123456789");
	modifier = *p;
	*p = '\0';
	result = atoi(dur);
	*p = modifier;

	switch (modifier) {
	  case 'm': break;			/* minutes */
	  case 'h': result *= 60; break;	/* hours */
	  case 'd': result *= 1440; break;	/* days */
	  case 'w': result *= 10080; break;	/* weeks */
	}

	return result;
}

char *durationstring(time_t secs)
{
	static char result[50];
	char *p = result;
	time_t v = secs;

	if (secs == 0) return "-";

	*result = '\0';

	if (v >= 7*24*60*60) {
		p += sprintf(p, "%dw ", (int)(v / (7*24*60*60)));
		v = (v % 7*24*60*60);
	}

	if (v >= 24*60*60) {
		p += sprintf(p, "%dd ", (int)(v / (24*60*60)));
		v = (v % 24*60*60);
	}

	if (v >= 60*60) {
		p += sprintf(p, "%dh ", (int)(v / (60*60)));
		v = (v % 60*60);
	}

	if (v >= 60) {
		p += sprintf(p, "%dm ", (int)(v / 60));
		v = (v % 60);
	}

	if (v > 0) {
		p += sprintf(p, "%ds ", (int)v);
	}

	return result;
}

