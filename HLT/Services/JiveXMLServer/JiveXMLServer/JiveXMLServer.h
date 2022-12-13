/*
  Copyright (C) 2022 Komeilkma/leandrogardi for the benefit of the ATLAS collaboration
*/

#ifndef JIVEXML_JIVEXMLSERVER_H
#define JIVEXML_JIVEXMLSERVER_H

/**
 * @class JiveXMLServer
 * @brief Controls and handles the JiveXML server threads
 * This class creates, handles and controls the JiveXML server threads both for
 * incoming and outgoing data. It also provides an interface for the threads to
 * the TDAQ services, such as error logging.
 *
 * @author Sebastian Boeser <sboeser --at-- hep.ucl.ac.uk>
 **/

#include <vector>
#include <string>

//athena includes
#include <GaudiKernel/IMessageSvc.h>
#include <GaudiKernel/StatusCode.h>
#include <CxxUtils/checker_macros.h>

//JiveXML includes
#include <JiveXML/IServer.h>
#include <JiveXML/EventStream.h>

//tdaq includes
#include <owl/semaphore.h>

namespace JiveXML {

  //Forward declarations
  class ThreadCollection;

  class JiveXMLServer: virtual public IServer {

    public:
      //Constructor with port number
      JiveXMLServer( int port = 0 );

      //Destructor
      virtual ~JiveXMLServer();

      /**@name IEventReceiver methods */
      //@{
      /** Put this event as new current event for stream given by name */
      virtual StatusCode UpdateEventForStream( const EventStreamID& evtStreamID, const std::string & event) override;
      //@}

      /** @name IEventServer methods */
      //@{
      /** get the names of all the streams */
      virtual std::vector<std::string> GetStreamNames() const override;
      /** get the current EventStreamID for a particular stream */
      virtual const EventStreamID GetEventStreamID(const std::string& streamName) const override;
      /** get the current event for a particular stream */
      virtual const std::string GetEvent( const EventStreamID& evtStreamID ) const override;
      /** get the Status of the application */
      virtual int GetState() const override;
      //@}
      
      //@{
      /** This function is exposed to allow using ERS messaging service from
       * other threads */
      virtual void Message( const MSG::Level level, const std::string& msg ) const override;
      /** Get the logging level **/
      virtual MSG::Level LogLevel() const override;
      //@}

      /**@name Event serving thread control */
      //@{
      /** Start the serving thread */
      StatusCode StartServingThread();
      /** Stop the serving thread */
      StatusCode StopServingThread();
      /** The server thread will stop once this flag is set to false */
      virtual bool GetRunServerFlag () const override { return m_runServerThread; };
      /** Callback whenever the server thread is stopped **/
      virtual void ServerThreadStopped() override;
      /** Wait for the server finish */
      void Wait();
      //@}
      
      
    private:

      //Signal handler must be static function
      static void signalHandler (int signum);

      //Static semaphore used to suspend main thread while server thread is
      //running. The semaphore will reach its post-condition either by
      // a) receiving a signal through the signal handler
      // b) the ServerThreadStopped callback being called
      inline static OWLSemaphore lock ATLAS_THREAD_SAFE;

      //Store the received signal in a static member
      inline static std::atomic<int> m_receivedSignal{0};

      //Port number property - defaults to zero in which case
      //it is dynamically assigned
      int portNumber;

      //A map of the streams and their current events
      EventStreamMap m_eventStreamMap;

      //A mutex (mutual exclusive) lock for the data map
      mutable pthread_mutex_t m_accessLock ATLAS_THREAD_SAFE;

      //A handle to the server thread
      pthread_t m_ServerThreadHandle;

      //Once this flag is set to false, the thread will stop after handling its
      //last request
      bool m_runServerThread;
  };
}

#endif
