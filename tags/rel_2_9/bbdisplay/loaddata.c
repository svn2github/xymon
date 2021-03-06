/*----------------------------------------------------------------------------*/
/* Big Brother webpage generator tool.                                        */
/*                                                                            */
/* This is a replacement for the "mkbb.sh" and "mkbb2.sh" scripts from the    */
/* "Big Brother" monitoring tool from BB4 Technologies.                       */
/*                                                                            */
/* Primary reason for doing this: Shell scripts perform badly, and with a     */
/* medium-sized installation (~150 hosts) it takes several minutes to         */
/* generate the webpages. This is a problem, when the pages are used for      */
/* 24x7 monitoring of the system status.                                      */
/*                                                                            */
/* Copyright (C) 2002 Henrik Storner <henrik@storner.dk>                      */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: loaddata.c,v 1.108 2003-09-08 12:40:48 henrik Exp $";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include "bbgen.h"
#include "util.h"
#include "sendmsg.h"
#include "loaddata.h"
#include "reportdata.h"
#include "larrdgen.h"
#include "infogen.h"
#include "debug.h"

#define MAX_TARGETPAGES_PER_HOST 10

char    *nopropyellowdefault = NULL;
char    *nopropreddefault = NULL;
char	*larrdgraphs_default = NULL;
int     enable_purpleupd = 1;
int	purpledelay = 0;			/* Lifetime of purple status-messages. Default 0 for
						   compatibility with standard bb-display.sh behaviour */
char	*ignorecolumns = NULL;			/* Columns that will be ignored totally */
char    *wapcolumns = NULL;                     /* Default columns included in WAP cards */
char	*dialupskin = NULL;			/* BBSKIN used for dialup tests */
char	*reverseskin = NULL;			/* BBSKIN used for reverse tests */

link_t  null_link = { "", "", "", NULL };	/* Null link for pages/hosts/whatever with no link */
bbgen_col_t   null_column = { "", NULL };		/* Null column */
char	*null_text = "";

int	hostcount = 0;
int	statuscount = 0;
int	pagecount = 0;
int	purplecount = 0;
char	*purplelogfn = NULL;
static FILE *purplelog = NULL;
time_t	snapshot = 0;

static pagelist_t *pagelisthead = NULL;

static time_t oldestentry;


/* WEB prefixes for host notes and help-files */
char *notesskin = NULL;	/* BBNOTESSKIN */
char *helpskin = NULL;	/* BBHELPSKIN */


void addtopagelist(bbgen_page_t *page)
{
	pagelist_t *newitem;

	newitem = (pagelist_t *) malloc(sizeof(pagelist_t));
	newitem->pageentry = page;
	newitem->next = pagelisthead;
	pagelisthead = newitem;
}

char *build_noprop(const char *defset, const char *specset)
{
	static char result[MAX_LINE_LEN];
	char *set;
	char *item;
	char ibuf[MAX_LINE_LEN];
	char op;
	char *p;

	/* It's guaranteed that specset is non-NULL. defset may be NULL */
	if ((*specset != '+') && (*specset != '-')) {
		/* Old-style - specset is the full set of tests */
		sprintf(result, ",%s,", specset);
		return result;
	}

	set = malcop(specset);
	strcpy(result, ((defset != NULL) ? defset : ""));
	item = strtok(set, ",");

	while (item) {
		if ((*item == '-') || (*item == '+')) {
			op = *item;
			sprintf(ibuf, ",%s,", item+1);
		}
		else {
			op = '+';
			sprintf(ibuf, ",%s,", item);
		}

		p = strstr(result, ibuf);
		if (p && (op == '-')) {
			/* Remove this item */
			memmove(p, (p+strlen(item)), strlen(p));
		}
		else if ((p == NULL) && (op == '+')) {
			/* Add this item (it's not already included) */
			if (strlen(result) == 0) {
				sprintf(result, ",%s,", item+1);
			}
			else {
				strcat(result, item+1);
				strcat(result, ",");
			}
		}

		item = strtok(NULL, ",");
	}

	free(set);
	return ((strlen(result) > 0) ? result : NULL);
}

bbgen_page_t *init_page(const char *name, const char *title)
{
	bbgen_page_t *newpage = (bbgen_page_t *) malloc(sizeof(bbgen_page_t));

	pagecount++;
	dprintf("init_page(%s, %s)\n", textornull(name), textornull(title));

	if (name) {
		newpage->name = malcop(name);
	}
	else name = null_text;

	if (title) {
		newpage->title = malcop(title);
	}else
		title = null_text;

	newpage->color = -1;
	newpage->oldage = 1;
	newpage->pretitle = NULL;
	newpage->groups = NULL;
	newpage->hosts = NULL;
	newpage->parent = NULL;
	newpage->subpages = NULL;
	newpage->next = NULL;

	return newpage;
}

group_t *init_group(const char *title, const char *onlycols)
{
	group_t *newgroup = (group_t *) malloc(sizeof(group_t));

	dprintf("init_group(%s, %s)\n", textornull(title), textornull(onlycols));

	if (title) {
		newgroup->title = malcop(title);
	}
	else title = null_text;

	if (onlycols) {
		newgroup->onlycols = (char *) malloc(strlen(onlycols)+3); /* Add a '|' at start and end */
		sprintf(newgroup->onlycols, "|%s|", onlycols);
	}
	else newgroup->onlycols = NULL;
	newgroup->pretitle = NULL;
	newgroup->hosts = NULL;
	newgroup->next = NULL;
	return newgroup;
}

