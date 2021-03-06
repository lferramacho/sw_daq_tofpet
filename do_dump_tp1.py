# -*- coding: utf-8 -*-
import atb
from loadLocalConfig import loadLocalConfig
from bitarray import bitarray
from sys import argv, stdout, maxint
from time import time, sleep
import ROOT
from rootdata import DataFile
import DSHM

import tofpet
import argparse


parser = argparse.ArgumentParser(description='Scan vbl and ib1 ASIC parameters while acquiring fetp data')
parser.add_argument('acqTime', type=float,
                   help='acquisition time for each channel (in seconds)')

parser.add_argument('hvBias', type=float,
                   help='The voltage to be set for the HV DACs')

parser.add_argument('tpDAC', type=int,
                   help='The amplitude of the test pulse, in DAC units (63 is  minimum and 0 is maximum)')

parser.add_argument('OutputFilePrefix',
                   help='output file prefix (files with .raw2 and .idx suffixes will be created)')


parser.add_argument('--asics', nargs='*', type=int, help='If set, only the selected asics will acquire data')

parser.add_argument('--channel_start', type=int, default=0, help='If set, the channel number ID (form 0 to 63) for which to start the scan (default = 0)')

parser.add_argument('--channel_step', type=int, default=6, help='If set, the scan will be performed for one in every channel_step (default = 6). If only interested in one channel per ASIC, set this to 64.')

parser.add_argument('--comments', type=str, default="", help='Any comments regarding the acquisition. These will be saved as a header in OutputFilePrefix.params')

args = parser.parse_args()


acquisitionTime = args.acqTime
dataFilePrefix = args.OutputFilePrefix


# ASIC clock period
T = 6.25E-9

# Preliminary coincidence window
# Coincidence window to use be used for preliminary event selection
# 0 causes all events to be accepted
cWindow = 0
#cWindow = 25E-9

tpDAC = args.tpDAC
tpFrameInterval = 16
tpCoarsePhase = 0
tpFinePhase = 1
tpLength = 512
tpInvert=True

uut = atb.ATB("/tmp/d.sock", False, F=1/T)

baseConfig = loadLocalConfig(useBaseline=False)

uut.config = baseConfig
uut.initialize()
uut.openAcquisition(dataFilePrefix, cWindow, writer="writeRaw")
uut.config.writeParams(dataFilePrefix, args.comments)


if args.asics == None:
	activeAsics =  uut.getActiveTOFPETAsics()
else:
	activeAsics= args.asics

print activeAsics

for tChannel in range(args.channel_start,64,args.channel_step):
	atbConfig=loadLocalConfig(useBaseline=False)

	for asic in activeAsics:
		atbConfig.asicConfig[asic].globalConfig.setValue("test_pulse_en", 1)
		atbConfig.asicConfig[asic].channelTConfig[tChannel] = bitarray('1')
		atbConfig.asicConfig[asic].globalTConfig = bitarray(atb.intToBin(tpDAC, 6) + '1')
	
	uut.config=atbConfig
	uut.uploadConfig()
	uut.setTestPulsePLL(tpLength, tpFrameInterval, tpFinePhase, tpInvert)
	uut.setAllHVDAC(args.hvBias)
	uut.doSync()
	for step1 in range(0,64,4): # vib
		for step2 in range(0,64,4): #vbl
			t0 = time()
			lastWriteTime = t0


		
			for ac in atbConfig.asicConfig:
				if ac==None:
					continue
				ac.globalConfig.setValue("vib1", step1);
				for cc in ac.channelConfig:
					cc.setValue("vbl", step2);
			uut.config=atbConfig
			uut.uploadConfig()
			uut.doSync()
			print "Acquiring for channel %d, step %d %d..." % (tChannel,step1, step2)
			uut.acquire(step1, step2, acquisitionTime)

  

uut.setAllHVDAC(5.0)
