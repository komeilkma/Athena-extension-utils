/*
  Copyright (C) 2022 Komeilkma/leandrogardi for the benefit of the ATLAS collaboration
*/

#include <JiveXMLServer/JiveXMLServer.h>

//tdaq includes
#include <ers/ers.h>

//JiveXML includes
#include <JiveXML/ONCRPCThreadCollection.h>
#include <JiveXML/ONCRPCServerThreads.h>
#include <JiveXML/ONCRPCXDRProcs.h>
#include <JiveXML/ONCRPCServer.h>

#include <signal.h>

//Define warning and error
#define ERS_WARNING( message ) \
{ \
  ERS_REPORT_IMPL( ers::warning, ers::Message, message, ); \
}      

#define ERS_ERROR( message ) \
{ \
  ERS_REPORT_IMPL( ers::error, ers::Message, message, ); \
}      

namespace {
  std::string fmterror(int code) {
    char buf[256];
    return std::string(strerror_r(code, buf, sizeof(buf)));
  }
}

namespace JiveXML {

  /**
   * Constructor
   */
  JiveXMLServer::JiveXMLServer( int port ):
    portNumber(port){

    //Make sure ServerThread does not start unexpectedly
    m_runServerThread = false ;

    //And then start it
    StartServingThread().ignore();

    //Also register the signal handlers
    signal( SIGINT , JiveXMLServer::signalHandler );
    signal( SIGTERM, JiveXMLServer::signalHandler );
  }

  /**
   * Destructor
   */
  JiveXMLServer::~JiveXMLServer(){
  
    //Just  stop the serving thread
    StopServingThread().ignore();
  }

  /**
   * Create the server by
   * - passing a this-pointer as an argument
   * - setting the thread-running flag
   * - starting the server thread
   */
  StatusCode JiveXMLServer::StartServingThread(){

    ERS_DEBUG(MSG::VERBOSE,"StartServingThread()");

    //Initialize access lock mechanism to ensure that the data map is never
    //accessed by more than one thread at the same time. NULL means default
    int retVal = pthread_mutex_init(&m_accessLock,NULL);
    if (retVal != 0){
       ERS_WARNING("Unable to initialize access lock while starting server: " << fmterror(retVal));
       return StatusCode::FAILURE;
    }

    //The arguments passed on to the server - create new object on the heap that
    //is persistent through the lifetime of the thread
    ServerThreadArguments* args = new ServerThreadArguments(this,portNumber);

    //set runServer flag to true, so the  thread will start
    m_runServerThread = true ;

    //create thread itself
    if ( ( pthread_create(&m_ServerThreadHandle, NULL , &JiveXML::ONCRPCServerThread, (void *) args )) != 0){
      
      //Create thread failed
      ERS_WARNING("Thread creation failed");
      return StatusCode::FAILURE;
    }

    return StatusCode::SUCCESS;

  }

  /**
   * Stop the server by
   * - unsetting the thread-running flag
   * - waiting till the server thread has finished
   * - destroying the thread handle
   */
  StatusCode JiveXMLServer::StopServingThread(){

    ERS_DEBUG(MSG::VERBOSE,"StopServingThread()");

    /**
     * Ping the server which will cause another request
     * Otherwise the server won't update its loop condition
     */

    //Create a client - this will already cause an update of the file
    //descriptors on the sockets
    CLIENT* client = clnt_create("localhost", ONCRPCSERVERPROG,ONCRPCSERVERVERS, "tcp");
    if (!client){
      ERS_ERROR("Unable to create shutdown client for local server");
      return StatusCode::FAILURE;
    }

    // Next unset the runServerThread flag, which will cause the loop to stop
    // This needs to happen after client creation, otherwise the next call won't
    // be serverd anymore
    m_runServerThread = false ; 

    // Now issue the call with a timeout
    struct timeval timeout; timeout.tv_sec = 1; timeout.tv_usec = 0;
// xdr_void is defined inconsistently in xdr.h and gets a warning from gcc8.
#if __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    clnt_call(client, NULLPROC, (xdrproc_t)xdr_void, NULL, (xdrproc_t)xdr_void, NULL, timeout);
#if __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

    // A pointer to the return value of the thread
    void* ret = NULL;
    // wait till the server thread has finished
    ERS_INFO("Waiting for server thread to terminate ...");
    pthread_join(m_ServerThreadHandle, &ret);
    ERS_INFO(" ... finished server thread");

    //check if there was a return value
    if (ret){
      //Get the return value
      unsigned long NRequests = *(unsigned long*)ret;
      ERS_DEBUG(MSG::DEBUG,"Server thread stopped after handling " << NRequests << " requests");
    } else 
      ERS_WARNING("Server thread stopped unexpectedly");

    //Destroy the access lock
    int retVal = pthread_mutex_destroy(&m_accessLock);
    if (retVal != 0){
       ERS_WARNING("Unable to destroy access lock after stopping server: " << fmterror(retVal));
       return StatusCode::FAILURE;
    }

    return StatusCode::SUCCESS;
  }

