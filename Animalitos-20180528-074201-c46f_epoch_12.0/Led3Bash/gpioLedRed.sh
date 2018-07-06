#!/bin/bash
cd /sys/class/gpio
sudo -kSs << EOF
xxxpassxxx
echo
echo 481 >export
echo out >gpio481/direction
echo 1 >gpio481/value
sleep 1
echo 0 >gpio481/value
echo 481 >unexport
EOF

