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
# from 'TDAQ_PARTITION' if it this parameter is empty, otherwise from here.
# ############################################################

ByteStreamEmonInputSvc.Partition = "rhauser_hlt"

# #########################################
# The source of events, SFI for full events
# #########################################
ByteStreamEmonInputSvc.Key = "SFI"

# ############################################################
# A list of of key values, e.g. a list of SFIs to contact.
# If not defined, all event providers of this type (e.g. all SFIs)
# ############################################################

# ByteStreamEmonInputSvc.KeyValue = ["SFI-1", "SFI-2" ]

# ###########################################################
# Preferred way: just indicate the number of SFIs you
# want to connect to.
# ###########################################################
ByteStreamEmonInputSvc.KeyCount = 3

# ############################################################
# GroupName: Monitors with the same group name and selection 
# criteria will never receive same events.
# ############################################################
# ByteStreamEmonInputSvc.GroupName = ""

# ############################################################
# Number of internally buffered events. Keep small for
# full events.
# ############################################################
# ByteStreamEmonInputSvc.BufferSize = 2

# #######################################
# Set this to the IS server where you want
# to publish histograms, too. If unset, no
# histograms are published.
# #######################################
#ByteStreamEmonInputSvc.ISServer = 'Histogramming'

# ########################################
# The provider name under which your histograms
#  appear in OH.
# ########################################
#ByteStreamEmonInputSvc.PublishName = 'HelloWorld'

# ###################################################
# Should histograms be cleared at new run ? default: yes
# ###################################################
# ByteStreamEmonInputSvc.ClearHistograms = True

# ####################################################
# A regular expression to restrict which histograms are published.
# ####################################################
# ByteStreamEmonInputSvc.Include = '.*'
# ByteStreamEmonInputSvc.Exclude = ''

# ###############################################
# Frequency of updates (in number of events, not secs...)
# ###############################################
# ByteStreamEmonInputSvc.Frequency = 5

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
# This can be used instead of or in addition to LVL1Bits.
# ###########################################################
#ByteStreamEmonInputSvc.LVL1Names = ['L1_MU2', 'L1_MU3', 'L1_EM10' ]

# #################################################
# Shall athena exit if the partition is shutdown ?
# For offline athena tasks mainly.
# #################################################
#ByteStreamEmonInputSvc.ExitOnPartitionShutdown = False

ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
theApp.ExtSvc += [ "ByteStreamCnvSvc"]

# It looks as if one needs this now online ?
if False:
    from AthenaServices.AthenaServicesConf import MetaDataSvc
    svcMgr += MetaDataSvc( "MetaDataSvc" )
    # Add in MetaData Stores
    from StoreGate.StoreGateConf import StoreGateSvc
    svcMgr += StoreGateSvc( "MetaDataStore" )
    svcMgr += StoreGateSvc( "InputMetaDataStore" )

    # Make MetaDataSvc an AddressProvider
    svcMgr.ProxyProviderSvc.ProviderNames += [ "MetaDataSvc" ]

    # enable IOVDbSvc to read metadata
    #svcMgr.MetaDataSvc.MetaDataContainer = "MetaDataHdr"
    #svcMgr.MetaDataSvc.MetaDataTools += [ "IOVDbMetaDataTool" ]

    MetaDataStore=svcMgr.MetaDataStore

#--------------------------------------------------------------
# Private Application Configuration options, replace with
# your configuration.
#--------------------------------------------------------------
from AthExHelloWorld.AthExHelloWorldConf import HelloAlg

HelloWorld = HelloAlg("HelloWorld")

from AthenaCommon.AlgSequence import AlgSequence
topSequence = AlgSequence()
topSequence += HelloWorld

#--------------------------------------------------------------
# Algorithms Private Options
#--------------------------------------------------------------

# For the genuine HelloWorld algorithm
HelloWorld.MyInt = 42
HelloWorld.MyBool = TRUE
HelloWorld.MyDouble = 3.14159
HelloWorld.MyStringVec = [ "Welcome", "to", "Athena", "Framework", "Tutorial" ]
