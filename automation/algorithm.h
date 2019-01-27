#ifndef _AUTOMATION_ALGORITHM_H_
#define _AUTOMATION_ALGORITHM_H_

#include <vector>
#include <algorithm>

namespace automation
{
namespace algorithm
{

template <typename ItemT>
int indexOf(const ItemT& item, const std::vector<ItemT>& list) {
  auto it = std::find(list.begin(),list.end(),item);
  if ( it == list.end() ) {
    return -1;
  }
  return std::distance(list.begin(),it);
}

} // namespace algorithm
} // namespace automation

#endif
