#!/bin/bash

if [ "${1}" == "false" ]; then
    /etc/init.d/sshd stop
    /etc/init.d/sshd disable
else
    /etc/init.d/sshd enable
    /etc/init.d/sshd start
fi
