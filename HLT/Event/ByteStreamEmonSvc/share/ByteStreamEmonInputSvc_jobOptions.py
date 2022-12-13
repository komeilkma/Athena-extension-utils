#
# Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
#



from AthenaConfiguration.AllConfigFlags import ConfigFlags
from AthenaConfiguration.ComponentAccumulator import CAtoGlobalWrapper
from ByteStreamEmonSvc.EmonByteStreamConfig import EmonByteStreamCfg

CAtoGlobalWrapper(EmonByteStreamCfg, ConfigFlags)
