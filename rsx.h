#ifndef SDE_PROXY_RSX_H
#define SDE_PROXY_RSX_H

#include <map>
#include <vector>

void rsx( const std::map<int, std::vector<signed char> >& data,
          std::map<int, std::vector<signed char> >& rcvBuffer );

#endif
