/*----------------------------------------------------------------------------*/
/* Hobbit RRD handler module.                                                 */
/*                                                                            */
/* Copyright (C) 2004-2006 Henrik Storner <henrik@hswn.dk>                    */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*
 * This is a backend handler for the statistics data that is collected by
 * my BEA snmp shell script. You'll find this script in the Hobbit
 * "scripts" directory.
 */

static char bea_rcsid[] = "$Id: do_bea2.c,v 1.3 2006/06/09 22:23:49 henrik Rel $";

static char *bea_memory_params[] = { "rrdcreate", rrdfn, 
				     "DS:freeheap:GAUGE:600:0:U",
				     "DS:usedheap:GAUGE:600:0:U",
				     "DS:totalheap:GAUGE:600:0:U",
				     "DS:freephysmem:GAUGE:600:0:U",
				     "DS:usedphysmem:GAUGE:600:0:U",
				     "DS:totalphysmem:GAUGE:600:0:U",
				     "DS:totalthreads:GAUGE:600:0:U",
				     "DS:daemonthreads:GAUGE:600:0:U",
				     "DS:nurserysize:GAUGE:600:0:U",
				     rra1, rra2, rra3, rra4, NULL };
static char *bea_memory_tpl      = NULL;

static char *bea_thread_params[] = { "rrdcreate", rrdfn,
				     "DS:currentidlecount:GAUGE:600:0:U",
				     "DS:currentcount:GAUGE:600:0:U",
				     "DS:totalcount:DERIVE:600:0:U",
				     rra1, rra2, rra3, rra4, NULL };
static char *bea_thread_tpl      = NULL;

typedef struct bea_idx_t {
	char *idx;
	struct bea_idx_t *next;
} bea_idx_t;


static bea_idx_t *bea_idxhead = NULL;


static void find_idxes(char *buf, char *searchstr)
{
	bea_idx_t *idxwalk;
	char *bol, *eoln, *idxval;

	/* If we've done it before, clear out the old indexes */
	while (bea_idxhead) {
		idxwalk = bea_idxhead;
		bea_idxhead = bea_idxhead->next;
		xfree(idxwalk->idx);
		xfree(idxwalk);
	}
	bea_idxhead = NULL;

	bol = buf;
	while (bol && ((bol = strstr(bol, searchstr)) != NULL)) {
		idxval = bol + strlen(searchstr);
		eoln = strchr(idxval, '\n'); if (eoln) *eoln = '\0';

		idxwalk = (bea_idx_t *)malloc(sizeof(bea_idx_t));
		idxwalk->idx = strdup(idxval);
		idxwalk->next = bea_idxhead;
		bea_idxhead = idxwalk;

		if (eoln) { *eoln = '\n'; bol = eoln; } else bol = NULL;
	}
}

static unsigned long getintval(char *key, char *idx, char *buf)
{
	char keystr[1024];
	char *p;

	MEMDEFINE(keystr);

	sprintf(keystr, "\nObject ID: %s.%s\nINTEGER: ", key, idx);
	p = strstr(buf, keystr);
	if (p) { MEMUNDEFINE(keystr); return atol(p + strlen(keystr)); }

	MEMUNDEFINE(keystr);
	return 0;
}

static unsigned char *getstrval(char *key, char *idx, char *buf)
{
	static char result[1024];
	char keystr[1024];
	char *p, *endp;
	char savech;

	MEMDEFINE(result); MEMDEFINE(keystr);

	sprintf(keystr, "\nObject ID: %s.%s\nSTRING: ", key, idx);
	p = strstr(buf, keystr);
	if (p) {
		p += strlen(keystr);
		p += strspn(p, " \t\"");
		endp = p + strcspn(p, "\"\r\n");
		savech = *endp;
		*endp = '\0';
		strncpy(result, p, sizeof(result)-1);
		*endp = savech;
		MEMUNDEFINE(result); MEMUNDEFINE(keystr);
		return result;
	}

	MEMUNDEFINE(result); MEMUNDEFINE(keystr);
	return NULL;
}

