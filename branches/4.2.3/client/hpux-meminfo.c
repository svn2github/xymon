/*----------------------------------------------------------------------------*/
/* Hobbit memory information tool for HP-UX.                                  */
/* This tool retrieves information about the total and free RAM.              */
/*                                                                            */
/* Copyright (C) 2005-2006 Henrik Storner <henrik@hswn.dk>                    */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: hpux-meminfo.c,v 1.4 2006-05-03 21:12:33 henrik Exp $";

#include <sys/pstat.h>
#include <stdio.h>

main(int argc, char *argv[])
{
	struct pst_static sbuf;
	struct pst_dynamic dbuf;
	unsigned long pgsizekb;
	unsigned long kpages;

	pstat_getstatic(&sbuf, sizeof(sbuf), 1, 0);
	pstat_getdynamic(&dbuf, sizeof(dbuf), 1, 0);
	pgsizekb = sbuf.page_size / 1024;
	kpages = dbuf.psd_free / 1024;

	printf("Total:%ld\n", sbuf.physical_memory/256);
	printf("Free:%lu\n", pgsizekb*kpages);
}

