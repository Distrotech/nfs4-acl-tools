#!/bin/bash
#
# part of turning the ridiculously awful Qt API format into a Javadoc-ish thing
# so it only takes up 1 browser window.


file="$1"
outdir="$2"
outfile="nosuck-${file}"

if [ -z "$outdir" ]; then
	outdir="./"
elif [ ! -d "$outdir" ]; then
	if ! (mkdir -p "$outdir"); then
		echo "ERROR: unable to create \`$outdir'!"
		exit 1
	fi
fi
sed -e 's/\(tr[.]address.*\)$/body { font-size: 12px ; }\n\1/' $file >${outdir}/${outfile}
