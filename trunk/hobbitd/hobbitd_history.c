#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <sys/wait.h>

#include "bbdworker.h"

/*
 * This is a bbgend worker module.
 * It hangs off the "stachg" channel (fed via bbd_channel), 
 * and updates the standard BB history files.
 */

static void dropdirectory(char *dirfn)
{
	DIR *dirfd;
	struct dirent *de;
	char fn[MAX_PATH];
	struct stat st;

	dprintf("Starting to remove directory %s\n", dirfn);
	dirfd = opendir(dirfn);
	if (dirfd) {
		while ( (de = readdir(dirfd)) != NULL ) {
			sprintf(fn, "%s/%s", dirfn, de->d_name);
			if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..") && (stat(fn, &st) == 0)) {
				if (S_ISREG(st.st_mode)) {
					dprintf("Removing file %s\n", fn);
					unlink(fn);
				}
				else if (S_ISDIR(st.st_mode)) {
					dprintf("Recurse into %s\n", fn);
					dropdirectory(fn);
				}
			}
		}
		closedir(dirfd);
	}
	dprintf("Removing directory %s\n", dirfn);
	rmdir(dirfn);
}

void sig_handler(int signum)
{
	int status;

	switch (signum) {
	  case SIGCHLD:
		  wait(&status);
		  break;
	}
}

