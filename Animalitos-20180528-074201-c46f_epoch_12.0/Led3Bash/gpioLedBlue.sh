#!/bin/bash
cd /sys/class/gpio
sudo -kSs << EOF
xxxpassxxx
echo
echo 397 >export
echo out >gpio397/direction
echo 1 >gpio397/value
sleep 1
echo 0 >gpio397/value
echo 481 >unexport
EOF

