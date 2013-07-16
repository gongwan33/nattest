#!/bin/bash
s=1
while [ "$s" != "0" ]
do
        if [ $(ps -A | grep -c 'IOTC_Server') == "0" ]; then
                /home/administrator/TUTK_SERVER/IOTC_Server_1.0/IOTC_Server_1.0.3.0  --config=config.db C34991R692UZ8G6PWFZT 10001 \
                    m1.iotcplatform.com m2.iotcplatform.com m3.iotcplatform.com \
                    m4.iotcplatform.com m5.iotcplatform.com &
        fi
        sleep 5
done
