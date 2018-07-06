#!/bin/bash
cd /sys/class/gpio
sudo -kSs << EOF
xxxpassxxx
echo
echo 255 >export
echo out >gpio255/direction
echo 1 >gpio255/value
sleep 1
echo 0 >gpio255/value
echo 481 >unexport
EOF

