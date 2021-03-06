/*----------------------------------------------------------------------------*/
/* Hobbit message daemon.                                                     */
/*                                                                            */
/* Client backend module for NetBSD                                           */
/*                                                                            */
/* Copyright (C) 2005 Henrik Storner <henrik@hswn.dk>                         */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char netbsd_rcsid[] = "$Id: netbsd.c,v 1.7 2005-07-25 09:18:39 henrik Exp $";

void handle_netbsd_client(char *hostname, namelist_t *hinfo, char *sender, time_t timestamp, char *clientdata)
{
	char *timestr;
	char *uptimestr;
	char *whostr;
	char *psstr;
	char *topstr;
	char *dfstr;
	char *meminfostr;
	char *swapctlstr;
	char *msgsstr;
	char *netstatstr;
	char *vmstatstr;

	char *p;

	char fromline[1024];

	sprintf(fromline, "\nStatus message received from %s\n", sender);

	splitmsg(clientdata);

	timestr = getdata("date");
	uptimestr = getdata("uptime");
	whostr = getdata("who");
	psstr = getdata("ps");
	topstr = getdata("top");
	dfstr = getdata("df");
	meminfostr = getdata("meminfo");
	swapctlstr = getdata("swapctl");
	msgsstr = getdata("msgsstr");
	netstatstr = getdata("netstat");
	vmstatstr = getdata("vmstat");

	combo_start();

	unix_cpu_report(hostname, hinfo, fromline, timestr, uptimestr, whostr, psstr, topstr);
	unix_disk_report(hostname, hinfo, fromline, timestr, "Capacity", "Mounted", dfstr);

	if (meminfostr) {
		unsigned long memphystotal, memphysfree, memphysused;
		unsigned long memswaptotal, memswapfree, memswapused;
		int found = 0;

		memphystotal = memphysfree = memphysused = 0;
		memswaptotal = memswapfree = memswapused = 0;

		p = strstr(meminfostr, "Total:"); if (p) { memphystotal = atol(p+6); found++; }
		p = strstr(meminfostr, "Free:");  if (p) { memphysfree  = atol(p+5); found++; }
		memphysused = memphystotal - memphysfree;
		p = strstr(meminfostr, "Swaptotal:"); if (p) { memswaptotal = atol(p+10); found++; }
		p = strstr(meminfostr, "Swapused:");  if (p) { memswapused  = atol(p+9); found++; }
		memswapfree = memswaptotal - memswapused;

		if (found == 4) {
			unix_memory_report(hostname, hinfo, fromline, timestr,
				   memphystotal, memphysused, -1, memswaptotal, memswapused);
		}
	}

	unix_procs_report(hostname, hinfo, fromline, timestr, "COMMAND", psstr);
	msgs_report(hostname, hinfo, fromline, timestr, msgsstr);

	combo_end();

	unix_netstat_report(hostname, hinfo, "netbsd", netstatstr);
	unix_vmstat_report(hostname, hinfo, "netbsd", vmstatstr);
}

