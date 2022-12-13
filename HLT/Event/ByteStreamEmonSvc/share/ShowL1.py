#########################################
#
# Example setup for ByteStreamEmonSvc
# 
#########################################

# Common part, copy from here

from AthenaCommon.AppMgr import ServiceMgr as svcMgr
from AthenaCommon.Constants import *


include("ByteStreamEmonSvc/ByteStreamEmonInputSvc_jobOptions.py")

# #####################################################
# If using the ERSBootstrap.py file, enable the output
# via ERS
# ####################################################
#MessageSvc.useErs = True

# Define the input
ByteStreamEmonInputSvc = svcMgr.ByteStreamInputSvc

# ############################################################
# The name of the partition you want to connect to is taken
# from 'TDAQ_PARTITION' if it exists, otherwise from here.
# ############################################################

ByteStreamEmonInputSvc.Partition = "rhauser_hlt"

# #########################################
# The source of events, SFI for full events
# #########################################
ByteStreamEmonInputSvc.Key = "SFI"

# ############################################################
# A list of of key values, e.g. a list of SFIs to contact.
# If not defined, one event providers of this type.
# ############################################################

ByteStreamEmonInputSvc.KeyCount = 2

# ##############################
# one of 'Ignore', 'Or', 'And'
# ##############################
ByteStreamEmonInputSvc.LVL1Logic = "Ignore"

# ###########################################################
# A LVL1 bit pattern to select events on.
#
# From 1 to 4 64bit values, first entry = Bit 0..63 in CTP
# ###########################################################
#ByteStreamEmonInputSvc.LVL1Bits = [ 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff]

# ###########################################################
# A list of L1 bit names to select on. This requires the
# L1CT.TrigConfL1Items to be published in IS.
# 
# This can be used instead of or in addition to LVL1Bits
# and LVL1Items.
# ###########################################################
#ByteStreamEmonInputSvc.LVL1Names = ['L1_MU2', 'L1_MU3', 'L1_EM10' ]

# ###########################################################
# A list of numerical L1 items to select on. 
# 
# This can be used instead of or in addition to LVL1Bits and
# LVL1Names
# ###########################################################
#ByteStreamEmonInputSvc.LVL1Items = [ 1, 20, 45 ] + range(22,28)

# #################################################
# Shall athena exit if the partition is shutdown ?
# For offline athena tasks mainly.
# #################################################
#ByteStreamEmonInputSvc.ExitOnPartitionShutdown = False

ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
theApp.ExtSvc += [ "ByteStreamCnvSvc"]

# -----------------------------------------------------
# An example algorithm in Python
# -----------------------------------------------------
import AthenaPython.PyAthena as PyAthena

from GaudiPython.Bindings import gbl,InterfaceCast

class ShowLVL1(PyAthena.Alg):
    """
    An athena algorithm written in python.
    """

    def __init__(self, name="ShowLVL1", **kw):
        kw['name'] = name
        super(ShowLVL1,self).__init__(name=name)
        # How to access properties
        self.SomeProperty = kw.get('SomeProperty', 'DefaultValue')

    def initialize(self):
        self.msg.info("initializing [%s]", self.name())

        self.sg = PyAthena.py_svc("StoreGateSvc")
        if not self.sg:
            self.msg.error("Could not get StoreGateSvc")
            return PyAthena.StatusCode.Failure

        return PyAthena.StatusCode.Success

    def execute(self):
        self.msg.debug("executing [%s]", self.name())

        # Get EventInfo
        ev = self.sg.retrieve('EventInfo')

        # Print some LVL1 information
        print "Run:",ev.event_ID().run_number(),"Event:",ev.event_ID().event_number()
        for x in  ev.trigger_info().level1TriggerInfo():
            print "0x%08x" % x

        return PyAthena.StatusCode.Success        

    def finalize(self):
        self.msg.debug("finalizing [%s]", self.name())
        return PyAthena.StatusCode.Success        

ShowLVL1Algo = ShowLVL1("ShowMyLVL1")

#--------------------------------------------------------------
# Private Application Configuration options, replace with
# your configuration.
#--------------------------------------------------------------
from AthenaCommon.AlgSequence import AlgSequence
topSequence = AlgSequence()
topSequence += ShowLVL1Algo

#--------------------------------------------------------------
# Algorithms Private Options
#--------------------------------------------------------------
ShowLVL1Algo.SomeProperty = 'Some Value'
