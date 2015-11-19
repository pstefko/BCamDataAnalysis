#!/usr/bin/python

#import fileinput
import sys
import ROOT
from datetime import datetime
from array import array


# read input file from argument and open output rootfile, make two trees, tree with BCam data "treeBCam" and tree with Magnet data "treeMagnet"


outFile = ROOT.TFile("./../ResultTrees/BCamData_RAW.root", "recreate")
treeBCam = ROOT.TTree("treeBCam","Data from all cameras")

def MakeTemperatureTree(temperature_data_file):
    
    inFile = open(temperature_data_file, 'r')
    zeit = ROOT.TDatime()
    print temperature_data_file

    tree = ROOT.TTree("treeTemperature","Temperatures in the IT boxes")
    print tree

    li = ['A','B','C','T']

    times = [[array('i',[0]) for k in li] for e in xrange(3)]	# Array 3x4 (3 detectors, 4 boxes)
    temperatures = [[array('f',[0.]) for k in li] for e in xrange(3)]

    for k in xrange(3):
        for e in range(len(li)):
            tree.Branch('t{0}{1}'.format(k+1,li[e]), times[k][e], 't{0}{1}/I'.format(k+1,li[e]))
            tree.Branch('Temp{0}{1}'.format(k+1,li[e]), temperatures[k][e], 'Temp{0}{1}/F'.format(k+1,li[e]))
	    print str(k) + "   " + str(e)

    tree.Print()


    inFile.readline() 	# skip first line of file
    inFile.readline()

    for line in inFile:
        
        line = line.strip()[1:]
	boxList = line.split(",")

	alternate = map(';'.join, zip(boxList[::2], boxList[1::2]))
	#print alternate

	for e in range(len(alternate)):

	    if alternate[e] == ';':
	        continue

	    t,temperatureValue = alternate[e].split(";",2)
	    day, time = t.split(" ",2)
	    day = datetime.strptime(day, "%d/%m/%Y").strftime("%Y-%m-%d")
	    daytime = day + " " + time
	    zeit.Set(daytime)
	    times[e/4][e%4][0] = zeit.Convert()

	    temperatures[e/4][e%4][0] = float(temperatureValue)
        
	tree.Fill()

    tree.Write()
    inFile.close()

def MakeMagnetTree(magnet_data_file):
    
    inFile = open(magnet_data_file, 'r')
    zeit = ROOT.TDatime()
    print magnet_data_file

    tree = ROOT.TTree("treeMagnet","Current and polarity in magnet")
    print tree

    timez = array('i',[0])
    current = array('f',[0.])
    polarity = array('f',[0.])

    tree.Branch('t' , timez, 't/I')
    tree.Branch('current', current, 'current/F')
    tree.Branch('polarity', polarity, 'polarity/F')

    inFile.readline() 	# skip first line of file

    for line in inFile:
        
        line = line.strip()

	if line.endswith(","):	#if the line ends with comma, ignore (no poalrity info)
	    continue

	t,currentValue,polarityValue = line.split(",",3)
	day, time = t.split(" ",2)
	day = datetime.strptime(day, "%d-%m-%Y").strftime("%Y-%m-%d")
	daytime = day + " " + time
	zeit.Set(daytime)
	timez[0] = zeit.Convert()

	current[0] = float(currentValue)
	
	if (polarityValue == '0'):	# make proper polariy
	    polarityValue = -1.0

	polarity[0] = float(polarityValue)
	
	tree.Fill()
    
    tree.Write()
    inFile.close()


def AddBCamBranches(BCam_data_file):
    inFile = open(BCam_data_file, 'r')

    zeit = ROOT.TDatime()		#time

    print BCam_data_file

    whichCam = inFile.readline()[16:18]	#two numbers taken from first line of input file indicating the camera

    i = array('i',[0])
    timez = array('i',[0])
    xValues = array('f',[0.])
    yValues = array('f',[0.])
    zValues = array('f',[0.])
    xAvg = array('f',[0.])
    yAvg = array('f',[0.])
    zAvg = array('f',[0.])

    treeBCam.Branch('t' + whichCam, timez, 't/I')
    treeBCam.Branch('x' + whichCam, xValues, 'x' + whichCam + '/F')
    treeBCam.Branch('y' + whichCam, yValues, 'y/F')
    treeBCam.Branch('z' + whichCam, zValues, 'z/F')

    global iterator, average
    iterator = 1

    for line in inFile:
        #parse each line 	

        line = line.strip()
        t,x,y,z = line.split(",",4) 
        day, time = t.split(" ",2)

	day = datetime.strptime(day, "%d-%m-%Y").strftime("%Y-%m-%d")
        daytime =  day + " " + time
        
  	zeit.Set(daytime)
        timez[0] = zeit.Convert()  

        xValues[0] = float(x)
        yValues[0] = float(y)
        zValues[0] = float(z)
        i[0] = iterator
        iterator = iterator + 1
        treeBCam.Fill()

    inFile.close()


    treeBCam.SetBranchStatus('t' + whichCam, 0)
    treeBCam.SetBranchStatus('x' + whichCam, 0)
    treeBCam.SetBranchStatus('y' + whichCam, 0)
    treeBCam.SetBranchStatus('z' + whichCam, 0)

    return

def EnableBranches(BCam_data_file):


    inFile = open(BCam_data_file, 'r')
    whichCam = inFile.readline()[16:18]	#two numbers taken from first line of input file indicating the camera
    treeBCam.SetBranchStatus('t' + whichCam, 1)
    treeBCam.SetBranchStatus('x' + whichCam, 1)
    treeBCam.SetBranchStatus('y' + whichCam, 1)
    treeBCam.SetBranchStatus('z' + whichCam, 1)

    return

MakeMagnetTree("./../BCamDataAll/Magnet.csv")
MakeTemperatureTree("./../BCamDataAll/Temperature.csv")

AddBCamBranches("./../BCamDataAll/BCam11.csv")    
AddBCamBranches("./../BCamDataAll/BCam12.csv")    
AddBCamBranches("./../BCamDataAll/BCam21.csv")    
AddBCamBranches("./../BCamDataAll/BCam22.csv")    
AddBCamBranches("./../BCamDataAll/BCam31.csv")    
AddBCamBranches("./../BCamDataAll/BCam32.csv")    

treeBCam.SetEntries(iterator-1) 

EnableBranches("./../BCamDataAll/BCam11.csv")    
EnableBranches("./../BCamDataAll/BCam12.csv")    
EnableBranches("./../BCamDataAll/BCam21.csv")    
EnableBranches("./../BCamDataAll/BCam22.csv")    
EnableBranches("./../BCamDataAll/BCam31.csv")    
EnableBranches("./../BCamDataAll/BCam32.csv")    

treeBCam.Write()
outFile.Close()
