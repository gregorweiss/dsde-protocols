#ifndef SDE_PROXY_PEX_H
#define SDE_PROXY_PEX_H

#include <map>
#include <vector>

void pex( const std::map<int, std::vector<signed char> >& messages,
          std::map<int, std::vector<signed char> >& recvBuffer );

#endif
