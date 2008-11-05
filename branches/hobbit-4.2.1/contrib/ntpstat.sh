#!/bin/sh

# This script is an extension for the BB client running on
# your server. It will feed data about the local NTP daemon
# into Hobbit, where the offset between the NTP reference
# clock and the local clock will be graphed.

$BB $BBDISP "data $MACHINE.ntpstat

`ntpq -c \"rv 0 offset\"`
"

exit 0

