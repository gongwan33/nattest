#!/bin/bash
if [ -e index.html ]; then
        rm index.html
fi
wget -O index.html checkip.dyndns.com -q
OldIP=`cat index.html | sed 's/^.*Address: //g' | sed 's/<.*$//g'`
rm index.html

s=1
while [ $s != 0 ]; do
	wget -O index.html checkip.dyndns.com -q
	IP=`cat index.html | sed 's/^.*Address: //g' | sed 's/<.*$//g'`
	rm index.html
	if [[ $OldIP != $IP ]]
	then 
		echo "IP Changed!! so kill IOTC_Server"
		killall IOTC_Server
		OldIP=$IP
	fi
	sleep 60
done
