#!/bin/bash

#
# script for uploading html docs to sourceforge
#

USER=''
USAGE="USAGE: `basename $0` <username>"

# read command line arguments
while [ $# -gt 0 ]
do
	case "$1" in
		-*) echo "$USAGE"; exit 0;;
		*)  USER="$1";;
	esac
	shift
done
if [ -z "$USER" ]
then
	echo "$USAGE" 1>&2
	exit 1
fi

rsync -e ssh -arvz --delete html $USER,gamera@web.sourceforge.net:/home/groups/g/ga/gamera/htdocs/doc