int main(int argc, char *argv[])
{
	char *histdir = NULL;
	char *histlogdir = NULL;
	char *msg;
	int argi, seq;
	int save_allevents = 1;
	int save_hostevents = 1;
	int save_statusevents = 1;
	int save_histlogs = 1;
	FILE *alleventsfd = NULL;

	if (getenv("BBALLHISTLOG")) save_allevents = (strcmp(getenv("BBALLHISTLOG"), "TRUE") == 0);
	if (getenv("BBHOSTHISTLOG")) save_hostevents = (strcmp(getenv("BBHOSTHISTLOG"), "TRUE") == 0);
	if (getenv("SAVESTATUSLOG")) save_histlogs = (strcmp(getenv("SAVESTATUSLOG"), "TRUE") == 0);

	for (argi = 1; (argi < argc); argi++) {
		if (strncmp(argv[argi], "--histdir=", 10) == 0) {
			histdir = strchr(argv[argi], '=')+1;
		}
		else if (strncmp(argv[argi], "--histlogdir=", 10) == 0) {
			histlogdir = strchr(argv[argi], '=')+1;
		}
		else if (strcmp(argv[argi], "--debug") == 0) {
			debug = 1;
		}
	}

	if (getenv("BBHIST") && (histdir == NULL)) {
		histdir = strdup(getenv("BBHIST"));
	}
	if (histdir == NULL) {
		errprintf("No history directory given, aborting\n");
		return 1;
	}

	if (save_histlogs && (histlogdir == NULL) && getenv("BBHISTLOGS")) {
		histlogdir = strdup(getenv("BBHISTLOGS"));
	}
	if (save_histlogs && (histlogdir == NULL)) {
		errprintf("No history-log directory given, aborting\n");
		return 1;
	}

	if (save_allevents) {
		char alleventsfn[MAX_PATH];
		sprintf(alleventsfn, "%s/allevents", histdir);
		alleventsfd = fopen(alleventsfn, "a");
		if (alleventsfd == NULL) {
			errprintf("Cannot open the all-events file '%s'\n", alleventsfn);
		}
		setlinebuf(alleventsfd);
	}

	/* For picking up lost children */
	signal(SIGCHLD, sig_handler);

	while ((msg = get_bbgend_message("bbd_history", &seq, NULL)) != NULL) {
		char *items[20] = { NULL, };
		int icount;
		char *p;
		char *statusdata = "";
		char *hostname, *hostnamecommas, *testname;
		time_t tstamp, lastchg;
		int newcolor, oldcolor;
		struct tm tstamptm;
		char newcol2[3];
		char oldcol2[3];
		int trend;

		p = strchr(msg, '\n'); 
		if (p) {
			*p = '\0'; 
			statusdata = msg_data(p+1);
		}
		p = gettok(msg, "|"); icount = 0;
		while (p && (icount < 20)) {
			items[icount++] = p;
			p = gettok(NULL, "|");
		}

		if (strncmp(items[0], "@@stachg", 8) == 0) {
			/* @@stachg#seq|timestamp|sender|hostname|testname|expiretime|color|prevcolor|changetime */
			sscanf(items[1], "%d.%*d", (int *)&tstamp);
			memcpy(&tstamptm, localtime(&tstamp), sizeof(tstamptm));
			hostname = items[3];
			testname = items[4];
			newcolor = parse_color(items[6]);
			oldcolor = parse_color(items[7]);
			lastchg = atoi(items[8]);

			if (save_histlogs) {
				char *hostdash;
				char fname[MAX_PATH];
				FILE *histlogfd;

				p = hostdash = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = '_';
				sprintf(fname, "%s/%s", histlogdir, hostdash);
				mkdir(fname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
				p = fname + sprintf(fname, "%s/%s/%s", histlogdir, hostdash, testname);
				mkdir(fname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);

				p += strftime(p, sizeof(fname)-(p-fname), "/%a_%b_", &tstamptm);
				p += sprintf(p, "%d", tstamptm.tm_mday);
				p += strftime(p, sizeof(fname)-(p-fname), "_%H:%M:%S_%Y", &tstamptm);
				histlogfd = fopen(fname, "w");
				if (histlogfd) {
					fwrite(statusdata, strlen(statusdata), 1, histlogfd);
					fclose(histlogfd);
				}
				else {
					errprintf("Cannot create histlog file '%s' : %s\n", fname, strerror(errno));
				}
				free(hostdash);
			}

			p = hostnamecommas = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = ',';

			if (save_statusevents) {
				char statuslogfn[MAX_PATH];
				int logexists;
				FILE *statuslogfd;
				char oldcol[100];
				char timestamp[40];
				struct stat st;

				sprintf(statuslogfn, "%s/%s.%s", histdir, hostnamecommas, testname);
				stat(statuslogfn, &st);
				statuslogfd = fopen(statuslogfn, "r+");
				logexists = (statuslogfd != NULL);

				if (logexists) {
					/*
					 * There is a fair chance bbd_net has not been
					 * running all the time while this system was monitored.
					 * So get the time of the latest status change from the file,
					 * instead of relying on the "lastchange" value we get
					 * from bbd_net. This is also needed when migrating from 
					 * standard bbd to bbd_net.
					 */
					long pos = -1;
					char l[1024];
					int gotit;

					fseek(statuslogfd, 0, SEEK_END);
					if (ftell(statuslogfd) > 512) 
						/* Go back 512 from EOF */
						fseek(statuslogfd, -512, SEEK_END);
					else
						/* Read from beginning of file */
						fseek(statuslogfd, 0, SEEK_SET);

					/* Skip to start of a line */
					gotit = (fgets(l, sizeof(l)-1, statuslogfd) == NULL);

					while (!gotit) {
						long tmppos = ftell(statuslogfd);
						time_t dur;

						if (fgets(l, sizeof(l)-1, statuslogfd)) {
							/* Sun Oct 10 06:49:42 2004 red   1097383782 602 */

							if ((strlen(l) > 24) && 
							    (sscanf(l+24, " %s %d %d", oldcol, (int *)&lastchg, (int *)&dur) == 2)) {
								/* 
								 * Record the start location of the line
								 */
								pos = tmppos;
							}
						}
						else {
							gotit = 1;
						}
					}

					if (pos == -1) {
						/* 
						 * Couldnt find anything in the log.
						 * Take lastchg from the timestamp of the logfile,
						 * and just append the data.
						 */
						lastchg = st.st_mtime;
						fseek(statuslogfd, 0, SEEK_END);
					}
					else {
						/*
						 * lastchg was updated above.
						 * Seek to where the last line starts.
						 */
						fseek(statuslogfd, pos, SEEK_SET);
					}
				}
				else {
					/*
					 * Logfile does not exist.
					 */
					lastchg = tstamp;
					statuslogfd = fopen(statuslogfn, "w");
				}

				if (statuslogfd) {
					if (logexists) {
						struct tm oldtm;

						/* Re-print the old record, now with the final duration */
						memcpy(&oldtm, localtime(&lastchg), sizeof(oldtm));
						strftime(timestamp, sizeof(timestamp), "%a %b %e %H:%M:%S %Y", &oldtm);
						fprintf(statuslogfd, "%s %s %d %d\n", 
							timestamp, oldcol, (int)lastchg, (int)(tstamp - lastchg));
					}

					/* And the new record. */
					strftime(timestamp, sizeof(timestamp), "%a %b %e %H:%M:%S %Y", &tstamptm);
					fprintf(statuslogfd, "%s %s %d", timestamp, colorname(newcolor), (int)tstamp);

					fclose(statuslogfd);
				}
				else {
					errprintf("Cannot open status historyfile '%s' : %s\n", 
						statuslogfn, strerror(errno));
				}
			}

			strncpy(oldcol2, ((oldcolor >= 0) ? colorname(oldcolor) : "-"), 2);
			strncpy(newcol2, colorname(newcolor), 2);
			newcol2[2] = oldcol2[2] = '\0';

			if (oldcolor == -1)           trend = -1;	/* we dont know how bad it was */
			else if (newcolor > oldcolor) trend = 2;	/* It's getting worse */
			else if (newcolor < oldcolor) trend = 1;	/* It's getting better */
			else                          trend = 0;	/* Shouldn't happen ... */

			if (save_hostevents) {
				char hostlogfn[MAX_PATH];
				FILE *hostlogfd;

				sprintf(hostlogfn, "%s/%s", histdir, hostname);
				hostlogfd = fopen(hostlogfn, "a");
				if (hostlogfd) {
					fprintf(hostlogfd, "%s %d %d %d %s %s %d\n",
						testname, (int)tstamp, (int)lastchg, (int)(tstamp - lastchg),
						newcol2, oldcol2, trend);
					fclose(hostlogfd);
				}
				else {
					errprintf("Cannot open host logfile '%s' : %s\n", hostlogfn, strerror(errno));
				}
			}

			if (save_allevents) {
				fprintf(alleventsfd, "%s %s %d %d %d %s %s %d\n",
					hostname, testname, (int)tstamp, (int)lastchg, (int)(tstamp - lastchg),
					newcol2, oldcol2, trend);
			}
		}
		else if ((strncmp(items[0], "@@drophost", 10) == 0) && (fork() == 0)) {
			/* @@drophost|timestamp|sender|hostname */

			hostname = items[3];

			if (save_histlogs) {
				char *hostdash;
				char testdir[MAX_PATH];

				/* Remove all directories below the host-specific histlog dir */
				p = hostdash = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = '_';
				sprintf(testdir, "%s/%s", histlogdir, hostdash);
				dropdirectory(testdir);
				free(hostdash);
			}

			if (save_hostevents) {
				char hostlogfn[MAX_PATH];
				struct stat st;

				sprintf(hostlogfn, "%s/%s", histdir, hostname);
				if ((stat(hostlogfn, &st) == 0) && S_ISREG(st.st_mode)) {
					unlink(hostlogfn);
				}
			}

			if (save_statusevents) {
				DIR *dirfd;
				struct dirent *de;
				char *hostlead;
				char statuslogfn[MAX_PATH];
				struct stat st;

				/* Remove bbvar/hist/host,name.* */
				p = hostnamecommas = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = ',';
				hostlead = malloc(strlen(hostname) + 2);
				strcpy(hostlead, hostnamecommas); strcat(hostlead, ".");

				dirfd = opendir(histdir);
				if (dirfd) {
					while ((de = readdir(dirfd)) != NULL) {
						if (strncmp(de->d_name, hostlead, strlen(hostlead)) == 0) {
							sprintf(statuslogfn, "%s/%s", histdir, de->d_name);
							if ((stat(statuslogfn, &st) == 0) && S_ISREG(st.st_mode)) {
								unlink(statuslogfn);
							}
						}
					}
					closedir(dirfd);
				}

				free(hostlead);
				free(hostnamecommas);
			}

			exit(0);	/* Child exits */
		}
		else if ((strncmp(items[0], "@@droptest", 10) == 0) && (fork() == 0)) {
			/* @@droptest|timestamp|sender|hostname|testname */

			hostname = items[3];
			testname = items[4];

			if (save_histlogs) {
				char *hostdash;
				char testdir[MAX_PATH];

				p = hostdash = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = '_';
				sprintf(testdir, "%s/%s/%s", histlogdir, hostdash, testname);
				dropdirectory(testdir);
				free(hostdash);
			}

			if (save_statusevents) {
				char *hostnamecommas;
				char statuslogfn[MAX_PATH];
				struct stat st;

				p = hostnamecommas = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = ',';
				sprintf(statuslogfn, "%s/%s.%s", histdir, hostnamecommas, testname);
				if ((stat(statuslogfn, &st) == 0) && S_ISREG(st.st_mode)) unlink(statuslogfn);
				free(hostnamecommas);
			}

			exit(0);	/* Child exits */
		}
		else if ((strncmp(items[0], "@@renamehost", 12) == 0) && (fork() == 0)) {
			/* @@renamehost|timestamp|sender|hostname|newhostname */
			char *newhostname;

			hostname = items[3];
			newhostname = items[4];

			if (save_histlogs) {
				char *hostdash;
				char *newhostdash;
				char olddir[MAX_PATH];
				char newdir[MAX_PATH];

				p = hostdash = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = '_';
				p = newhostdash = strdup(newhostname); while ((p = strchr(p, '.')) != NULL) *p = '_';
				sprintf(olddir, "%s/%s", histlogdir, hostdash);
				sprintf(newdir, "%s/%s", histlogdir, newhostdash);
				rename(olddir, newdir);
				free(newhostdash);
				free(hostdash);
			}

			if (save_hostevents) {
				char hostlogfn[MAX_PATH];
				char newhostlogfn[MAX_PATH];

				sprintf(hostlogfn, "%s/%s", histdir, hostname);
				sprintf(newhostlogfn, "%s/%s", histdir, newhostname);
				rename(hostlogfn, newhostlogfn);
			}

			if (save_statusevents) {
				DIR *dirfd;
				struct dirent *de;
				char *hostlead;
				char *newhostnamecommas;
				char statuslogfn[MAX_PATH];
				char newlogfn[MAX_PATH];

				p = hostnamecommas = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = ',';
				hostlead = malloc(strlen(hostname) + 2);
				strcpy(hostlead, hostnamecommas); strcat(hostlead, ".");

				p = newhostnamecommas = strdup(newhostname); while ((p = strchr(p, '.')) != NULL) *p = ',';


				dirfd = opendir(histdir);
				if (dirfd) {
					while ((de = readdir(dirfd)) != NULL) {
						if (strncmp(de->d_name, hostlead, strlen(hostlead)) == 0) {
							char *testname = strchr(de->d_name, '.');
							sprintf(statuslogfn, "%s/%s", histdir, de->d_name);
							sprintf(newlogfn, "%s/%s%s", histdir, newhostnamecommas, testname);
							rename(statuslogfn, newlogfn);
						}
					}
					closedir(dirfd);
				}

				free(newhostnamecommas);
				free(hostlead);
				free(hostnamecommas);
			}
		}
		else if (strncmp(items[0], "@@renametest", 12) == 0) {
			/* @@renametest|timestamp|sender|hostname|oldtestname|newtestname */
			char *newtestname;

			hostname = items[3];
			testname = items[4];
			newtestname = items[5];

			if (save_histlogs) {
				char *hostdash;
				char olddir[MAX_PATH];
				char newdir[MAX_PATH];

				p = hostdash = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = '_';
				sprintf(olddir, "%s/%s/%s", histlogdir, hostdash, testname);
				sprintf(newdir, "%s/%s/%s", histlogdir, hostdash, newtestname);
				rename(olddir, newdir);
				free(hostdash);
			}

			if (save_statusevents) {
				char *hostnamecommas;
				char statuslogfn[MAX_PATH];
				char newstatuslogfn[MAX_PATH];

				p = hostnamecommas = strdup(hostname); while ((p = strchr(p, '.')) != NULL) *p = ',';
				sprintf(statuslogfn, "%s/%s.%s", histdir, hostnamecommas, testname);
				sprintf(newstatuslogfn, "%s/%s.%s", histdir, hostnamecommas, newtestname);
				rename(statuslogfn, newstatuslogfn);
				free(hostnamecommas);
			}
		}
	}

	fclose(alleventsfd);
	return 0;
}

