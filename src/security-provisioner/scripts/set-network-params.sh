#!/bin/sh

NET_INTERFACE="${1}"
NET_MODE="${2}"
NET_IPADDR="${3}"
NET_GATEWAY="${4}"
NET_DNS="${5}"
NET_MASK="${6}"

uci set network.${NET_INTERFACE}.ipaddr="${NET_IPADDR}"
uci set network.${NET_INTERFACE}.netmask="${NET_MASK}"
uci set network.${NET_INTERFACE}.gateway="${NET_GATEWAY}"
uci set network.${NET_INTERFACE}.dns="${NET_DNS}"
uci commit ${NET_INTERFACE}

reload_config
