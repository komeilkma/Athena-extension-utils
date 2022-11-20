#!/usr/bin/env python

import os
import optparse

import pm.project
import pm.common

from HLTUtils import hltOksUtils
from pm.dal import dal

parser = optparse.OptionParser(description="Create CoralServer S/W respository",
                               usage = '%prog')

parser.add_option("-o", "--output", action="store",
                  default="CoralServer-repository.data.xml",
                  help="Output file name [%default]")

(opt, args) = parser.parse_args()

dbproxy_res = pm.project.Project('daq/sw/common-resources.data.xml').getObject('RM_HW_Resource','DBPROXY')

apps = [
    dal.Binary('coralServer',
               BinaryName='coralServer',
               Description='CORAL Server',
               Authors=['project-lcg-coral-server-developers@cern.ch']
               ),
    
    dal.Binary('coralServerProxy',
               BinaryName='coralServerProxy',
               Description='Proxies HLT node access to configuration database',
               Authors=['salnikov@slac.stanford.edu'],
               Needs=[dbproxy_res]
               )    
    ]
    
swRep = dal.SW_Repository('CoralServerRepo',
                          Name='CoralServerRepo',
                          InstallationPath="${ATLAS_EXTERNAL}/LCGCMT/LCGCMT_%s/InstallArea" % os.environ['LCGCMTVERS'],
                          SW_Objects=apps,
                          Tags=hltOksUtils.defaultTags()
                          )

for app in apps:
    app.BelongsTo = swRep

proj = pm.project.Project(opt.output,
                          ['daq/schema/core.schema.xml',
                           'daq/sw/repository.data.xml',
                           'daq/sw/common-resources.data.xml'
                           ])
    
proj.addObjects([swRep])

    
