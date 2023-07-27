#ifndef SDE_PROXY_NBX_H
#define SDE_PROXY_NBX_H

#include <map>
#include <vector>

void nbx( const std::map<int, std::vector<signed char> >& data,
          std::map<int, std::vector<signed char> >& recvBuffer );

#endif
