#include "pex.h"
#include "mpi.h"

void pex( const std::map<int, std::vector<signed char> >& messages,
          std::map<int, std::vector<signed char> >& recvBuffer ) {
  int rank;
  int size;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  int tag = 314159;
  
  // distribute vector of message sizes with personalized exchange
  std::vector<int> sendMsgSizes( size, 0 );
  for ( const auto& msg: messages ) {
    sendMsgSizes[ msg.first ] = msg.second.size();
  }
  std::vector<int> recvMsgSizes( size, 0 );
  MPI_Alltoall( &( sendMsgSizes[ 0 ] ),
                1,
                MPI_INT,
                &( recvMsgSizes[ 0 ] ),
                1,
                MPI_INT,
                MPI_COMM_WORLD );
  
  // post nonblocking receive operations for non-zero send counts
  std::vector<MPI_Request> irRequests{};
  for ( int sender_rank = 0; sender_rank < size; ++sender_rank ) {
    auto count = recvMsgSizes[ sender_rank ];
    if ( count > 0 ) {
      MPI_Request request;
      std::vector<signed char> recvMessage( count );
      MPI_Irecv( &( recvMessage[0] ),
                 count,
                 MPI_SIGNED_CHAR,
                 sender_rank,
                 tag,
                 MPI_COMM_WORLD,
                 &request );
      recvBuffer[ sender_rank ] = std::move( recvMessage );
      irRequests.push_back( request );
    }
  }
  
  // post send operations
  for ( const auto& msg: messages ) {
    MPI_Send( &( msg.second[ 0 ] ),
              msg.second.size(),
              MPI_SIGNED_CHAR,
              msg.first,
              tag,
              MPI_COMM_WORLD );
  }

  // wait for nonblocking receive operations
  MPI_Waitall( irRequests.size(), irRequests.data(), MPI_STATUS_IGNORE );
}
