#!/bin/sh

DATESTR="$1"
if [ "$DATESTR" = "" ]
then
	echo "Usage: $0 DATE"
	exit 1
fi

SRCDIRS="bbdisplay bbnet bbproxy build client common contrib docs hobbitd web include lib debian rpm demotool"

cd ~/hobbit
for f in $SRCDIRS; do find $f/  -type f | egrep -v "RCS|/pkg/|~\$|c-ares|docs/manpages"; done | while read FN
do
	./build/revlog $DATESTR $FN
done

exit 0

