/*
   Copyright (C) 2022 Komeilkma/leandrogardi for the benefit of the ATLAS collaboration
*/

/** @file
 * @author Sebastian Boeser <sboeser --at-- hep.ucl.ac.uk>
 * Standalone JiveXML server
 * This application provides a standalone ONCRPC (SunRPC) server that receives
 * events (in the JiveXML format) from an (athena) application and servers
 * these same events to the AtlantisJava client.
 * The actual serving thread is part of the JiveXML package, which also provides a
 * serving thread controlled by Athena (JiveXML/ONCRCPServerSvc).
 * This package is intended to be used only in the Atlas Online environment.
 */

#include <JiveXMLServer/JiveXMLServer.h>

//tdaq includes
#include <cmdl/cmdargs.h>
#include <ipc/core.h>
#include <ers/ers.h>


/**
 * Main routine
 */
int main(int argc, char ** argv)
{

  // Declare arguments
  CmdArgStr partition_name('p', "partition", "partition-name", "The name of the partition in which to run the server");
  
  // Initialise IPC
  try {
     IPCCore::init(argc,argv);
  } catch (daq::ipc::Exception & ex) {
     ers::fatal( ex );
  }

  // Declare command object and its argument-iterator
  CmdLine cmd(*argv, &partition_name, NULL);
  cmd.description("Standalone JiveXML server using ONCRPC (SunRPC)");
  CmdArgvIter arg_iter(--argc, ++argv);

  // Parse arguments
  cmd.parse(arg_iter);

  //Say hello
  ERS_INFO("Starting JiveXML server in partition: "<< partition_name );
  
  //Create the object and start the server
  JiveXML::JiveXMLServer server;
  
  //Now wait for the server to finish
  server.Wait();

  //Leaving the scope will call ~JiveXMLServer and destroy the server
  //so no explicit shutdown needed

  return 0;
}

