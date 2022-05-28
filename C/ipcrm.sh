#!/bin/bash

for i in ipcs -s | grep $USER | awk '{print $2}'; do
  ipcrm -s $i
done

for i in ipcs -q | grep $USER | awk '{print $2}'; do
  ipcrm -q $i
done 

for i in ipcs -m | grep $USER | awk '{print $2}'; do
  ipcrm -m $i
done

GREEN='\033[0;32m'      # Green (0;32)
NC='\033[0m'         # No Color
echo -e "[$GREEN OK $NC] All IPC objects removed correctly"