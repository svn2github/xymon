/*----------------------------------------------------------------------------*/
/* Hobbit RRD handler module.                                                 */
/*                                                                            */
/* This module handles data sent by the orcahobbit utility.                   */
/*                                                                            */
/* Copyright (C) 2006 Henrik Storner <henrik@hswn.dk>                         */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char orca_rcsid[] = "$Id: do_orca.c,v 1.1 2006/06/21 08:41:12 henrik Rel $";

typedef struct orca_fields_t {
	char *key;
	enum { F_GAUGE, F_COUNTER, F_GAUGE_PTN, F_COUNTER_PTN } ftype;
	char *fn;
	char *fnmod;
	char *val;
} orca_fields_t;

static char *orca_tstamp;
static char orca_ncpus[10];
static char orca_id[200];

static orca_fields_t orca_fields[] = {
	{ "state_D",   F_GAUGE, "gauge_state_D_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_N",   F_GAUGE, "gauge_state_N_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_n",   F_GAUGE, "gauge_state_n_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_s",   F_GAUGE, "gauge_state_s_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_r",   F_GAUGE, "gauge_state_r_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_k",   F_GAUGE, "gauge_state_k_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_c",   F_GAUGE, "gauge_state_c_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_m",   F_GAUGE, "gauge_state_m_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_d",   F_GAUGE, "gauge_state_d_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_i",   F_GAUGE, "gauge_state_i_per_%s.rrd",    orca_ncpus,  NULL },
	{ "state_t",   F_GAUGE, "gauge_state_t_per_%s.rrd",    orca_ncpus,  NULL },
	{ "usr%",      F_GAUGE, "gauge_usr_pct.rrd",           NULL,        NULL },
	{ "sys%",      F_GAUGE, "gauge_sys_pct.rrd",           NULL,        NULL },
	{ "wio%",      F_GAUGE, "gauge_wio_pct.rrd",           NULL,        NULL },
	{ "idle%",     F_GAUGE, "gauge_idle_pct.rrd",          NULL,        NULL },
	{ "1runq",     F_GAUGE, "gauge_1runq.rrd",             NULL,        NULL },
	{ "5runq",     F_GAUGE, "gauge_5runq.rrd",             NULL,        NULL },
	{ "15runq",    F_GAUGE, "gauge_15runq.rrd",            NULL,        NULL },
	{ "#proc",     F_GAUGE, "gauge_num_proc.rrd" ,         NULL,        NULL },
	{ "#runque",   F_GAUGE, "gauge_num_runque.rrd",        NULL,        NULL },
	{ "#waiting",  F_GAUGE, "gauge_num_waiting.rrd",       NULL,        NULL },
	{ "#swpque",   F_GAUGE, "gauge_num_swpque.rrd",        NULL,        NULL },
	{ "scanrate",  F_GAUGE, "gauge_num_scanrate.rrd",      NULL,        NULL },
	{ "#proc/s",   F_GAUGE, "gauge_num_proc_per_s.rrd",    NULL,        NULL },
	{ "#proc/p5s", F_GAUGE, "gauge_num_proc_per_p5s.rrd",  NULL,        NULL },
	{ "smtx",      F_GAUGE, "gauge_smtx.rrd",              NULL,        NULL },
	{ "smtx/cpu",  F_GAUGE, "gauge_smtx_per_cpu.rrd",      NULL,        NULL },
	{ "disk_rd/s", F_GAUGE, "gauge_disk_rd_per_s.rrd",     NULL,        NULL },
	{ "disk_wr/s", F_GAUGE, "gauge_disk_wr_per_s.rrd",     NULL,        NULL },
	{ "disk_rK/s", F_GAUGE, "gauge_disk_rK_per_s.rrd",     NULL,        NULL },
	{ "disk_wK/s", F_GAUGE, "gauge_disk_wK_per_s.rrd",     NULL,        NULL },
	{ "tape_rd/s", F_GAUGE, "gauge_tape_rd_per_s.rrd",     NULL,        NULL },
	{ "tape_wr/s", F_GAUGE, "gauge_tape_wr_per_s.rrd",     NULL,        NULL },
	{ "tape_rK/s", F_GAUGE, "gauge_tape_rK_per_s.rrd",     NULL,        NULL },
	{ "tape_wK/s", F_GAUGE, "gauge_tape_wK_per_s.rrd",     NULL,        NULL },
	{ "page_rstim",F_GAUGE, "gauge_page_rstim.rrd",        NULL,        NULL },
	{ "free_pages",F_GAUGE, "gauge_free_pages.rrd",        NULL,        NULL },
	{ "tcp_Iseg/s",F_GAUGE, "gauge_tcp_Iseg_per_s.rrd",    NULL,        NULL },
	{ "tcp_Oseg/s",F_GAUGE, "gauge_tcp_Oseg_per_s.rrd",    NULL,        NULL },
	{ "tcp_InKB/s",F_GAUGE, "gauge_tcp_InKB_per_s.rrd",    NULL,        NULL },
	{ "tcp_OuKB/s",F_GAUGE, "gauge_tcp_OuKB_per_s.rrd",    NULL,        NULL },
	{ "tcp_Ret%",  F_GAUGE, "gauge_tcp_Ret_pct.rrd",       NULL,        NULL },
	{ "tcp_Dup%",  F_GAUGE, "gauge_tcp_Dup_pct.rrd",       NULL,        NULL },
	{ "tcp_Icn/s", F_GAUGE, "gauge_tcp_Icn_per_s.rrd",     NULL,        NULL },
	{ "tcp_Ocn/s", F_GAUGE, "gauge_tcp_Ocn_per_s.rrd",     NULL,        NULL },
	{ "tcp_estb",  F_GAUGE, "gauge_tcp_estb.rrd",          NULL,        NULL },
	{ "tcp_Rst/s", F_GAUGE, "gauge_tcp_Rst_per_s.rrd",     NULL,        NULL },
	{ "tcp_Atf/s", F_GAUGE, "gauge_tcp_Atf_per_s.rrd",     NULL,        NULL },
	{ "tcp_Ldrp/s",F_GAUGE, "gauge_tcp_Ldrp_per_s.rrd",    NULL,        NULL },
	{ "tcp_LdQ0/s",F_GAUGE, "gauge_tcp_LdQ0_per_s.rrd",    NULL,        NULL },
	{ "tcp_HOdp/s",F_GAUGE, "gauge_tcp_HOdp_per_s.rrd",    NULL,        NULL },
	{ "nfs_call/s",F_GAUGE, "gauge_nfs_call_per_s.rrd",    NULL,        NULL },
	{ "nfs_timo/s",F_GAUGE, "gauge_nfs_timo_per_s.rrd",    NULL,        NULL },
	{ "nfs_badx/s",F_GAUGE, "gauge_nfs_badx_per_s.rrd",    NULL,        NULL },
	{ "nfss_calls",F_COUNTER, "counter_nfss_calls.rrd",    NULL,        NULL },
	{ "nfss_calls",F_COUNTER, "counter_nfss_calls.rrd",    NULL,        NULL },
	{ "nfss_bad",  F_COUNTER, "counter_nfss_bad.rrd",      NULL,        NULL },
	{ "v2reads",   F_COUNTER, "counter_v2reads.rrd",       NULL,        NULL },
	{ "v2writes",  F_COUNTER, "counter_v2writes.rrd",      NULL,        NULL },
	{ "v3reads",   F_COUNTER, "counter_v3reads.rrd",       NULL,        NULL },
	{ "v3writes",  F_COUNTER, "counter_v3writes.rrd",      NULL,        NULL },
	{ "dnlc_ref/s",F_GAUGE, "gauge_dnlc_ref_per_s.rrd",    NULL,        NULL },
	{ "dnlc_hit%", F_GAUGE, "gauge_dnlc_hit_pct.rrd",      NULL,        NULL },
	{ "inod_ref/s",F_GAUGE, "gauge_inod_ref_per_s.rrd",    NULL,        NULL },
	{ "inod_hit%", F_GAUGE, "gauge_inod_hit_pct.rrd",      NULL,        NULL },
	{ "inod_stl/s",F_GAUGE, "gauge_inod_stl_per_s.rrd",    NULL,        NULL },
	{ "pp_kernel", F_GAUGE, "gauge_pp_kernel.rrd",         NULL,        NULL },
	{ "pageslock", F_GAUGE, "gauge_pageslock.rrd",         NULL,        NULL },
	{ "pagestotl", F_GAUGE, "gauge_pagestotl.rrd",         NULL,        NULL },
	{ "(ce\\d+)Ipkt/s", F_GAUGE_PTN, "gauge_%sIpkt_per_s.rrd", orca_id,     NULL },
	{ "(ce\\d+)Opkt/s", F_GAUGE_PTN, "gauge_%sOpkt_per_s.rrd", orca_id,     NULL },
	{ "(ce\\d+)InKB/s", F_GAUGE_PTN, "gauge_%sInKB_per_s.rrd", orca_id,     NULL },
	{ "(ce\\d+)OuKB/s", F_GAUGE_PTN, "gauge_%sOuKB_per_s.rrd", orca_id,     NULL },
	{ "(ce\\d+)IErr/s", F_GAUGE_PTN, "gauge_%sIErr_per_s.rrd", orca_id,     NULL },
	{ "(ce\\d+)OErr/s", F_GAUGE_PTN, "gauge_%sOErr_per_s.rrd", orca_id,     NULL },
	{ "(ce\\d+)Coll%",  F_GAUGE_PTN, "gauge_%sColl_pct.rrd",   orca_id,     NULL },
	{ "(ce\\d+)NoCP/s", F_GAUGE_PTN, "gauge_%sNoCP_per_s.rrd", orca_id,     NULL },
	{ "(ce\\d+)Defr/s", F_GAUGE_PTN, "gauge_%sDefr_per_s.rrd", orca_id,     NULL },
	{ "disk_runp_(.+)", F_GAUGE_PTN, "gauge_disk_runp_%s.rrd", orca_id,     NULL },
	{ "mntP_(.+)",      F_GAUGE_PTN, "gauge_mtnP_%s.rrd",      orca_id,     NULL },
	{ "mntp_(.+)",      F_GAUGE_PTN, "gauge_mtnp_%s.rrd",      orca_id,     NULL },
	{ NULL,        F_GAUGE, NULL,                          NULL,        NULL }
};

