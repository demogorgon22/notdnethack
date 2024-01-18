#!/bin/sh
sysctl -w kernel.core_pattern=/tmp/cores/core-%e.%p.%h.%t
cd /app/dnh/notdnethackdir
/app/dnh/notdnethackdir/notdnethack -D -u `hostname` > /dev/null
sleep 10
CORES_DIR="/cores/notdnethack-$(git rev-parse HEAD)"
mkdir $CORES_DIR
cp /tmp/cores/* $CORES_DIR
if [ ! -d /cores/dnh ]
	then
    	cp -r /app/dnh $CORES_DIR/dnh
fi
