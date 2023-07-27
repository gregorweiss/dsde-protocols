#include "helper.h"
#include <iostream>
#include "mpi.h"

void print_messages( const std::map<int, std::vector<signed char>>& rank_message_pairs ) {
  int rank;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  std::cout << "On rank " << rank << "\n";
  for ( const auto& pair: rank_message_pairs ) {
    std::cout << "Message to/from " << pair.first << ": ";
    for ( const auto& value: pair.second ) {
      std::cout << static_cast<int>(value) << ", ";
    }
    std::cout << "; ";
  }
  std::cout << std::endl;
}