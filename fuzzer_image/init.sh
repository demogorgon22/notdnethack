#!/bin/sh
sysctl -w kernel.core_pattern=/tmp/cores/core-%e.%p.%h.%t
cd /app/dnh/notdnethackdir
/app/dnh/notdnethackdir/notdnethack -D -u `hostname` > /dev/null
sleep 10
CORES_DIR="/cores/notdnethack-$(git rev-parse HEAD)"
mkdir $CORES_DIR
cp /tmp/cores/* $CORES_DIR
if [ -e /cores/dnh/notdnethackdir/notdnethack ]
	then
    	echo "nDNH install exists, checking hash"
    	ourHash=$(md5sum /app/dnh/notdnethackdir/notdnethack | cut -f 1 -d ' ')
    	theirHash=$(md5sum $CORES_DIR/dnh/notdnethackdir/notdnethack | cut -f 1 -d ' ')
    	echo "$ourHash, $theirHash"
    	if [[ "$ourHash" == "$theirHash" ]]
        	then
                echo "nDNH install in date, exiting"
        	else
                echo "nDNH install out of date, updating"
                rm -rf $CORES_DIR/dnh
                cp -r /app/dnh $CORES_DIR/dnh
    	fi
	else
    	echo "nDNH install nonexistent, updating"
    	rm -rf $CORES_DIR/dnh
    	cp -r /app/dnh $CORES_DIR/dnh
fi
