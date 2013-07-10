#!/bin/sh
sudo iptables -F -t nat
sudo iptables -F
echo "Please choose a nat mode:[1~4]"
echo "[1]Full Cone NAT"
echo "[2]Port Restricted Cone NAT"
echo "[3]Restricted Cone NAT"

read -p "Input your choice:" c1

case $c1 in
"1")
  sudo iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 192.168.1.110 
  sudo iptables -t nat -A PREROUTING -i eth0 -j DNAT --to-destination 192.10.1.1
  echo "Full Cone NAT setup completed!"
  ;;
"2")
  sudo iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 192.168.1.110
  echo "Port Restricted Cone NAT setup completed!"
  ;;
"3")
  sudo iptables -t nat -A POSTROUTING -o eth0 -j SNAT --to-source 192.168.1.110
  sudo iptables -t nat -A PREROUTING -i eth0 -j DNAT --to-destination 192.10.1.1 
  sudo iptables -A FORWARD -d 192.168.1.110 -j ACCEPT
  sudo iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT
  sudo iptables -I FORWARD 1 -s 192.10.1.1 -j ACCEPT
  echo "Restricted Cone NAT setup completed!"
  ;;
*)
  echo "Please input a correct number.";;
esac
