#include "rsx.h"
#include "mpi.h"

void rsx( const std::map<int, std::vector<signed char> >& data,
          std::map<int, std::vector<signed char> >& recvBuffer ) {
  int rank;
  int size;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  const int tag = 314159;
  
  // increase message counter via RMA
  int msgCount = 0;
  const int bytes = sizeof( int );
  const int one = 1;
  MPI_Win window;
  MPI_Win_create( &msgCount, bytes, bytes, MPI_INFO_NULL, MPI_COMM_WORLD, &window );
  MPI_Win_fence( 0, window );
  for ( const auto& target: data ) {
    MPI_Accumulate( &one, 1, MPI_INT, target.first, 0, 1, MPI_INT, MPI_SUM, window );
  }
  MPI_Win_fence( 0, window );
  MPI_Win_free( &window );
  
  // start nonblocking send operations
  std::vector<MPI_Request> isRequests;
  isRequests.reserve( data.size() );
  for ( const auto& msg: data ) {
    MPI_Request request;
    MPI_Isend( &( msg.second[ 0 ] ),
               msg.second.size(),
               MPI_SIGNED_CHAR,
               msg.first,
               tag,
               MPI_COMM_WORLD,
               &request );
    isRequests.push_back( request );
  }
  
  // probe and receive the announced messages
  for ( int s_i = 0; s_i < msgCount; ++s_i ) {
    // blocking probe for incoming message
    MPI_Status status;
    MPI_Probe( MPI_ANY_SOURCE,
               tag,
               MPI_COMM_WORLD,
               &status );
    // allocate buffer
    int count;
    MPI_Get_count( &status, MPI_SIGNED_CHAR, &count );
    std::vector<signed char> recvMessage( count );
    // receive message
    MPI_Recv( &( recvMessage[ 0 ] ),
              count,
              MPI_SIGNED_CHAR,
              status.MPI_SOURCE,
              tag,
              MPI_COMM_WORLD,
              MPI_STATUS_IGNORE );
    recvBuffer[ status.MPI_SOURCE ] = std::move( recvMessage );
  }
}
