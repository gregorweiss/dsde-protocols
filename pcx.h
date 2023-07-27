#ifndef SDE_PROXY_PCX_H
#define SDE_PROXY_PCX_H

#include <map>
#include <vector>

void pcx( const std::map<int, std::vector<signed char> >& input,
          std::map<int, std::vector<signed char> >& output );

#endif
