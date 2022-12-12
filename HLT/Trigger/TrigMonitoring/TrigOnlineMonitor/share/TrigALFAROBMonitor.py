# Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
#**************************************************************
#
# TrigAlfaROBMonitor.py
#
#==============================================================
from AthenaCommon.Constants import *
from AthenaCommon import CfgMgr
ALFAROBMonitor = CfgMgr.TrigALFAROBMonitor("ALFAROBMonitor")

#--------------------------------------------------------------
# Add the algorithm to the topSequence
# (in L2 it should be running after steering, so that the ROB
#  cache is already filled)
#--------------------------------------------------------------
from AthenaCommon.AlgSequence import AlgSequence 
topSequence = AlgSequence()
from TrigOnlineMonitor.TrigOnlineMonitorConfig import TrigALFAROBMonitor
topSequence += TrigALFAROBMonitor()

#monTool1 = GenericMonitoringTool('MonTool1')
#monTool1.HistPath="ALFAROBMonitor/python_kk_background"
#monTool = GenericMonitoringTool('MonTool')
#monTool.HistPath="ALFAROBMonitor/python_kk"
#ALFAROBMonitor.MonTool += [monTool, monTool1]
#ALFAROBMonitor.MonTool = monTool

#topSequence.ALFAROBMonitor.MonTool = [monTool, monTool1]
#--------------------------------------------------------------
# Private Application Configuration options
#--------------------------------------------------------------
#
# --- do ROB checksum test
#
#ALFAROBMonitor.TestROBChecksum            = True
#
# --- Histogram definitions for ROB checksum test
#
#MuCTPiROBMonitor.HistFailedChecksumForROB   = ('FailedChecksumForROB', 0, 50, 50)
#MuCTPiROBMonitor.HistFailedChecksumForSD    = ('FailedChecksumForSubDetector', 0, 60, 60)
#
# --- plot generic/specific status bits for ROB fragments
#
#MuCTPiROBMonitor.TestROBStatus              = True
#
# ---- ROB Id for CTP/muCTPi ROB from L1 in Level-2
#
#MuCTPiROBMonitor.Lvl1CTPROBid               = 0x770001
#MuCTPiROBMonitor.Lvl1MuCTPiROBid            = 0x760001
#
# ---- ROB Id for CTP/muCTPi ROB from L1 in DAQ
#
#MuCTPiROBMonitor.DaqCTPROBid                = 0x770000
#MuCTPiROBMonitor.DaqMuCTPiROBid             = 0x760000
#
# --- Number of Sectors/Units per side for Barrel/Endcap/Forward
#
#MuCTPiROBMonitor.NumberOfBarrelUnits        = 32
#MuCTPiROBMonitor.NumberOfEndcapUnits        = 48
#MuCTPiROBMonitor.NumberOfForwardUnits       = 24
#
# --- Histogram definitions for muCTPi monitor
#
#MuCTPiROBMonitor.HistNumberOfRoIs            = ('Number_of_RoIs', 0, 50, 50)
#MuCTPiROBMonitor.HistSectorID_Barrel_muCTPi  = ('muCTPi_SectorID_Barrel',  0, 32, 32)
#MuCTPiROBMonitor.HistSectorID_Endcap_muCTPi  = ('muCTPi_SectorID_Endcap',  0, 48, 48)
#MuCTPiROBMonitor.HistSectorID_Forward_muCTPi = ('muCTPi_SectorID_Forward', 0, 24, 24)
#MuCTPiROBMonitor.HistDifferenceRoIs_RoIB_DAQ_muCTPi = ('muCTPi_differenceRoIs_RoIB_DAQ', -5, 5, 11)
#MuCTPiROBMonitor.HistBarrelHashforProblemRoIs  = ('muCTPi_Barrel_Hash_for_RoIs_only_in' ,0, 100, 100)
#MuCTPiROBMonitor.HistEndcapHashforProblemRoIs  = ('muCTPi_Endcap_Hash_for_RoIs_only_in' ,0, 100, 100)
#MuCTPiROBMonitor.HistForwardHashforProblemRoIs = ('muCTPi_Forward_Hash_for_RoIs_only_in',0, 100, 100)
#
# --- do timing for muCTPi monitoring algorithm
#
#MuCTPiROBMonitor.DoTiming                   = True
#MuCTPiROBMonitor.HistTimeMuCTPiMonitor      = ('Time_MuCTPi_Monitor' ,0.,5.,50)
#==============================================================
#
# End of TrigAlfaMonitor.py
#
#**************************************************************
