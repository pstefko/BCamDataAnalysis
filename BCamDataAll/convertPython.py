#!/usr/bin.python

import ROOT
from datetime import datetime

inFile = open('./Magnet.csv', 'r')
f = open('./MagnetConvert.csv', 'w')

zeit = ROOT.TDatime()

inFile.readline()   # skip first line of file

for line in inFile:

    line = line.strip()

    if line.endswith(","):  #if the line ends with comma, ignore (no poalrity info)
        continue

    t,currentValue,polarityValue = line.split(",",3)
    day, time = t.split(" ",2)
    day = datetime.strptime(day, "%d-%m-%Y").strftime("%Y-%m-%d")
    daytime = day + " " + time
    zeit.Set(daytime)
    timez = zeit.Convert()

    current = float(currentValue)

    f.write(str(timez) + ',' + currentValue + ',' + polarityValue + '\n')
