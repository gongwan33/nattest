#!/bin/sh
echo "Please choose a nat mode:[1~4]"
echo "[1]Full Cone NAT"
echo "[2]Port Restricted Cone NAT"
echo "[3]Restricted Cone NAT"

read -p "Input your choice:" c1

case $c1 in
"1")
  iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 202.96.209.111
  iptables -t nat -A PREROUTING -i eth0 -j DNAT --to-destination 192.168.1.1
  echo "Full Cone NAT setup completed!"
  ;;
"2")
  iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 202.96.209.111
  echo "Port Restricted Cone NAT setup completed!"
  ;;
"3")
  iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 202.96.209.111
  iptables -t nat -A PREROUTING -i eth0 -j DNAT --to-destination 192.168.1.1 
  iptables -A FORWARD -d 202.96.209.111 -j ACCEPT
  iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT
  iptables -I FORWARD 1 -s 85.214.128.86 -j ACCEPT
  echo "Restricted Cone NAT setup completed!"
  ;;
*)
  echo "Please input a correct number.";;
esac