static void do_one_bea_domain(char *hostname, char *testname, char *domname, char *msg, time_t tstamp)
{
	bea_idx_t *idxwalk;
	char *srvname, *objname, *p, *q;

	find_idxes(msg, "\nObject ID: .iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeIndex.");

	for (idxwalk = bea_idxhead; (idxwalk); idxwalk = idxwalk->next) {
		unsigned long freeheap, usedheap, totalheap;
		unsigned long freephysmem, usedphysmem, totalphysmem;
		unsigned long totalthreads, daemonthreads, nurserysize;

		p = getstrval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeParent", idxwalk->idx, msg);
		if (p == NULL) continue;
		q = strchr(p, ':'); if (q) srvname = strdup(q+1); else srvname = strdup(p);

		if (domname) setupfn("bea.memory.%s.%s.rrd", domname, srvname);
		else setupfn("bea.memory.%s.rrd", srvname);

		freeheap      = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeFreeHeap", idxwalk->idx, msg);
		usedheap      = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeUsedHeap", idxwalk->idx, msg);
		totalheap     = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeTotalHeap", idxwalk->idx, msg);
		freephysmem   = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeFreePhysicalMemory", idxwalk->idx, msg);
		usedphysmem   = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeUsedPhysicalMemory", idxwalk->idx, msg);
		totalphysmem  = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeTotalPhysicalMemory", idxwalk->idx, msg);
		totalthreads  = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeTotalNumberOfThreads", idxwalk->idx, msg);
		daemonthreads = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeNumberOfDaemonThreads", idxwalk->idx, msg);
		nurserysize   = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.jrockitRuntimeTable.jrockitRuntimeEntry.jrockitRuntimeTotalNurserySize", idxwalk->idx, msg);

		sprintf(rrdvalues, "%d:%ld:%ld:%ld:%ld:%ld:%ld:%ld:%ld:%ld", 
			(int) tstamp,
			freeheap, usedheap, totalheap,
			freephysmem, usedphysmem, totalphysmem,
			totalthreads, daemonthreads, nurserysize);
		create_and_update_rrd(hostname, rrdfn, bea_memory_params, bea_memory_tpl);

		xfree(srvname);
	}

	find_idxes(msg, "\nObject ID: .iso.org.dod.internet.private.enterprises.bea.wls.executeQueueRuntimeTable.executeQueueRuntimeEntry.executeQueueRuntimeIndex.");

	for (idxwalk = bea_idxhead; (idxwalk); idxwalk = idxwalk->next) {
		unsigned long curridlecount, currcount, totalcount;

		p = getstrval(".iso.org.dod.internet.private.enterprises.bea.wls.executeQueueRuntimeTable.executeQueueRuntimeEntry.executeQueueRuntimeName", idxwalk->idx, msg);
		if (p == NULL) continue;
		objname = strdup(p);

		p = getstrval(".iso.org.dod.internet.private.enterprises.bea.wls.executeQueueRuntimeTable.executeQueueRuntimeEntry.executeQueueRuntimeParent", idxwalk->idx, msg);
		if (p == NULL) { free(objname); continue; }
		q = strchr(p, ':'); if (q) srvname = strdup(q+1); else srvname = strdup(p);

		if (domname) setupfn("bea.threads.%s.%s.%s.rrd", domname, srvname, objname);
		else setupfn("bea.threads.%s.%s.rrd", srvname, objname);

		curridlecount = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.executeQueueRuntimeTable.executeQueueRuntimeEntry.executeQueueRuntimeExecuteThreadCurrentIdleCount", 
					idxwalk->idx, msg);
		currcount = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.executeQueueRuntimeTable.executeQueueRuntimeEntry.executeQueueRuntimePendingRequestCurrentCount", 
					idxwalk->idx, msg);
		totalcount = getintval(".iso.org.dod.internet.private.enterprises.bea.wls.executeQueueRuntimeTable.executeQueueRuntimeEntry.executeQueueRuntimeServicedRequestTotalCount", 
					idxwalk->idx, msg);

		sprintf(rrdvalues, "%d:%ld:%ld:%ld", (int)tstamp, curridlecount, currcount, totalcount);
		create_and_update_rrd(hostname, rrdfn, bea_thread_params, bea_thread_tpl);

		xfree(srvname); xfree(objname);
	}
}

int do_bea_rrd(char *hostname, char *testname, char *msg, time_t tstamp)
{
	char *domname = NULL, *currdom;

	if (bea_thread_tpl == NULL) bea_thread_tpl = setup_template(bea_thread_params);
	if (bea_memory_tpl == NULL) bea_memory_tpl = setup_template(bea_memory_params);

	currdom = strstr(msg, "\nDOMAIN:");
	if (currdom) {
		while (currdom) {
			char *eol, *nextdom, savech;

			currdom += strlen("\nDOMAIN:"); currdom += strspn(currdom, " \t");
			eol = currdom + strcspn(currdom, " \t\r\n"); savech = *eol; *eol = '\0';
			domname = strdup(currdom);
			*eol = savech;

			nextdom = strstr(currdom, "\nDOMAIN:");
			if (nextdom) *nextdom = '\0';
			do_one_bea_domain(hostname, testname, domname, currdom, tstamp);
			if (nextdom) { *nextdom = '\n'; currdom = nextdom; } else currdom = NULL;

			xfree(domname);
		}
	}
	else {
		do_one_bea_domain(hostname, testname, NULL, msg, tstamp);
	}

	return 0;
}

