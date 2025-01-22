#!/bin/sh

echo "Set data to be read opcode 0xA3"
i2ctransfer -v -y 4 w6@0x60 0x5c 0x04 0xa3 0x01 0x00 0x80

echo "Check the query status"
i2ctransfer -v -y 4 w1@0x60 0x5c r5

echo "Read back from data register"
i2ctransfer -v -y 4 w1@0x60 0x5d r4
