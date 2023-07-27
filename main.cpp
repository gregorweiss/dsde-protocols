#include <iostream>
#include <string.h>

#include <random>
#include <algorithm>
#include <limits>

#include "mpi.h"

#include "pex.h"
#include "pcx.h"
#include "rsx.h"
#include "nbx.h"

#include "helper.h"

std::vector<int> random_recipients( int k ) {
  int rank;
  int size;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  
  std::random_device rd;
  std::mt19937_64 gen64( rd() );
  std::vector<int> weights( size, 1 );
  weights[ rank ] = 0;
  std::discrete_distribution<int> discrete_dist( weights.begin(), weights.end() );
  
  std::vector<int> all_ranks( ( size - 1 ), -1 );
  std::iota( all_ranks.begin(),
             all_ranks.begin() + rank,
             0 );
  std::iota( all_ranks.begin() + rank,
             all_ranks.end(),
             ( rank + 1 ) );
  
  std::vector<int> receivers{};
  std::sample( all_ranks.begin(),
               all_ranks.end(),
               std::back_inserter( receivers ),
               k,
               gen64 );
  
  return receivers;
}

std::vector<std::vector<signed char> > random_messages( const int min_msg_size,
                                                        const int max_msg_size,
                                                        const int k ) {
  
  std::random_device rd;
  std::mt19937_64 gen64( rd() );
  std::uniform_int_distribution<int> dist_msg_sizes( min_msg_size, max_msg_size );
  std::uniform_int_distribution<signed char> dist_messages( std::numeric_limits<signed char>::min(),
                                                            std::numeric_limits<signed char>::max() );
  std::vector<std::vector<signed char> > messages( k );
  for ( auto& message: messages ) {
    message = std::vector<signed char>( dist_msg_sizes( gen64 ) );
    std::generate( message.begin(), message.end(),
                   [ &dist_messages, &gen64 ]()
                   { return dist_messages( gen64 ); } );
  }
  
  return messages;
}

std::map<int, std::vector<signed char>>
zip( std::vector<int>& receivers, const std::vector<std::vector<signed char>>& messages ) {
  std::map<int, std::vector<signed char> > receiver_message_pairs;
  std::transform( receivers.begin(), receivers.end(), messages.begin(),
                  std::inserter( receiver_message_pairs, receiver_message_pairs.end() ),
                  []( const auto& recv, const auto& msg )
                  { return std::make_pair( recv, msg ); } );
  return receiver_message_pairs;
}

int main( int argc, char* argv[] ) {
  
  const int k = std::stoi( argv[ 1 ] ); // 6
  const int min_msg_size = std::stoi( argv[ 2 ] ); // 1
  const int max_msg_size = std::stoi( argv[ 3 ] ); // 1024
  const int iterations = std::stoi( argv[ 4 ] ); // 1000
  const int protocol = std::stoi( argv[ 5 ] ); // [0: pex, 1: pcx, 2: rsx, 3: nbx]
  bool check = false;
  if ( argc == 7 && strcmp( argv[ 6 ], "-check" ) == 0 ) {
    check = true;
  }
  
  MPI_Init( &argc, &argv );
  
  int rank;
  int size;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  if ( size == 1 ) {
    printf( "This application is meant to be run with more than 1 MPI processes.\n" );
    MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
  }

  double start;
  std::vector<double> timings(iterations, 0.0);
  for ( int iter = 0; iter < iterations; ++iter ) {

    auto recipients = random_recipients( k );
    auto messages = random_messages( min_msg_size, max_msg_size, k );
    auto outgoing_messages = zip( recipients, messages );

    std::map<int, std::vector<signed char>> incoming_messages;
    if ( protocol == 0 ) {
      MPI_Barrier( MPI_COMM_WORLD );
      start = MPI_Wtime();
      pex( outgoing_messages, incoming_messages );
      MPI_Barrier( MPI_COMM_WORLD );
      timings[iter] = MPI_Wtime() - start;
    } else if ( protocol == 1 ) {
      MPI_Barrier( MPI_COMM_WORLD );
      start = MPI_Wtime();
      pcx( outgoing_messages, incoming_messages );
      MPI_Barrier( MPI_COMM_WORLD );
      timings[iter] = MPI_Wtime() - start;
    } else if ( protocol == 2 ) {
      MPI_Barrier( MPI_COMM_WORLD );
      start = MPI_Wtime();
      rsx( outgoing_messages, incoming_messages );
      MPI_Barrier( MPI_COMM_WORLD );
      timings[iter] = MPI_Wtime() - start;
    } else if ( protocol == 3 ) {
      MPI_Barrier( MPI_COMM_WORLD );
      start = MPI_Wtime();
      nbx( outgoing_messages, incoming_messages );
      MPI_Barrier( MPI_COMM_WORLD );
      timings[iter] = MPI_Wtime() - start;
    }
    
    if ( check ) {
      std::map<int, std::vector<signed char> > retour_messages;
      nbx( incoming_messages, retour_messages );
      bool equal = std::equal( outgoing_messages.begin(),
                               outgoing_messages.end(),
                               retour_messages.begin() );
      if ( !equal ) {
        std::cout << rank << ": returned messages are corrupted" << std::endl;
      }
    }
  }
  
  std::vector<double> max_times(iterations, 0.0);
  MPI_Reduce( timings.data(),
              max_times.data(),
              iterations,
              MPI_DOUBLE,
              MPI_MAX,
              0,
              MPI_COMM_WORLD );
  if ( rank == 0 ) {
    auto sum = std::accumulate( max_times.begin(), max_times.end(), 0.0);
    auto mean = sum / max_times.size();
    std::vector<double> sqrs(iterations, 0.0);
    std::transform( max_times.begin(),
                    max_times.end(), 
                    sqrs.begin(),
                    [&mean](const auto& time)
                    { return (time - mean) * (time - mean); } );
    auto sqr_sum = std::accumulate( sqrs.begin(), sqrs.end(), 0.0 );
    sqr_sum /= sqrs.size();
    auto stdev = std::sqrt( sqr_sum );
    std::cout << "Used ";
    if ( protocol == 0 ) { std::cout << "PEX"; }
    else if ( protocol == 1 ) { std::cout << "PCX"; }
    else if ( protocol == 2 ) { std::cout << "RSX"; }
    else if ( protocol == 3 ) { std::cout << "NBX"; }
    std::cout << " protocol on " << size << " ranks ";
    std::cout << " -- sum and mean time of communication (sec) : " << sum << " " << mean << " +/- " << stdev << "\n";
  }
  
  MPI_Finalize();
  
  return 0;
}
