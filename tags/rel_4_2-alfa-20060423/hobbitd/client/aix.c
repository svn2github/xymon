/*----------------------------------------------------------------------------*/
/* Hobbit message daemon.                                                     */
/*                                                                            */
/* Client backend module for AIX                                              */
/*                                                                            */
/* Copyright (C) 2005 Henrik Storner <henrik@hswn.dk>                         */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char aix_rcsid[] = "$Id: aix.c,v 1.5 2006-04-20 07:08:59 henrik Exp $";

void handle_aix_client(char *hostname, enum ostype_t os, namelist_t *hinfo, char *sender, time_t timestamp, char *clientdata)
{
	char *timestr;
	char *uptimestr;
	char *whostr;
	char *psstr;
	char *topstr;
	char *dfstr;
	char *msgsstr;
	char *netstatstr;
	char *ifstatstr;
	char *portsstr;
	char *vmstatstr;
	char *realmemstr;
	char *freememstr;
	char *swapmemstr;

	char fromline[1024];

	sprintf(fromline, "\nStatus message received from %s\n", sender);

	splitmsg(clientdata);

	timestr = getdata("date");
	uptimestr = getdata("uptime");
	whostr = getdata("who");
	psstr = getdata("ps");
	topstr = getdata("top");
	dfstr = getdata("df");
	msgsstr = getdata("msgs");
	netstatstr = getdata("netstat");
	ifstatstr = getdata("ifstat");
	portsstr = getdata("ports");
	vmstatstr = getdata("vmstat");
	realmemstr = getdata("realmem");
	freememstr = getdata("freemem");
	swapmemstr = getdata("swap");

	unix_cpu_report(hostname, hinfo, fromline, timestr, uptimestr, whostr, psstr, topstr);
	unix_disk_report(hostname, hinfo, fromline, timestr, "%Used", "Mounted", dfstr);
	unix_procs_report(hostname, hinfo, fromline, timestr, "COMMAND", "CMD", psstr);
	msgs_report(hostname, hinfo, fromline, timestr, msgsstr);
	file_report(hostname, hinfo, fromline, timestr);

	if (realmemstr && freememstr && swapmemstr) {
		long memphystotal, memphysused, memphysfree, memswaptotal, memswappct;
		char *p;

		memphystotal = memphysused = memswaptotal = memswappct = 0;

		if (strncmp(realmemstr, "realmem ", 8) == 0) memphystotal = atoi(realmemstr+8) / 1024;
		if (sscanf(freememstr, "%*d %*d %*d %ld", &memphysfree) == 1) memphysfree /= 256;

		p = strchr(swapmemstr, '\n'); if (p) p++;
		if (p && (sscanf(p, " %ldMB %ld%%", &memswaptotal, &memswappct) != 2)) {
			memswaptotal = memswappct = -1;
		}

		unix_memory_report(hostname, hinfo, fromline, timestr,
				memphystotal, (memphystotal - memphysfree), -1,
				memswaptotal, ((memswaptotal * memswappct) / 100));
	}

	unix_netstat_report(hostname, hinfo, "aix", netstatstr);
	unix_ifstat_report(hostname, hinfo, "aix", ifstatstr);
	unix_ports_report(hostname, hinfo, fromline, timestr, 3, 4, 5, portsstr);

	unix_vmstat_report(hostname, hinfo, "aix", vmstatstr);
}

