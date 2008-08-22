#!/bin/bash
#
# part of turning the ridiculously awful Qt API format into a Javadoc-ish thing
# so it only takes up 1 browser window.

file="$1"
all="`cat $file`"
all="`echo "$all" | grep href | sed 's/a href/ahref/'`"
allnames=""
i=`cat $file | grep href | wc -l`

for line in $all; do
		var=`echo "$line" | sed 's/^.*href="\([^.]*\)[.]html".*$/\1/'`
		eval "${var}val=\"$line\""
		#echo "LINE: $line"
		#echo "VAR:  $var"
		#eval "echo AND?: $var is \`eval echo '$`echo ${var}`val'\`"
		allnames="$allnames $var"

		i=$(($i - 1))
		if [ "$i" -lt 1 ]; then
				allnames=`echo "$allnames" | tr ' ' '\n' | sort`
				#echo "ALLNAMES: `echo "$allnames" `"
				echo
			
				
				for name in $allnames; do
					#eval "echo $name  --  \`echo $`echo ${name}`val\`"
					fixedline=`eval echo $\`echo ${name}\`val`
					fixedline=`echo $fixedline | sed -e 's/ahref=/a href="nosuck-/g' -e 's,\(href[^>]*\),\1" target="classFrame",g' -e 's,[<]td[>],,g' -e 's,[<]/td[>],<br>,g'`
					echo "$fixedline"
				done
				exit 0
		fi
done