  /**
   * When the signal handler is called, switch the lock to the post condition
   */
  void JiveXMLServer::signalHandler(int signal){
    //Store signal
    m_receivedSignal=signal;
    //finish semaphore lock
    lock.post();
  }

  /**
   * Wait for the server thread to finish. This can be because
   * a) we received a SIGTERM or SIGINT signal
   * b) the server thread stopped by itself
   **/
  void JiveXMLServer::Wait(){
    //just wait for the lock
    lock.wait();
    //Tell why the lock was released
    ERS_INFO("Reached post-condition after received signal " << m_receivedSignal );
  }

  /**
   * When the server thread stopped, we will also call the
   * the signal handler with the special value -1
   **/
  void JiveXMLServer::ServerThreadStopped(){
    //call the signal handler, so we will also reach post condition
    signalHandler(-1);
  }

  /**
   * Get one event and put it as the new event for the this stream, which is
   * identified by EventStreamID
   */
  StatusCode JiveXMLServer::UpdateEventForStream( const EventStreamID& evtStreamID, const std::string & event) {
 
    ERS_DEBUG(MSG::VERBOSE,"UpdateEventForStream");

    //Check that the event stream id is valid
    if (!evtStreamID.isValid()){
      ERS_ERROR("Invalid event stream identifier - cannot add event");
      return StatusCode::FAILURE;
    }

    //Make sure we don't have already exceeded the maximum number of streams
    if (m_eventStreamMap.size() > NSTREAMMAX ){
      ERS_ERROR("Reached max. allowed number of streams " << NSTREAMMAX << " - cannot add event");
      return StatusCode::FAILURE;
    }

    //Make sure the event is not larger than the allowed maximal size
    if (event.length() > NBYTESMAX ){
      ERS_ERROR("Event is larger than allowed max. of " << NBYTESMAX << " bytes - cannot add event");
      return StatusCode::FAILURE;
    }

    //Make sure we are the only one accessing the data right now, by trying to
    //obtain a lock. If the lock can not be obtained after a certain time, an
    //error is reported
    
    //Timeout of 30 second and 0 nanoseconds
    struct timespec timeout = { 30, 0 };
    //Try to obtain the lock
    int retVal = pthread_mutex_timedlock(&m_accessLock, &timeout);
    if ( retVal != 0 ){
      ERS_ERROR("Unable to obtain access lock to update event: " << fmterror(retVal));
      return StatusCode::FAILURE;
    }

    //Using try/catch to ensure the mutex gets unlocked in any case
    try {

      //Using std::map::operator[] and std::map::insert() will create a new event
      //if it did not exist, otherwise just replace the existing entry (making a
      //copy of the std::string) but would not update the key which holds new
      //event/run number. Therefore delete existing entry first.

      //Delete old entry if there is one
      EventStreamMap::iterator OldEvtItr = m_eventStreamMap.find(evtStreamID);
      if (OldEvtItr != m_eventStreamMap.end()) 
        m_eventStreamMap.erase(OldEvtItr);
      
      //Now add the new event
      m_eventStreamMap.insert(EventStreamPair(evtStreamID,event));
    
    } catch ( const std::exception& e ) {
      ERS_ERROR("Exception caught while updating event for stream "
                << evtStreamID.StreamName() << ": " << e.what());
      //Also release the lock in this case
      pthread_mutex_unlock(&m_accessLock);
      //before we return
      return StatusCode::FAILURE;
    }

    //Finally release the lock again
    retVal = pthread_mutex_unlock(&m_accessLock);
    if ( retVal != 0 ){
      ERS_ERROR("Unable to release access lock after updating event: " << fmterror(retVal));
      return StatusCode::FAILURE;
    }

    ERS_DEBUG(MSG::DEBUG, "Updated stream " << evtStreamID.StreamName() 
               << " with event Nr. " << evtStreamID.EventNumber() 
               << " from run Nr. " << evtStreamID.RunNumber());

    return StatusCode::SUCCESS;
  }


