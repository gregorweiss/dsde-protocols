#include "nbx.h"
#include "mpi.h"

void nbx( const std::map<int, std::vector<signed char> >& data,
          std::map<int, std::vector<signed char> >& recvBuffer ) {
  int rank;
  int size;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  MPI_Barrier( MPI_COMM_WORLD );
  int tag = 314159;
  
  // post nonblocking synchronous send messages with data
  std::vector<MPI_Request> issRequests;
  issRequests.reserve( data.size() );
  for ( const auto& msg: data ) {
    MPI_Request request;
    MPI_Issend( &( msg.second[ 0 ] ),
                msg.second.size(),
                MPI_SIGNED_CHAR,
                msg.first,
                tag,
                MPI_COMM_WORLD,
                &request );
    issRequests.push_back( request );
  }
  
  bool barrier_activated = false;
  MPI_Request barrier = MPI_REQUEST_NULL;
  int done = 0;
  while ( !done ) {
    int flag = 0;
    int count = 0;
    // probe for incoming message
    MPI_Status status;
    MPI_Iprobe( MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag, &status );
    if ( flag ) {
      // post receive operation
      MPI_Get_count( &status, MPI_SIGNED_CHAR, &count );
      std::vector<signed char> recvMessage( count );
      MPI_Recv( &( recvMessage[ 0 ] ),
                count,
                MPI_SIGNED_CHAR,
                status.MPI_SOURCE,
                tag,
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE );
      recvBuffer[ status.MPI_SOURCE ] = std::move( recvMessage );
    }
    
    if ( barrier_activated ) {
      // test if all processes have reached the nonblocking barrier
      MPI_Test( &barrier, &done, MPI_STATUS_IGNORE );
    } else {
      // test if all posted send operations have been received
      int sent;
      MPI_Testall( issRequests.size(), issRequests.data(), &sent, MPI_STATUSES_IGNORE );
      if ( sent ) {
        // activate nonblocking barrier
        MPI_Ibarrier( MPI_COMM_WORLD, &barrier );
        barrier_activated = true;
      }
    }
  }
}