host_t *init_host(const char *hostname, const char *displayname, const char *comment, const char *description,
		  const int ip1, const int ip2, const int ip3, const int ip4, 
		  const int dialup, const int prefer, const double warnpct, const char *reporttime,
		  char *alerts, int nktime, char *waps, char *tags,
		  char *nopropyellowtests, char *nopropredtests, char *larrdgraphs, int modembanksize)
{
	host_t 		*newhost = (host_t *) malloc(sizeof(host_t));
	hostlist_t	*oldlist;

	hostcount++;
	dprintf("init_host(%s, %d,%d,%d.%d, %d, %d, %s, %s, %s, %s)\n", 
		textornull(hostname), ip1, ip2, ip3, ip4,
		dialup, prefer, textornull(alerts), textornull(tags),
		textornull(nopropyellowtests), textornull(nopropredtests));

	newhost->hostname = newhost->displayname = malcop(hostname);
	if (displayname) newhost->displayname = malcop(displayname);
	newhost->comment = (comment ? malcop(comment) : NULL);
	newhost->description = (description ? malcop(description) : NULL);
	sprintf(newhost->ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
	newhost->link = find_link(hostname);
	newhost->pretitle = NULL;
	newhost->entries = NULL;
	newhost->color = -1;
	newhost->oldage = 1;
	newhost->prefer = prefer;
	newhost->dialup = dialup;
	newhost->reportwarnlevel = warnpct;
	newhost->reporttime = (reporttime ? malcop(reporttime) : NULL);
	if (alerts && nktime) {
		char *p;
		p = skipword(alerts); if (*p) *p = '\0'; else p = NULL;

		newhost->alerts = (char *) malloc(strlen(alerts)+3);
		sprintf(newhost->alerts, ",%s,", alerts);
		if (p) *p = ' ';
	}
	else {
		newhost->alerts = NULL;
	}
	newhost->anywaps = 0;
	newhost->wapcolor = -1;

	/* Wap set is :
	 * - Specific WML: tag
	 * - NK: tag
	 * - --wap=COLUMN cmdline option
	 * - NULL
	 */
	if (waps || alerts) {
		char *p;
		p = skipword((waps ? waps : alerts)); if (*p) *p = '\0'; else p = NULL;
		newhost->waps = malcop(build_noprop(wapcolumns, (waps ? waps : alerts)));
		if (p) *p = ' ';
	}
	else {
		newhost->waps = wapcolumns;
	}

	if (nopropyellowtests) {
		char *p;
		p = skipword(nopropyellowtests); if (*p) *p = '\0'; else p = NULL;
		newhost->nopropyellowtests = malcop(build_noprop(nopropyellowdefault, nopropyellowtests));
		if (p) *p = ' ';
	}
	else {
		newhost->nopropyellowtests = nopropyellowdefault;
	}
	if (nopropredtests) {
		char *p;
		p = skipword(nopropredtests); if (*p) *p = '\0'; else p = NULL;
		newhost->nopropredtests = malcop(build_noprop(nopropreddefault, nopropredtests));
		if (p) *p = ' ';
	}
	else {
		newhost->nopropredtests = nopropreddefault;
	}
	if (larrdgraphs) {
		char *p;
		p = skipword(larrdgraphs); if (*p) *p = '\0'; else p = NULL;
		newhost->larrdgraphs = malcop(larrdgraphs);
		if (p) *p = ' ';
	}
	else newhost->larrdgraphs = larrdgraphs_default;
	if (tags) {
		newhost->rawentry = malcop(tags);
	}
	else newhost->rawentry = null_text;
	newhost->parent = NULL;
	newhost->rrds = NULL;
	newhost->banks = NULL;
	newhost->banksize = modembanksize;
	if (modembanksize) {
		int i;
		newhost->banks = (int *) malloc(modembanksize * sizeof(int));
		for (i=0; i<modembanksize; i++) newhost->banks[i] = -1;

		if (comment) {
			newhost->comment = (char *) realloc(newhost->comment, strlen(newhost->comment) + 22);
			sprintf(newhost->comment+strlen(newhost->comment), " - [%s]", newhost->ip);
		}
		else {
			newhost->comment = newhost->ip;
		}
	}
	newhost->next = NULL;

	/*
	 * Add this host to the hostlist_t list of known hosts.
	 * HOWEVER: It might be a duplicate! In that case, we need
	 * to figure out which host record we want to use.
	 */
	for (oldlist = hosthead; (oldlist && (strcmp(oldlist->hostentry->hostname, hostname) != 0)); oldlist = oldlist->next) ;
	if (oldlist == NULL) {
		hostlist_t *newlist;

		newlist = (hostlist_t *) malloc(sizeof(hostlist_t));
		newlist->hostentry = newhost;
		newlist->clones = NULL;
		newlist->next = hosthead;
		hosthead = newlist;
	}
	else {
		int usenew = 0;
		hostlist_t *clone = (hostlist_t *) malloc(sizeof(hostlist_t));

		dprintf("Duplicate host definition for host '%s'\n", hostname);

		if (newhost->prefer && !oldlist->hostentry->prefer) {
			usenew = 1;
			dprintf("Using new entry as it has 'prefer' tag and old entry does not\n");
		}
		else if (newhost->prefer && oldlist->hostentry->prefer) {
			usenew = 0;
			errprintf("Warning: Multiple prefer entries for host %s - using first one\n", hostname);
		}
		else if (!newhost->prefer && !oldlist->hostentry->prefer) {
			if ( (strcmp(oldlist->hostentry->ip, "0.0.0.0") == 0) && (strcmp(newhost->ip, "0.0.0.0") != 0) ) {
				usenew = 1;
				dprintf("Using new entry as old one has IP 0.0.0.0\n");
			}

			if ( strstr(oldlist->hostentry->rawentry, "noconn") && (strstr(newhost->rawentry, "noconn") == NULL) ) {
				usenew = 1;
				dprintf("Using new entry as old one has noconn\n");
			}
		}

		if (usenew) {
			clone->hostentry = oldlist->hostentry;
			clone->clones = NULL;
			clone->next = oldlist->clones;
			oldlist->clones = clone;

			oldlist->hostentry = newhost;
		}
		else {
			clone->hostentry = newhost;
			clone->clones = NULL;
			clone->next = oldlist->clones;
			oldlist->clones = clone;
		}
	}

	return newhost;
}

link_t *init_link(char *filename, const char *urlprefix)
{
	char *p;
	link_t *newlink = NULL;

	dprintf("init_link(%s, %s)\n", textornull(filename), textornull(urlprefix));

	newlink = (link_t *) malloc(sizeof(link_t));
	newlink->filename = malcop(filename);
	newlink->urlprefix = malcop(urlprefix);
	newlink->next = NULL;

	p = strrchr(filename, '.');
	if (p == NULL) p = (filename + strlen(filename));

	if ( (strcmp(p, ".php") == 0)    ||
             (strcmp(p, ".php3") == 0)   ||
             (strcmp(p, ".asp") == 0)    ||
             (strcmp(p, ".doc") == 0)    ||
	     (strcmp(p, ".shtml") == 0)  ||
	     (strcmp(p, ".phtml") == 0)  ||
	     (strcmp(p, ".html") == 0)   ||
	     (strcmp(p, ".htm") == 0))      
	{
		*p = '\0';
	}

	/* Without extension, this time */
	newlink->name = malcop(filename);

	return newlink;
}

bbgen_col_t *find_or_create_column(const char *testname)
{
	static bbgen_col_t *lastcol = NULL;	/* Cache the last lookup */
	bbgen_col_t *newcol;

	dprintf("find_or_create_column(%s)\n", textornull(testname));
	if (lastcol && (strcmp(testname, lastcol->name) == 0))
		return lastcol;

	for (newcol = colhead; (newcol && (strcmp(testname, newcol->name) != 0)); newcol = newcol->next);
	if (newcol == NULL) {
		newcol = (bbgen_col_t *) malloc(sizeof(bbgen_col_t));
		newcol->name = malcop(testname);
		newcol->link = find_link(testname);

		/* No need to maintain this list in order */
		if (colhead == NULL) {
			colhead = newcol;
			newcol->next = NULL;
		}
		else {
			newcol->next = colhead;
			colhead = newcol;
		}
	}
	lastcol = newcol;

	return newcol;
}

char *parse_testflags(char *l)
{
	char *result = NULL;
	char *flagstart = strstr(l, "[flags:");

	if (flagstart) {
		char *flagend;

		flagstart += 7;
		flagend = strchr(flagstart, ']');

		if (flagend) {
			*flagend = '\0';
			result = malcop(flagstart);
			*flagend = ']';
		}
	}

	return result;
}

int testflag_set(entry_t *e, char flag)
{
	if (e->testflags) 
		return (strchr(e->testflags, flag) != NULL);
	else
		return 0;
}


state_t *init_state(const char *filename, int dopurple, int *is_purple)
{
	FILE 		*fd;
	char		*p;
	char		*hostname;
	char		*testname;
	state_t 	*newstate;
	char		l[MAXMSG];
	char		fullfn[MAX_PATH];
	host_t		*host;
	struct stat 	log_st;
	time_t		now = time(NULL);
	time_t		histentry_start;

	statuscount++;
	dprintf("init_state(%s, %d, ...)\n", textornull(filename), dopurple);

	*is_purple = 0;

	/* Ignore summary files and dot-files (this catches "." and ".." also) */
	if ( (strncmp(filename, "summary.", 8) == 0) || (filename[0] == '.')) {
		return NULL;
	}

	if (reportstart || snapshot) {
		/* Dont do reports for info- and larrd-columns */
		p = strrchr(filename, '.');
		if (p == NULL) return NULL;
		p++;
		if (strcmp(p, infocol) == 0) return NULL;
		if (strcmp(p, larrdcol) == 0) return NULL;
	}

	sprintf(fullfn, "%s/%s", getenv(((reportstart || snapshot) ? "BBHIST" : "BBLOGS")), filename);

	/* Check that we can access this file */
	if ( (stat(fullfn, &log_st) == -1)       || 
	     (!S_ISREG(log_st.st_mode))            ||
	     ((fd = fopen(fullfn, "r")) == NULL)   ) {
		errprintf("Weird file %s/%s skipped\n", fullfn);
		return NULL;
	}

	/* Pick out host- and test-name */
	hostname = malcop(filename);
	p = strrchr(hostname, '.');

	/* Skip files that have no '.' in filename */
	if (p) {
		/* Pick out the testname ... */
		*p = '\0'; p++;
		testname = malcop(p);

		/* ... and change hostname back into normal form */
		for (p=hostname; (*p); p++) {
			if (*p == ',') *p='.';
		}
	}
	else {
		free(hostname);
		fclose(fd);
		return NULL;
	}

	sprintf(l, ",%s,", testname);
	if (ignorecolumns && strstr(ignorecolumns, l)) {
		free(hostname);
		free(testname);
		fclose(fd);
		return NULL;	/* Ignore this type of test */
	}

	host = find_host(hostname);

	/* If the host is a modem-bank host, dont mix in normal status messages */
	if (host && (host->banksize > 0)) {
		errprintf("Modembank %s has additional status-logs - ignored\n", hostname);
		return NULL;
	}

	newstate = (state_t *) malloc(sizeof(state_t));
	newstate->entry = (entry_t *) malloc(sizeof(entry_t));
	newstate->next = NULL;

	newstate->entry->column = find_or_create_column(testname);
	newstate->entry->color = -1;
	strcpy(newstate->entry->age, "");
	newstate->entry->oldage = 0;
	newstate->entry->propagate = 1;
	newstate->entry->testflags = NULL;
	newstate->entry->skin = NULL;
	newstate->entry->repinfo = NULL;
	newstate->entry->causes = NULL;
	newstate->entry->histlogname = NULL;

	if (host) {
		newstate->entry->alert = checkalert(host->alerts, testname);

		/* If no WAP's specified, default all tests to be on WAP page */
		newstate->entry->onwap = (host->waps ? checkalert(host->waps, testname) : 1);
	}
	else {
		dprintf("   hostname %s not found\n", hostname);
		newstate->entry->alert = newstate->entry->onwap = 0;
	}

	newstate->entry->sumurl = NULL;

	if (reportstart) {
		/* Determine "color" for this test from the historical data */
		newstate->entry->repinfo = (reportinfo_t *) calloc(1, sizeof(reportinfo_t));
		newstate->entry->color = parse_historyfile(fd, newstate->entry->repinfo, 
				(dynamicreport ? NULL: hostname), (dynamicreport ? NULL : testname), 
				reportstart, reportend, 0, 
				(host ? host->reportwarnlevel : reportwarnlevel), 
				reportgreenlevel,
				(host ? host->reporttime : NULL));
		newstate->entry->causes = (dynamicreport ? NULL : save_replogs());
	}
	else if (snapshot) {
		newstate->entry->color = history_color(fd, snapshot, &histentry_start, &newstate->entry->histlogname);
	}
	else if (fgets(l, sizeof(l), fd)) {
		newstate->entry->color = parse_color(l);
		newstate->entry->testflags = parse_testflags(l);
		if (testflag_set(newstate->entry, 'D')) newstate->entry->skin = dialupskin;
		if (testflag_set(newstate->entry, 'R')) newstate->entry->skin = reverseskin;
	}
	else {
		errprintf("Empty or unreadable status file %s/%s\n", ((reportstart || snapshot) ? "BBHIST" : "BBLOGS"), filename);
		newstate->entry->color = COL_CLEAR;
	}

	if ( !reportstart && !snapshot && (log_st.st_mtime <= now) && (strcmp(testname, larrdcol) != 0) && (strcmp(testname, infocol) != 0) ) {
		/* Log file too old = go purple */

		if (host && host->dialup) {
			/* Dialup hosts go clear, not purple */
			newstate->entry->color = COL_CLEAR;
		}
		else {
			/* Not in bb-hosts, or logfile too old */
			newstate->entry->color = COL_PURPLE;
			*is_purple = 1;
			purplecount++;
			if (purplelog) fprintf(purplelog, "%s %s%s\n", 
					       hostname, testname, (host ? " (expired)" : " (unknown host)"));
		}
	}

	/* Acked column ? */
	if (!reportstart && !snapshot && (newstate->entry->color != COL_GREEN)) {
		struct stat ack_st;
		char ackfilename[MAX_PATH];

		sprintf(ackfilename, "%s/ack.%s.%s", getenv("BBACKS"), hostname, testname);
		newstate->entry->acked = (stat(ackfilename, &ack_st) == 0);
	}
	else {
		newstate->entry->acked = 0;
	}

	newstate->entry->propagate = checkpropagation(host, testname, newstate->entry->color);

	if (reportstart) {
		/* Reports have no purple handling */
	}
	else if (snapshot) {
		time_t fileage = snapshot - histentry_start;

		newstate->entry->oldage = (fileage >= 86400);
		if (fileage >= 86400)
			sprintf(newstate->entry->age, "%.2f days", (fileage / 86400.0));
		else if (fileage > 3600)
			sprintf(newstate->entry->age, "%.2f hours", (fileage / 3600.0));
		else
			sprintf(newstate->entry->age, "%.2f minutes", (fileage / 60.0));
	}
	else if (dopurple && *is_purple) {
		/* Send a message to update status to purple */

		char *p;
		char *purplemsg;
		int bufleft = log_st.st_size + 1024;

		init_status(newstate->entry->color);

		for (p = strchr(l, ' '); (p && (*p == ' ')); p++); /* Skip old color */

		purplemsg = (char *) malloc(bufleft);
		sprintf(purplemsg, "status+%d %s.%s %s %s", purpledelay,
			commafy(hostname), testname,
                        colorname(newstate->entry->color), (p ? p : ""));
		bufleft -= strlen(purplemsg);

		if (host) {
			while (fgets(l, sizeof(l), fd)) {
				if (strncmp(l, "Status unchanged", 16) == 0) {
					char *p;

					p = strchr(l, '\n'); if (p) *p = '\0';
					strncat(newstate->entry->age, l+20, sizeof(newstate->entry->age)-1);
					newstate->entry->oldage = (strstr(l+20, "days") != NULL);
				}
				else if ( (strncmp(l, "Encrypted status message", 24) != 0)  &&
				          (strncmp(l, "Status message received from", 28) != 0) ) {
					strncat(purplemsg, l, bufleft);
				}
			}
			/* Avoid newlines piling up at end of logfile */
			for (p = purplemsg + strlen(purplemsg) - 1; 
				((p > purplemsg) && ((*p == '\n') || (*p == '\r')) ); p--) ;
			if (p>purplemsg) *(p+1) = '\0';
			strcat(purplemsg, "\n\n");
		}
		else {
			/* No longer in bb-hosts */
			sprintf(l, "%s\n\n", hostname);
			strcat(purplemsg, l);

			sprintf(l, "This entry is no longer listed in %s/etc/bb-hosts.  To remove this\n",
				getenv("BBHOME"));
			strcat(purplemsg, l);

			sprintf(l, "purple message, please delete the log files for this host located in\n");
			strcat(purplemsg, l);

			sprintf(l, "%s, %s and %s if this host is no longer monitored.\n",
				getenv("BBLOGS"), getenv("BBHIST"), getenv("BBHISTLOGS"));
			strcat(purplemsg, l);
		}

		addtostatus(purplemsg);
		free(purplemsg);
		finish_status();
	}
	else {
		if (*is_purple) {
			/* 
			 * dopurple is false, so we are not updating purple messages.
			 * That means we can use the age of the log file as an indicator
			 * for how old this status message really is.
			 */

			time_t fileage = (now - log_st.st_mtime);

			newstate->entry->oldage = (fileage >= 86400);
			if (fileage >= 86400)
				sprintf(newstate->entry->age, "%.2f days", (fileage / 86400.0));
			else if (fileage > 3600)
				sprintf(newstate->entry->age, "%.2f hours", (fileage / 3600.0));
			else
				sprintf(newstate->entry->age, "%.2f minutes", (fileage / 60.0));
		}
		else {
			if ((strcmp(testname, larrdcol) != 0) && (strcmp(testname, infocol) != 0)) {
				while (fgets(l, sizeof(l), fd) && (strncmp(l, "Status unchanged in ", 20) != 0)) ;

				if (strncmp(l, "Status unchanged in ", 20) == 0) {
					char *p;

					p = strchr(l, '\n'); if (p) *p = '\0';
					strncat(newstate->entry->age, l+20, sizeof(newstate->entry->age)-1);
					newstate->entry->oldage = (strstr(l+20, "days") != NULL);
				}
			}
			else {
				newstate->entry->oldage = 1;
			}
		}
	}

	dprintf("init_state: hostname=%s, testname=%s, color=%d, acked=%d, age=%s, oldage=%d, propagate=%d, alert=%d, *is_purple=%d\n",
		textornull(hostname), textornull(testname), 
		newstate->entry->color, newstate->entry->acked,
		textornull(newstate->entry->age), newstate->entry->oldage,
		newstate->entry->propagate, newstate->entry->alert, *is_purple);

	if (host) {
        	hostlist_t      *l;

		/* Add this state entry to the host's list of state entries. */
		newstate->entry->next = host->entries;
		host->entries = newstate->entry;

		/* There may be multiple host entries, if a host is
		 * listed in several locations in bb-hosts (for display purposes).
		 * This is handled by updating ALL of the cloned host records.
		 * Bug reported by Bluejay Adametz of Fuji.
		 */

		/* Cannot use "find_host()" here, as we need the hostlink record, not the host record */
		for (l=hosthead; (l && (strcmp(l->hostentry->hostname, host->hostname) != 0)); l=l->next);

		/* Walk through the clone-list and set the "entries" for all hosts */
		for (l=l->clones; (l); l = l->next) l->hostentry->entries = host->entries;
	}
	else {
		/* No host for this test - must be missing from bb-hosts */
		newstate->entry->next = NULL;
	}

	free(hostname);
	free(testname);
	fclose(fd);

	return newstate;
}

summary_t *init_summary(char *name, char *receiver, char *url)
{
	summary_t *newsum;

	dprintf("init_summary(%s, %s, %s)\n", textornull(name), textornull(receiver), textornull(url));

	/* Sanity check */
	if ((name == NULL) || (receiver == NULL) || (url == NULL)) 
		return NULL;

	newsum = (summary_t *) malloc(sizeof(summary_t));
	newsum->name = malcop(name);
	newsum->receiver = malcop(receiver);
	newsum->url = malcop(url);
	newsum->next = NULL;

	return newsum;
}

dispsummary_t *init_displaysummary(char *fn)
{
	FILE *fd;
	char sumfn[MAX_PATH];
	struct stat st;
	char l[MAX_LINE_LEN];
	dispsummary_t *newsum = NULL;

	dprintf("init_displaysummary(%s)\n", textornull(fn));

	sprintf(sumfn, "%s/%s", getenv("BBLOGS"), fn);

	/* Check that we can access this file */
	if ( (stat(sumfn, &st) == -1)          || 
	     (!S_ISREG(st.st_mode))            ||     /* Not a regular file */
	     ((fd = fopen(sumfn, "r")) == NULL)   ) {
		errprintf("Weird summary file BBLOGS/%s skipped\n", fn);
		return NULL;
	}

	if (st.st_mtime < time(NULL)) {
		/* Stale summary file - ignore and delete */
		errprintf("Stale summary file BBLOGS/%s - deleted\n", fn);
		unlink(sumfn);
		return NULL;
	}

	if (fgets(l, sizeof(l), fd)) {
		char *p, *rowcol;
		char *color = (char *) malloc(strlen(l));

		newsum = (dispsummary_t *) malloc(sizeof(dispsummary_t));
		newsum->url = (char *) malloc(strlen(l));

		sscanf(l, "%s %s", color, newsum->url);

		if (strncmp(color, "green", 5) == 0) {
			newsum->color = COL_GREEN;
		}
		else if (strncmp(color, "yellow", 6) == 0) {
			newsum->color = COL_YELLOW;
		}
		else if (strncmp(color, "red", 3) == 0) {
			newsum->color = COL_RED;
		}
		else if (strncmp(color, "blue", 4) == 0) {
			newsum->color = COL_BLUE;
		}
		else if (strncmp(color, "clear", 5) == 0) {
			newsum->color = COL_CLEAR;
		}
		else if (strncmp(color, "purple", 6) == 0) {
			newsum->color = COL_PURPLE;
		}

		rowcol = (char *) malloc(strlen(fn) + 1);
		strcpy(rowcol, fn+8);
		p = strrchr(rowcol, '.');
		if (p) *p = ' ';

		newsum->column = (char *) malloc(strlen(rowcol)+1);
		newsum->row = (char *) malloc(strlen(rowcol)+1);
		sscanf(rowcol, "%s %s", newsum->row, newsum->column);
		newsum->next = NULL;

		free(color);
		free(rowcol);
	}
	else {
		errprintf("Read error reading from file %s\n", sumfn);
		newsum = NULL;
	}


	fclose(fd);
	return newsum;
}

void init_modembank_status(char *fn)
{
	FILE *fd;
	char statusfn[MAX_PATH];
	struct stat st;
	char l[MAXMSG];
	host_t *targethost;

	dprintf("init_modembank_status(%s)\n", textornull(fn));

	sprintf(statusfn, "%s/%s", getenv("BBLOGS"), fn);

	/* Check that we can access this file */
	if ( (stat(statusfn, &st) == -1)          || 
	     (!S_ISREG(st.st_mode))            ||     /* Not a regular file */
	     ((fd = fopen(statusfn, "r")) == NULL)   ) {
		errprintf("Weird modembank/dialup logfile BBLOGS/%s skipped\n", fn);
		return;
	}

	if (st.st_mtime < time(NULL)) {
		/* Stale summary file - ignore and delete */
		errprintf("Stale modembank summary file BBLOGS/%s - deleted\n", fn);
		fclose(fd);
		unlink(statusfn);
		return;
	}

	targethost = find_host(fn+strlen("dialup."));
	if (targethost == NULL) {
		dprintf("Modembank status from unknown host %s - ignored\n", fn+strlen("dialup."));
		fclose(fd);
		return;
	}

	if (fgets(l, sizeof(l), fd)) {
		char *startip, *endip, *tag;
		int idx = -1;

		startip = endip = NULL;
		tag = strtok(l, " \n");
		while (tag) {
			if (idx >= 0) {
				/* Next result */
				if (idx < targethost->banksize) targethost->banks[idx] = parse_color(tag);
				idx++;
			}
			else if (strcmp(tag, "DATA") == 0) {
				if (startip && endip) idx = 0;
				else errprintf("Invalid modembank status logfile %s (missing FROM and/or TO)\n", fn);
			}
			else if (strcmp(tag, "FROM") == 0) {
				tag = strtok(NULL, " \n");

				if (tag) {
					startip = tag;
					if (strcmp(startip, targethost->ip) != 0) {
						errprintf("Modembank in bb-hosts begins with %s, but logfile begins with %s\n",
						  	targethost->ip, startip);
					}
				} else errprintf("Invalid modembank status logfile %s (truncated)\n", fn);
			}
			else if (strcmp(tag, "TO") == 0) {
				tag = strtok(NULL, " \n");

				if (tag) {
					if (startip) endip = tag;
					else errprintf("Invalid modembank status logfile %s (no FROM)\n", fn);
				} else errprintf("Invalid modembank status logfile %s (truncated)\n", fn);
			}

			if (tag) tag = strtok(NULL, " \n");
		}

		if ((idx >= 0) && (idx != targethost->banksize)) {
			errprintf("Modembank status log %s has more entries (%d) than expected (%d)\n", 
				  fn, (idx-1), targethost->banksize);
		}
	}

	fclose(fd);
}

void getnamelink(char *l, char **name, char **link)
{
	/* "page NAME title-or-link" splitup */
	char *p;

	dprintf("getnamelink(%s, ...)\n", textornull(l));

	*name = null_text;
	*link = null_text;

	/* Skip page/subpage keyword, and whitespace after that */
	p = skipwhitespace(skipword(l));

	*name = p; p = skipword(p);
	if (*p) {
		*p = '\0'; /* Null-terminate pagename */
		p++;
		*link = skipwhitespace(p);
	}
}


void getparentnamelink(char *l, bbgen_page_t *toppage, bbgen_page_t **parent, char **name, char **link)
{
	/* "subparent NAME PARENTNAME title-or-link" splitup */
	char *p;
	char *parentname;
	pagelist_t *walk;

	dprintf("getnamelink(%s, ...)\n", textornull(l));

	*name = null_text;
	*link = null_text;

	/* Skip page/subpage keyword, and whitespace after that */
	parentname = p = skipwhitespace(skipword(l));
	p = skipword(p);
	if (*p) {
		*p = '\0'; /* Null-terminate pagename */
		p++;
		*name = p = skipwhitespace(p);
	 	p = skipword(p);
		if (*p) {
			*p = '\0'; /* Null-terminate parentname */
			p++;
			*link = skipwhitespace(p);
		}
	}

	for (walk = pagelisthead; (walk && (strcmp(walk->pageentry->name, parentname) != 0)); walk = walk->next) ;
	if (walk) {
		*parent = walk->pageentry;
	}
	else {
		errprintf("Cannot find parent page '%s'\n", parentname);
		*parent = NULL;
	}
}


void getgrouptitle(char *l, char *pageset, char **title, char **onlycols)
{
	char grouponlytag[100], grouptag[100];

	*title = null_text;
	*onlycols = NULL;

	sprintf(grouponlytag, "%sgroup-only", pageset);
	sprintf(grouptag, "%sgroup", pageset);

	dprintf("getgrouptitle(%s, ...)\n", textornull(l));

	if (strncmp(l, grouponlytag, strlen(grouponlytag)) == 0) {
		char *p;

		*onlycols = skipwhitespace(skipword(l));

		p = skipword(*onlycols);
		if (*p) {
			*p = '\0'; p++;
			*title = skipwhitespace(p);
		}
	}
	else if (strncmp(l, grouptag, strlen(grouptag)) == 0) {
		*title = skipwhitespace(skipword(l));
	}
}

link_t *load_links(const char *directory, const char *urlprefix)
{
	DIR		*bblinks;
	struct dirent 	*d;
	char		fn[MAX_PATH];
	link_t		*curlink, *toplink, *newlink;

	dprintf("load_links(%s, %s)\n", textornull(directory), textornull(urlprefix));

	toplink = curlink = NULL;
	bblinks = opendir(directory);
	if (!bblinks) {
		errprintf("Cannot read links in directory %s\n", directory);
		return NULL;
	}

	while ((d = readdir(bblinks))) {
		strcpy(fn, d->d_name);
		newlink = init_link(fn, urlprefix);
		if (newlink) {
			if (toplink == NULL) {
				toplink = newlink;
			}
			else {
				curlink->next = newlink;
			}
			curlink = newlink;
		}
	}
	closedir(bblinks);
	return toplink;
}

link_t *load_all_links(void)
{
	link_t *l, *head1, *head2;
	char dirname[MAX_PATH];
	char *p;

	dprintf("load_all_links()\n");

	if (getenv("BBNOTESSKIN")) notesskin = malcop(getenv("BBNOTESSKIN"));
	else { 
		notesskin = (char *) malloc(strlen(getenv("BBWEB")) + strlen("/notes") + 1);
		sprintf(notesskin, "%s/notes", getenv("BBWEB"));
	}

	if (getenv("BBHELPSKIN")) helpskin = malcop(getenv("BBHELPSKIN"));
	else { 
		helpskin = (char *) malloc(strlen(getenv("BBWEB")) + strlen("/help") + 1);
		sprintf(helpskin, "%s/help", getenv("BBWEB"));
	}

	strcpy(dirname, getenv("BBNOTES"));
	head1 = load_links(dirname, notesskin);

	/* Change xxx/xxx/xxx/notes into xxx/xxx/xxx/help */
	p = strrchr(dirname, '/'); *p = '\0'; strcat(dirname, "/help");
	head2 = load_links(dirname, helpskin);

	if (head1) {
		/* Append help-links to list of notes-links */
		for (l = head1; (l->next); l = l->next) ;
		l->next = head2;
	}
	else {
		/* /notes was empty, so just return the /help list */
		head1 = head2;
	}

	return head1;
}


bbgen_page_t *load_bbhosts(char *pgset)
{
	FILE 	*bbhosts;
	char 	l[MAX_LINE_LEN], lcop[MAX_LINE_LEN];
	char	pagetag[100], subpagetag[100], subparenttag[100], 
		grouptag[100], summarytag[100], titletag[100], hosttag[100];
	char 	*name, *link, *onlycols;
	char 	hostname[MAX_LINE_LEN];
	bbgen_page_t 	*toppage, *curpage, *cursubpage, *cursubparent;
	group_t *curgroup;
	host_t	*curhost;
	char	*curtitle;
	int	ip1, ip2, ip3, ip4;
	int	modembanksize;
	char	*p;

	dprintf("load_bbhosts(pgset=%s)\n", textornull(pgset));

	bbhosts = stackfopen(getenv("BBHOSTS"), "r");
	if (bbhosts == NULL) {
		errprintf("Cannot open the BBHOSTS file '%s'\n", getenv("BBHOSTS"));
		return NULL;
	}

	if (pgset == NULL) pgset = "";
	sprintf(pagetag, "%spage", pgset);
	sprintf(subpagetag, "%ssubpage", pgset);
	sprintf(subparenttag, "%ssubparent", pgset);
	sprintf(grouptag, "%sgroup", pgset);
	sprintf(summarytag, "%ssummary", pgset);
	sprintf(titletag, "%stitle", pgset);
	sprintf(hosttag, "%s:", pgset); for (p=hosttag; (*p); p++) *p = toupper((int)*p);

	toppage = init_page("", "");
	addtopagelist(toppage);
	curpage = NULL;
	cursubpage = NULL;
	curgroup = NULL;
	curhost = NULL;
	cursubparent = NULL;
	curtitle = NULL;

	while (stackfgets(l, sizeof(l), "include")) {
		p = strchr(l, '\n'); 
		if (p) {
			*p = '\0'; 
		}
		else {
			errprintf("Warning: Lines in bb-hosts too long or has no newline: '%s'\n", l);
			fflush(stdout);
		}

		dprintf("load_bbhosts: -- got line '%s'\n", l);

		modembanksize = 0;

		if ((l[0] == '#') || (strlen(l) == 0)) {
			/* Do nothing - it's a comment */
		}
		else if (strncmp(l, pagetag, strlen(pagetag)) == 0) {
			getnamelink(l, &name, &link);
			if (curpage == NULL) {
				/* First page - hook it on toppage as a subpage from there */
				curpage = toppage->subpages = init_page(name, link);
			}
			else {
				curpage = curpage->next = init_page(name, link);
			}

			curpage->parent = toppage;
			if (curtitle) { 
				curpage->pretitle = curtitle; 
				curtitle = NULL; 
			}
			cursubpage = NULL;
			cursubparent = NULL;
			curgroup = NULL;
			curhost = NULL;
			addtopagelist(curpage);
		}
		else if (strncmp(l, subpagetag, strlen(subpagetag)) == 0) {
			getnamelink(l, &name, &link);
			if (cursubpage == NULL) {
				cursubpage = curpage->subpages = init_page(name, link);
			}
			else {
				cursubpage = cursubpage->next = init_page(name, link);
			}
			cursubpage->parent = curpage;
			if (curtitle) { 
				cursubpage->pretitle = curtitle; 
				curtitle = NULL;
			}
			cursubparent = NULL;
			curgroup = NULL;
			curhost = NULL;
			addtopagelist(cursubpage);
		}
		else if (strncmp(l, subparenttag, strlen(subparenttag)) == 0) {
			bbgen_page_t *parentpage, *walk;

			getparentnamelink(l, toppage, &parentpage, &name, &link);
			cursubparent = init_page(name, link);
			if (parentpage->subpages == NULL) {
				parentpage->subpages = cursubparent;
			} 
			else {
				for (walk = parentpage->subpages; (walk->next); (walk = walk->next)) ;
				walk->next = cursubparent;
			}
			if (curtitle) { 
				cursubparent->pretitle = curtitle; 
				curtitle = NULL;
			}
			cursubparent->parent = parentpage;
			curgroup = NULL;
			curhost = NULL;
			addtopagelist(cursubparent);
		}
		else if (strncmp(l, grouptag, strlen(grouptag)) == 0) {
			getgrouptitle(l, pgset, &link, &onlycols);
			if (curgroup == NULL) {
				curgroup = init_group(link, onlycols);
				if (cursubparent != NULL) {
					cursubparent->groups = curgroup;
				}
				else if (cursubpage != NULL) {
					/* We're in a subpage */
					cursubpage->groups = curgroup;
				}
				else if (curpage != NULL) {
					/* We're on a main page */
					curpage->groups = curgroup;
				}
				else {
					/* We're on the top page */
					toppage->groups = curgroup;
				}
			}
			else {
				curgroup->next = init_group(link, onlycols);
				curgroup = curgroup->next;
			}
			if (curtitle) { curgroup->pretitle = curtitle; curtitle = NULL; }
			curhost = NULL;
		}
		else if ( (sscanf(l, "%3d.%3d.%3d.%3d %s", &ip1, &ip2, &ip3, &ip4, hostname) == 5) ||
		          (!reportstart && !snapshot && (sscanf(l, "dialup %s %d.%d.%d.%d %d", hostname, &ip1, &ip2, &ip3, &ip4, &modembanksize) == 6) && (modembanksize > 0)) ) {
			int dialup = 0;
			int prefer = 0;
			int nodisp = 0;
			int nktime = 1;
			double warnpct = reportwarnlevel;
			char *alertlist, *onwaplist, *nopropyellowlist, *nopropredlist, *larrdgraphs, *reporttime;
			char *displayname, *comment, *description;
			char *targetpagelist[MAX_TARGETPAGES_PER_HOST];
			int targetpagecount;
			char *tag;
			char *startoftags = strchr(l, '#');

			displayname = NULL;

			/* If FQDN is not set, strip any domain off the hostname */
			if (!fqdn) {
				char *p = strchr(hostname, '.');
				if (p) {
					/* Save full name as "displayname", and modify hostname to be with no domain */
					displayname = malcop(hostname);
					*p = '\0';
				}
			}

			if (startoftags) {
				strcpy(lcop, startoftags+1);
				tag = strtok(lcop, " \t\r\n");
			}
			else tag = NULL;

			alertlist = onwaplist = nopropyellowlist = nopropredlist = larrdgraphs = reporttime = NULL;
			comment = description = NULL;
			for (targetpagecount=0; (targetpagecount < MAX_TARGETPAGES_PER_HOST); targetpagecount++) 
				targetpagelist[targetpagecount] = NULL;
			targetpagecount = 0;

			while (tag) {
				if (strcmp(tag, "dialup") == 0) 
					dialup = 1;
				else if (strcmp(tag, "prefer") == 0) 
					prefer = 1;
				else if ((strcmp(tag, "nodisp") == 0) || (strcmp(tag, "NODISP") == 0))
					nodisp = 1;
				else if (argnmatch(tag, "NK:")) 
					alertlist = malcop(tag+strlen("NK:"));
				else if (argnmatch(tag, "NKTIME=")) 
					nktime = within_sla(tag, "NKTIME", 1);
				else if (argnmatch(tag, "WML:")) 
					onwaplist = malcop(tag+strlen("WML:"));
				else if (argnmatch(tag, "NOPROP:")) 
					nopropyellowlist = malcop(tag+strlen("NOPROP:"));
				else if (argnmatch(tag, "NOPROPYELLOW:")) 
					nopropyellowlist = malcop(tag+strlen("NOPROPYELLOW:"));
				else if (argnmatch(tag, "NOPROPRED:")) 
					nopropredlist = malcop(tag+strlen("NOPROPRED:"));
				else if (argnmatch(tag, "LARRD:")) 
					larrdgraphs = malcop(tag+strlen("LARRD:"));
				else if (argnmatch(tag, "NAME:")) {
					p = tag+strlen("NAME:");
					displayname = (char *) malloc(strlen(l));
					if (*p == '\"') {
						p++;
						strcpy(displayname, p);
						p = strchr(displayname, '\"');
						if (p) *p = '\0'; 
						else {
							/* Scan forward to next " in input stream */
							tag = strtok(NULL, "\"\r\n");
							if (tag) {
								strcat(displayname, " ");
								strcat(displayname, tag);
							}
						}
					}
					else {
						strcpy(displayname, p);
					}
				}
				else if (argnmatch(tag, "COMMENT:")) {
					p = tag+strlen("COMMENT:");
					comment = (char *) malloc(strlen(l));
					if (*p == '\"') {
						p++;
						strcpy(comment, p);
						p = strchr(comment, '\"');
						if (p) *p = '\0'; 
						else {
							/* Scan forward to next " in input stream */
							tag = strtok(NULL, "\"\r\n");
							if (tag) {
								strcat(comment, " ");
								strcat(comment, tag);
							}
						}
					}
					else {
						strcpy(comment, p);
					}
				}
				else if (argnmatch(tag, "DESCR:")) {
					p = tag+strlen("DESCR:");
					description = (char *) malloc(strlen(l));
					if (*p == '\"') {
						p++;
						strcpy(description, p);
						p = strchr(description, '\"');
						if (p) *p = '\0'; 
						else {
							/* Scan forward to next " in input stream */
							tag = strtok(NULL, "\"\r\n");
							if (tag) {
								strcat(description, " ");
								strcat(description, tag);
							}
						}
					}
					else {
						strcpy(description, p);
					}
				}
				else if (argnmatch(tag, "WARNPCT:")) 
					warnpct = atof(tag+8);
				else if (argnmatch(tag, "REPORTTIME=")) 
					reporttime = malcop(tag);
				else if (argnmatch(tag, hosttag)) {
					targetpagelist[targetpagecount++] = malcop(tag+strlen(hosttag));
				}

				if (tag) tag = strtok(NULL, " \t\r\n");
			}

			if (nodisp) {
				/*
				 * Ignore this host.
				 */
			}
			else if (strlen(pgset) == 0) {
				/*
				 * Default pageset generated. Put the host into
				 * whatever group or page is current.
				 */
				if (curhost == NULL) {
					curhost = init_host(hostname, displayname, comment, description,
							    ip1, ip2, ip3, ip4, dialup, prefer, 
							    warnpct, reporttime,
							    alertlist, nktime, onwaplist,
							    startoftags, nopropyellowlist, nopropredlist,
							    larrdgraphs, modembanksize);
					if (curgroup != NULL) {
						curgroup->hosts = curhost;
					}
					else if (cursubparent != NULL) {
						cursubparent->hosts = curhost;
					}
					else if (cursubpage != NULL) {
						cursubpage->hosts = curhost;
					}
					else if (curpage != NULL) {
						curpage->hosts = curhost;
					}
					else {
						toppage->hosts = curhost;
					}
				}
				else {
					curhost = curhost->next = init_host(hostname, displayname, comment, description,
									    ip1, ip2, ip3, ip4, dialup, prefer, 
									    warnpct, reporttime,
									    alertlist, nktime, onwaplist,
									    startoftags, nopropyellowlist,nopropredlist,
									    larrdgraphs, modembanksize);
				}
				curhost->parent = (cursubparent ? cursubparent : (cursubpage ? cursubpage : curpage));
				if (curtitle) { curhost->pretitle = curtitle; curtitle = NULL; }
			}
			else if (targetpagecount) {

				int pgnum;

				for (pgnum=0; (pgnum < targetpagecount); pgnum++) {
					char *targetpagename = targetpagelist[pgnum];

					char savechar;
					int wantedgroup = 0;
					pagelist_t *targetpage = NULL;

					/* Put the host into the page specified by the PGSET: tag */
					p = strchr(targetpagename, ',');
					if (p) {
						savechar = *p;
						*p = '\0';
						wantedgroup = atoi(p+1);
					}
					else {
						savechar = '\0';
						p = targetpagename + strlen(targetpagename);
					}

					/* Find the page */
					for (targetpage = pagelisthead; (targetpage && (strcmp(targetpagename, targetpage->pageentry->name) != 0)); targetpage = targetpage->next) ;

					*p = savechar;
					if (targetpage == NULL) {
						errprintf("Warning: Cannot find any target page named %s - dropping host %s'\n", 
							targetpagename, hostname);
					}
					else {
						host_t *newhost = init_host(hostname, displayname, comment, description,
									    ip1, ip2, ip3, ip4, dialup, prefer, 
									    warnpct, reporttime,
									    alertlist, nktime, onwaplist,
									    startoftags, nopropyellowlist,nopropredlist,
									    larrdgraphs, modembanksize);

						if (wantedgroup > 0) {
							group_t *gwalk;
							host_t  *hwalk;
							int i;

							for (gwalk = targetpage->pageentry->groups, i=1; (gwalk && (i < wantedgroup)); i++,gwalk=gwalk->next) ;
							if (gwalk) {
								if (gwalk->hosts == NULL)
									gwalk->hosts = newhost;
								else {
									for (hwalk = gwalk->hosts; (hwalk->next); hwalk = hwalk->next) ;
									hwalk->next = newhost;
								}
							}
							else {
								errprintf("Warning: Cannot find group %d for host %s - dropping host\n",
									wantedgroup, hostname);
							}
						}
						else {
							/* Just put in on the page's hostlist */
							host_t *walk;
	
							if (targetpage->pageentry->hosts == NULL)
								targetpage->pageentry->hosts = newhost;
							else {
								for (walk = targetpage->pageentry->hosts; (walk->next); walk = walk->next) ;
								walk->next = newhost;
							}
						}

						newhost->parent = targetpage->pageentry;
						if (curtitle) newhost->pretitle = curtitle;
					}

					curtitle = NULL;
				}
			}

			if (displayname) free(displayname);
			if (description) free(description);
			if (comment) free(comment);
			if (alertlist) free(alertlist);
			if (onwaplist) free(onwaplist);
			if (nopropyellowlist) free(nopropyellowlist);
			if (nopropredlist) free(nopropredlist);
			if (larrdgraphs) free(larrdgraphs);
			if (reporttime) free(reporttime);
			for (targetpagecount=0; (targetpagecount < MAX_TARGETPAGES_PER_HOST); targetpagecount++) 
				if (targetpagelist[targetpagecount]) free(targetpagelist[targetpagecount]);
		}
		else if (strncmp(l, summarytag, strlen(summarytag)) == 0) {
			/* summary row.column      IP-ADDRESS-OF-PARENT    http://bb4.com/ */
			char sumname[MAX_LINE_LEN];
			char receiver[MAX_LINE_LEN];
			char url[MAX_LINE_LEN];
			summary_t *newsum;

			if (sscanf(l, "summary %s %s %s", sumname, receiver, url) == 3) {
				newsum = init_summary(sumname, receiver, url);
				newsum->next = sumhead;
				sumhead = newsum;
			}
		}
		else if (strncmp(l, titletag, strlen(titletag)) == 0) {
			/* Save the title for the next entry */
			curtitle = malcop(skipwhitespace(skipword(l)));
		}
		else {
		};
	}

	stackfclose(bbhosts);
	return toppage;
}


state_t *load_state(dispsummary_t **sumhead)
{
	DIR		*bblogs;
	struct dirent 	*d;
	char		fn[MAX_PATH];
	state_t		*newstate, *topstate;
	dispsummary_t	*newsum, *topsum;
	int		dopurple;
	struct stat	st;
	int		purplecount = 0;
	int		is_purple;

	dprintf("load_state()\n");

	if (chdir(getenv("BBLOGS")) != 0) {
		errprintf("Cannot access the BBLOGS directory %s\n", getenv("BBLOGS"));
		return NULL;
	}

	if (reportstart || snapshot) {
		dopurple = 0;
		purplelog = NULL;
		oldestentry = time(NULL);
	}
	else {
		if (stat(".bbstartup", &st) == -1) {
			/* Do purple if no ".bbstartup" file */
			dopurple = enable_purpleupd;
		}
		else {
			time_t now;

			/* Starting up - don't do purple hosts ("avoid purple explosion on startup") */
			dopurple = 0;

			/* Check if enough time has passed to remove the startup file */
			time(&now);
			if ((now - st.st_mtime) > 300) {
				remove(".bbstartup");
			}
		}

		if (purplelogfn) {
			purplelog = fopen(purplelogfn, "w");
			if (purplelog == NULL) errprintf("Cannot open purplelog file %s\n", purplelogfn);
			else fprintf(purplelog, "Stale (purple) logfiles as of %s\n\n", timestamp);
		}
		if (dopurple) combo_start();
	}

	topstate = NULL;
	topsum = NULL;

	bblogs = opendir(getenv("BBLOGS"));
	if (!bblogs) {
		errprintf("No logs! Cannot read the BBLOGS directory %s\n", getenv("BBLOGS"));
		return NULL;
	}

	while ((d = readdir(bblogs))) {
		strcpy(fn, d->d_name);

		if (strncmp(fn, "summary.", 8) == 0) {
			if (!reportstart && !snapshot) {
				newsum = init_displaysummary(fn);
				if (newsum) {
					newsum->next = topsum;
					topsum = newsum;
				}
			}
		}
		else if (strncmp(fn, "dialup.", 7) == 0) {
			init_modembank_status(fn);
		}
		else {
			is_purple = 0;

			newstate = init_state(fn, dopurple, &is_purple);
			if (newstate) {
				newstate->next = topstate;
				topstate = newstate;
				if (reportstart && (newstate->entry->repinfo->reportstart < oldestentry)) {
					oldestentry = newstate->entry->repinfo->reportstart;
				}
			}

			if (dopurple) {
				if (is_purple) purplecount++;
				if (purplecount > MAX_PURPLE_PER_RUN) {
					dopurple = 0;
					errprintf("%s : Too many purple updates (>%d) - disabling updates for purple logs\n", 
						timestamp, MAX_PURPLE_PER_RUN);
				}
			}
		}
	}

	closedir(bblogs);

	if (reportstart) sethostenv_report(oldestentry, reportend, reportwarnlevel, reportgreenlevel);
	if (dopurple) combo_end();
	if (purplelog) fclose(purplelog);

	*sumhead = topsum;
	return topstate;
}

