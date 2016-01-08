#!/usr/bin/python

#import fileinput
import sys
import ROOT
from datetime import datetime
from array import array


# read input file from argument and open output rootfile, make two trees, tree with BCam data "treeBCam" and tree with Magnet data "treeMagnet" and also with temperatures in IT boxes "treeTemperature"


outFile = ROOT.TFile("./../ResultTrees/OriginalData/BCamData_RAW.root", "recreate")
treeBCam = ROOT.TTree("treeBCam","Data from all cameras")

def MakeTemperatureTree(temperature_data_file, whichStation, whichBox):		# Take temperature file and name of box and make tree with temperatyures
    
    inFile = open(temperature_data_file, 'r')
    zeit = ROOT.TDatime()
    print temperature_data_file

    tree = ROOT.TTree("treeTemperature{0}{1}".format(whichStation,whichBox),"Temperature in the T{0} box {1}".format(whichStation,whichBox))
    print tree

    timez = array('i',[0])
    temperature = array('f',[0.])
        
    tree.Branch('t', timez, 't/I')
    tree.Branch('temperature', temperature, 'temperature/F')

    inFile.readline() 	# skip first two lines of file
    inFile.readline()

    dict = {'1A': 1,'1B': 2,'1C': 3,'1T': 4,'2A': 5,'2B': 6,'2C': 7,'2T': 8,'3A': 9,'3B': 10,'3C': 11,'3T': 12}
    boxNumber = dict[whichStation + whichBox] - 1

    for line in inFile:
        
        line = line.strip()[1:]
	boxList = line.split(",")

	alternate = map(';'.join, zip(boxList[::2], boxList[1::2]))

	if alternate[boxNumber] == ';':	# If the information from some box is missing, skip this box': 1,
	    continue

	t,temperatureValue = alternate[boxNumber].split(";",2)

	if ((float(temperatureValue) > 20.0) or (float(temperatureValue) < 1.0)):	# If the temperature is nonsense, get rid of it
	    continue

	day, time = t.split(" ",2)
	day = datetime.strptime(day, "%d/%m/%Y").strftime("%Y-%m-%d")
	daytime = day + " " + time
	zeit.Set(daytime)
	
	timez[0] = zeit.Convert()
	temperature[0] = float(temperatureValue)

	tree.Fill()

    tree.Print()
    tree.Write()
    inFile.close()

def MakeHybridVoltageTree(hybrid_voltage_data_file):
    
    inFile = open(hybrid_voltage_data_file, 'r')
    zeit = ROOT.TDatime()
    print hybrid_voltage_data_file

    tree = ROOT.TTree("treeHybridVoltage","Voltage in Hybrids")
    print tree

    timez = array('i',[0])
    hybridVoltage = array('f',[0.])
    prevVoltage = 0.0

    tree.Branch('t' , timez, 't/I')
    tree.Branch('hybridVoltage', hybridVoltage, 'hybridVoltage/F')

    inFile.readline() 	# skip first two lines of file
    inFile.readline() 	# skip first two lines of file

    for line in inFile:
        
        line = line.strip()[1:]
	print line

	t,voltageValue = line.split(",",2)

	day, time = t.split(" ",2)
	day = datetime.strptime(day, "%d/%m/%Y").strftime("%Y-%m-%d")
	daytime = day + " " + time
	zeit.Set(daytime)

	if float(voltageValue) != prevVoltage:		# Everytime the voltage changes filll the value of previous voltage 1 second before
	    timez[0] = zeit.Convert() - 1
	    hybridVoltage[0] = prevVoltage
	    tree.Fill()


	timez[0] = zeit.Convert()
	hybridVoltage[0] = float(voltageValue)
	
	prevVoltage = float(voltageValue)

	tree.Fill()
    
    tree.Print()
    tree.Write()
    inFile.close()


def MakeVoltageTree(voltage_data_file):
    
    inFile = open(voltage_data_file, 'r')
    zeit = ROOT.TDatime()
    print voltage_data_file

    tree = ROOT.TTree("treeVoltage","Voltage in detector")
    print tree

    timez = array('i',[0])
    voltage = array('f',[0.])

    tree.Branch('t' , timez, 't/I')
    tree.Branch('voltage', voltage, 'voltage/F')

    for line in inFile:
        
        line = line.strip()

	t,voltageValue = line.split(",",2)
	day, time = t.split(" ",2)
	day = datetime.strptime(day, "%d/%m/%Y").strftime("%Y-%m-%d")
	daytime = day + " " + time
	zeit.Set(daytime)
	timez[0] = zeit.Convert()

	voltage[0] = float(voltageValue)
	
	tree.Fill()
    
    tree.Print()
    tree.Write()
    inFile.close()

def MakeMagnetTemperaturesTree(magnettemperature_data_file):
    
    inFile = open(magnettemperature_data_file, 'r')
    zeit = ROOT.TDatime()
    print magnettemperature_data_file

    tree = ROOT.TTree("treeMagnetTemperature","Temperatures in B sensors in magnet")
    print tree

    timez = array('i',[0])
    temp0 = array('f',[0.])
    temp1 = array('f',[0.])
    temp2 = array('f',[0.])
    temp3 = array('f',[0.])

    tree.Branch('t' , timez, 't/I')
    tree.Branch('temp0', temp0, 'temp0/F')
    tree.Branch('temp1', temp1, 'temp1/F')
    tree.Branch('temp2', temp2, 'temp2/F')
    tree.Branch('temp3', temp3, 'temp3/F')

    inFile.readline() 	# skip first line of file

    for line in inFile:
        
        line = line.strip()

	t,temp0value,temp1value,temp2value,temp3value = line.split(",",5)

	if temp0value=='' or temp1value=='' or temp2value=='' or temp3value=='':
	  continue

	day, time = t.split(" ",2)
	day = datetime.strptime(day, "%d-%m-%Y").strftime("%Y-%m-%d")
	daytime = day + " " + time
	zeit.Set(daytime)
	timez[0] = zeit.Convert()

	try:
	  temp0[0] = float(temp0value)
	except ValueError:
	  print "blaaaa"
	  print line

	temp0[0] = float(temp0value)
	temp1[0] = float(temp1value)
	temp2[0] = float(temp2value)
	temp3[0] = float(temp3value)
	
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
MakeMagnetTemperaturesTree("./../BCamDataAll/MagnetTemperatures.csv")

for e in ['1','2','3']: 
  for i in ['A','B','C','T']:
    MakeTemperatureTree("./../BCamDataAll/BoxTemperatures.csv",e,i)

MakeVoltageTree("./../BCamDataAll/Voltage.csv")
MakeHybridVoltageTree("./../BCamDataAll/HybridVoltage.csv")

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
