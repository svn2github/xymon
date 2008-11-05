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

static char bea_rcsid[] = "$Id: do_bea.c,v 1.13 2006/06/09 22:23:49 henrik Rel $";

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
	while ((bol = strstr(bol, searchstr)) != NULL) {
		idxval = NULL;
		bol++;
		eoln = strchr(bol, '\n');
		if (eoln) *eoln = '\0';
		bol = strchr(bol, '='); 
		if (bol) bol = strchr(bol, '\"');
		if (bol) idxval = bol+1;
		if (bol) bol = strchr(bol+1, '\"');
		if (bol) {
			*bol = '\0';
			idxwalk = (bea_idx_t *)malloc(sizeof(bea_idx_t));
			idxwalk->idx = strdup(idxval);
			idxwalk->next = bea_idxhead;
			bea_idxhead = idxwalk;
			*bol = '\"';
		}
		if (eoln) *eoln = '\n';
	}
}

static unsigned long getintval(char *key, char *idx, char *buf)
{
	char keystr[1024];
	char *p;

	MEMDEFINE(keystr);

	sprintf(keystr, "\n%s.\"%s\" = INTEGER: ", key, idx);
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

	sprintf(keystr, "\n%s.\"%s\" = STRING: ", key, idx);
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

int do_bea_rrd(char *hostname, char *testname, char *msg, time_t tstamp)
{
	bea_idx_t *idxwalk;
	char *srvname, *objname, *p, *q;
	char *domname = NULL;

	if (bea_thread_tpl == NULL) bea_thread_tpl = setup_template(bea_thread_params);
	if (bea_memory_tpl == NULL) bea_memory_tpl = setup_template(bea_memory_params);

	p = strstr(msg, "\nDOMAIN:");
	if (p) {
		p += strlen("\nDOMAIN:"); 
		p += strspn(p, " \t");
		q = strchr(p, '\n'); if (q) *q = '\0';
		domname = strdup(p);
		if (q) *q = '\n';
	}

	find_idxes(msg, "\nBEA-WEBLOGIC-MIB::jrockitRuntimeIndex.");

	for (idxwalk = bea_idxhead; (idxwalk); idxwalk = idxwalk->next) {
		unsigned long freeheap, usedheap, totalheap;
		unsigned long freephysmem, usedphysmem, totalphysmem;
		unsigned long totalthreads, daemonthreads, nurserysize;

		p = getstrval("BEA-WEBLOGIC-MIB::jrockitRuntimeParent", idxwalk->idx, msg);
		if (p == NULL) continue;
		q = strchr(p, ':'); if (q) srvname = strdup(q+1); else srvname = strdup(p);

		if (domname) snprintf(rrdfn, sizeof(rrdfn)-1, "bea.memory.%s.%s.rrd", domname, srvname);
		else snprintf(rrdfn, sizeof(rrdfn)-1, "bea.memory.%s.rrd", srvname);
		rrdfn[sizeof(rrdfn)-1] = '\0';

		freeheap      = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeFreeHeap", idxwalk->idx, msg);
		usedheap      = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeUsedHeap", idxwalk->idx, msg);
		totalheap     = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeTotalHeap", idxwalk->idx, msg);
		freephysmem   = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeFreePhysicalMemory", idxwalk->idx, msg);
		usedphysmem   = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeUsedPhysicalMemory", idxwalk->idx, msg);
		totalphysmem  = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeTotalPhysicalMemory", idxwalk->idx, msg);
		totalthreads  = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeTotalNumberOfThreads", idxwalk->idx, msg);
		daemonthreads = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeNumberOfDaemonThreads", idxwalk->idx, msg);
		nurserysize   = getintval("BEA-WEBLOGIC-MIB::jrockitRuntimeTotalNurserySize", idxwalk->idx, msg);

		sprintf(rrdvalues, "%d:%ld:%ld:%ld:%ld:%ld:%ld:%ld:%ld:%ld", 
			(int) tstamp,
			freeheap, usedheap, totalheap,
			freephysmem, usedphysmem, totalphysmem,
			totalthreads, daemonthreads, nurserysize);
		create_and_update_rrd(hostname, rrdfn, bea_memory_params, bea_memory_tpl);

		xfree(srvname);
	}

	find_idxes(msg, "\nBEA-WEBLOGIC-MIB::executeQueueRuntimeIndex.");

	for (idxwalk = bea_idxhead; (idxwalk); idxwalk = idxwalk->next) {
		unsigned long curridlecount, currcount, totalcount;

		p = getstrval("BEA-WEBLOGIC-MIB::executeQueueRuntimeName", idxwalk->idx, msg);
		if (p == NULL) continue;
		objname = strdup(p);

		p = getstrval("BEA-WEBLOGIC-MIB::executeQueueRuntimeParent", idxwalk->idx, msg);
		if (p == NULL) { free(objname); continue; }
		q = strchr(p, ':'); if (q) srvname = strdup(q+1); else srvname = strdup(p);

		if (domname) snprintf(rrdfn, sizeof(rrdfn)-1, "bea.threads.%s.%s.%s.rrd", domname, srvname, objname);
		else snprintf(rrdfn, sizeof(rrdfn)-1, "bea.threads.%s.%s.rrd", srvname, objname);
		rrdfn[sizeof(rrdfn)-1] = '\0';

		curridlecount = getintval("BEA-WEBLOGIC-MIB::executeQueueRuntimeExecuteThreadCurrentIdleCount", 
					idxwalk->idx, msg);
		currcount = getintval("BEA-WEBLOGIC-MIB::executeQueueRuntimePendingRequestCurrentCount", 
					idxwalk->idx, msg);
		totalcount = getintval("BEA-WEBLOGIC-MIB::executeQueueRuntimeServicedRequestTotalCount", 
					idxwalk->idx, msg);

		sprintf(rrdvalues, "%d:%ld:%ld:%ld", (int)tstamp, curridlecount, currcount, totalcount);
		create_and_update_rrd(hostname, rrdfn, bea_thread_params, bea_thread_tpl);

		xfree(srvname); xfree(objname);
	}

	if (domname) xfree(domname);

	return 0;
}

