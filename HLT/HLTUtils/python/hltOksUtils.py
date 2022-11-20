# Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration

"""
Utilities for HLT OKS file generation
"""

import os

def defaultTags():
   """Return list of supported binary tags"""
   
   import pm.common

   tag = platform()
   opt_tag = '-'.join(tag.split('-')[:-1])+'-opt'
   dbg_tag = '-'.join(tag.split('-')[:-1])+'-dbg'
   tags = [pm.common.tdaqRepository.getObject('Tag', opt_tag),
           pm.common.tdaqRepository.getObject('Tag', dbg_tag)]

   return tags

def platform():
   """Return current BINARY_TAG"""

   project = os.environ.get('AtlasProject','')
   tag = os.environ.get('%s_PLATFORM' % project, os.environ.get('BINARY_TAG',None))

   return tag