  /**
   * Return the current athena state
   */
  int JiveXMLServer::GetState() const {
    /**
     * at the moement return a fixed value - will change this to some timeout
     * condition soon.
     **/
    return 3;
  }

  /**
   * Return an array with all the stream names
   */
  std::vector<std::string> JiveXMLServer::GetStreamNames() const {
    
    //Create a vector that can be returned
    std::vector<std::string> StreamNames;
    
    //Obtain an exclusive access lock
    int retVal = pthread_mutex_lock(&m_accessLock);
    if ( retVal != 0 ){
      ERS_ERROR("Unable to obtain access lock to get stream names: " << fmterror(retVal));
      return StreamNames;
    }

    // Iterate over map to get entries
    EventStreamMap::const_iterator MapItr = m_eventStreamMap.begin();
    for ( ; MapItr != m_eventStreamMap.end(); ++MapItr){
      
      //Get the EventStreamID object
      EventStreamID EvtStrID = (*MapItr).first;

      //Add the name of this EventStreamID to the list of stream names
      StreamNames.push_back(EvtStrID.StreamName());
    }

    //Release the lock
    retVal = pthread_mutex_unlock(&m_accessLock);
    if ( retVal != 0 )
      ERS_ERROR("Unable to release access lock after getting stream names: " << fmterror(retVal));

    //Return the list of names
    return StreamNames;
  }

  /**
   * Return the EventStreamID for the last event of a given stream
   */
  const EventStreamID JiveXMLServer::GetEventStreamID( const std::string& StreamName) const {
    
    //Obtain an exclusive access lock
    int retVal = pthread_mutex_lock(&m_accessLock);
    if ( retVal != 0 ){
      ERS_ERROR("Unable to obtain access lock to get stream ID: " << fmterror(retVal));
      return EventStreamID("");
    }

    // Search the entry in the map
    EventStreamMap::const_iterator MapItr = m_eventStreamMap.find(EventStreamID(StreamName));

    //Initialize with an invalid event stream identifier
    EventStreamID streamID = EventStreamID("");

    //If the element is found, get a copy of the found event stream identifier
    if ( MapItr != m_eventStreamMap.end()){
      streamID = EventStreamID((*MapItr).first);
    }

    //Release the lock
    retVal = pthread_mutex_unlock(&m_accessLock);
    if ( retVal != 0 )
      ERS_ERROR("Unable to release access lock after getting stream ID: " << fmterror(retVal));

    return streamID;
 
  }

  /**
   * Return the event for a given stream
   */
  const std::string JiveXMLServer::GetEvent( const EventStreamID& evtStreamID ) const {
    
    //Obtain an exclusive access lock
    int retVal = pthread_mutex_lock(&m_accessLock);
    if ( retVal != 0 ){
      ERS_ERROR("Unable to obtain access lock to get event: " << fmterror(retVal));
      return std::string("");
    }


    // Search the entry in the map
    EventStreamMap::const_iterator MapItr = m_eventStreamMap.find(evtStreamID);

    //Initialize with an empty event stream
    std::string event;

    //If the element is found, get a copy of the found event string
    if ( MapItr != m_eventStreamMap.end()){
      event = std::string((*MapItr).second);
    }

    //Release the lock
    retVal = pthread_mutex_unlock(&m_accessLock);
    if ( retVal != 0 )
      ERS_ERROR("Unable to release access lock after getting stream event: " << fmterror(retVal));

    return event;
  }

  /** 
   * Deliver a message - possibly from another thread - to ERS;
   */
  void JiveXMLServer::Message( const MSG::Level level, const std::string& msg) const {
    //Deliver message to the proper stream
    if (level <= MSG::DEBUG) ERS_REPORT_IMPL( ers::debug, ers::Message, msg, level);
    if (level == MSG::INFO) ERS_REPORT_IMPL( ers::info, ers::Message, msg, );
    if (level == MSG::WARNING) ERS_REPORT_IMPL( ers::warning, ers::Message, msg, );
    if (level == MSG::ERROR) ERS_REPORT_IMPL( ers::error, ers::Message, msg, );
    if (level >= MSG::FATAL) ERS_REPORT_IMPL( ers::fatal, ers::Message, msg, );
  }

  /** 
   * Currently only used to suppress client hostname lookup if not in debug mode
   */
   MSG::Level JiveXMLServer::LogLevel() const {
    //set to fixed value for now
    return MSG::DEBUG;
  }
}
