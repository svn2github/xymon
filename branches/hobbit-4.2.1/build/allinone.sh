#!/bin/sh

cd ~/hobbit
make -f Makefile.home clean
./build/dorelease.sh daily
cd beta/snapshot
diff -urN -x version.h -x changelog -x allinone.sh -x c-ares -x changelog-lastweek.txt -x changelog-yesterday.txt ../../release/betarelease/hobbit-4.2-RC-20060712/ ./  > ../../release/betapatches/allinone.patch

cd ~/hobbit
TSTAMP=`TZ=UTC date +"%Y-%m-%d %H:%M %Z"`
echo 's!^Last updated:.*$!Last updated: @TSTAMP@!' | sed -e "s/@TSTAMP@/$TSTAMP/" >build/allinone.sed
cat release/betapatches/index.html | sed -f build/allinone.sed >release/betapatches/index.html.new
mv release/betapatches/index.html release/betapatches/index.html.old
mv release/betapatches/index.html.new release/betapatches/index.html