static void orca_update(char *tstamp, char *val, char *hostname, char *fn, char **params, char *tpl)
{
	if (!tstamp) return;

	sprintf(rrdvalues, "%s:%s", tstamp, val);
	create_and_update_rrd(hostname, fn, params, tpl);
}

int do_orca_rrd(char *hostname, char *testname, char *msg, time_t tstamp) 
{ 
	static char *orca_gauge_params[]   = { "rrdcreate", rrdfn, "DS:Orca19990222:GAUGE:600:0:U", rra1, rra2, rra3, rra4, NULL };
	static char *orca_gauge_tpl        = NULL;
	static char *orca_counter_params[] = { "rrdcreate", rrdfn, "DS:Orca19990222:COUNTER:600:0:U", rra1, rra2, rra3, rra4, NULL };
	static char *orca_counter_tpl      = NULL;

	char *orcatstamp = NULL;
	char *l, *name, *val;

	if (orca_gauge_tpl == NULL) orca_gauge_tpl = setup_template(orca_gauge_params);
	if (orca_counter_tpl == NULL) orca_counter_tpl = setup_template(orca_counter_params);

	l = strchr(msg, '\n'); if (l) l++;
	while (l && *l && strncmp(l, "@@\n", 3)) {
		name = l; val = strchr(l, ':'); 
		if (val) { 
			*val = '\0';
			val++;
			l = strchr(val, '\n'); if (l) { *l = '\0'; l++; }
		}

		if (!name || !val) continue;

		if (strcmp(name, "timestamp") == 0) orcatstamp = val;
	}

	return 0;
}

