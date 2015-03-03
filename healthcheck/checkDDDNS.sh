#!/bin/bash

##
# @file
# DDDNS health check wrapper for CLI (bash, crontab) use.
#
# Note that this script requires you to have set up curl properly.
# Also you must provide the URL of your DDDNS web frontend, including any auth
# credentials, e.g. "https://user:pass@dddns.example.com", below.
#
# You may also want to consider the included PHP based script
# which saves you the HTTP detour.
#
# Although a matter of course, remember that this script will not make
# any sense unless it runs outside the dynamic IP LAN you want to
# monitor (but ideally on your DDDNS or any other independent server).
#
#
# @param string host
#   The host to check, e.g. "dyn.example.com". (Required.)
#
# @param string mailto
#   One or more mail recipients to be informed. Separate multiple
#   recipients by semicolons. (Required.)
#
# @param int threshold
#   A positive integer defining the lookback limit
#   in seconds. ("Check whether an update was performed
#   within the last n seconds.") (Required.)
#
# @param string disable
#   If set to "true", delete all records for the given host.

##
# Set this value to match your DDDNS web frontend.
#
# DO NOT add a trailing slash!
#url="https://user:pass@dddns.example.com"
url="http://dddns.udev"


if [ "$1" == "" ]; then
	echo "Error: No host name given."
	exit 1;
fi
host="$1"

if [ "$2" == "" ]; then
	echo "Error: No mail recipient given."
	exit 2;
fi
mailto="$2"

if [ "$3" == "" ]; then
	echo "Error: No threshold given."
	exit 3;
fi
threshold="$3"

if [ "$4" == "true" ]; then
	cleanup="true";
fi

# Issues a curl request with the given parameters and checks the HTTP result
# code (which is in the first response line).
# If the latter is different from "200" ("OK"), the script terminates with an
# error status for optional further integration.
curl -I "${url}/?show=check&host=${host}&threshold=${threshold}&mailto=${mailto}&cleanup=${cleanup}" | head -1 | egrep '\b200\b' > /dev/null 2>&1 || exit 1
