#!/bin/sh
#
#----------------------------------------------------------------------------#
# FreeBSD client for Hobbit                                                  #
#                                                                            #
# Copyright (C) 2005-2006 Henrik Storner <henrik@hswn.dk>                    #
#                                                                            #
# This program is released under the GNU General Public License (GPL),       #
# version 2. See the file "COPYING" for details.                             #
#                                                                            #
#----------------------------------------------------------------------------#
#
# $Id: hobbitclient-freebsd.sh,v 1.13 2006-06-01 15:28:31 henrik Exp $

echo "[date]"
date
echo "[uname]"
uname -a
echo "[uptime]"
uptime
echo "[who]"
who
echo "[df]"
# The sed stuff is to make sure lines are not split into two.
df -H -tnonfs,nullfs,cd9660,procfs,devfs,linprocfs | sed -e '/^[^ 	][^ 	]*$/{
N
s/[ 	]*\n[ 	]*/ /
}'
echo "[meminfo]"
$BBHOME/bin/freebsd-meminfo
echo "[swapinfo]"
swapinfo -k
echo "[ifconfig]"
ifconfig -a
echo "[route]"
netstat -rn
echo "[ifstat]"
netstat -i -b -n | egrep -v "^lo|<Link"
echo "[netstat]"
netstat -s
echo "[ports]"
(netstat -na -f inet; netstat -na -f inet6) | grep "^tcp"
echo "[ps]"
ps -ax -ww -o pid,ppid,user,start,state,pri,pcpu,cputime,pmem,rss,vsz,args
echo "[top]"
top -n 20
# vmstat
nohup sh -c "vmstat 300 2 1>$BBTMP/hobbit_vmstat.$MACHINEDOTS.$$ 2>&1; mv $BBTMP/hobbit_vmstat.$MACHINEDOTS.$$ $BBTMP/hobbit_vmstat.$MACHINEDOTS" </dev/null >/dev/null 2>&1 &
sleep 5
if test -f $BBTMP/hobbit_vmstat.$MACHINEDOTS; then echo "[vmstat]"; cat $BBTMP/hobbit_vmstat.$MACHINEDOTS; rm -f $BBTMP/hobbit_vmstat.$MACHINEDOTS; fi
# logfiles
if test -f $LOGFETCHCFG
then
    $BBHOME/bin/logfetch $LOGFETCHCFG $LOGFETCHSTATUS
fi

exit

