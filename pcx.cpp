#include "pcx.h"
#include "mpi.h"

void pcx( const std::map<int, std::vector<signed char> >& data,
          std::map<int, std::vector<signed char> >& recvBuffer ) {
  int rank;
  int size;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  int tag = 314159;
  
  // allocate local table with P entries, initialize all entries to 0
  std::vector<int> recvIDs( size, 0 );
  // set row target(i) in local table to 1
  for ( const auto& target: data ) { recvIDs[ target.first ] = 1; }
  // global sum of table row to count the number of incoming messages
  int msgCount = 0;
  std::vector<int> recvcounts( size, 1 );
  MPI_Reduce_scatter( &( recvIDs[ 0 ] ),
                      &msgCount,
                      &( recvcounts[ 0 ] ),
                      MPI_INT,
                      MPI_SUM,
                      MPI_COMM_WORLD );
  
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
  
  //std::map<int, std::vector<signed char> > recvBuffer{};
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