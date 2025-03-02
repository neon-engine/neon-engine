#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include <iostream>
#include <queue>
#include <vector>

namespace core
{
  template<typename T>
  class DataBuffer
  {
    int _index = 0;
    int _capacity = 0;
    std::vector<T> _refs;
    std::queue<int> _deleted_ids{};
    std::shared_ptr<Logger> _logger;

  public:
    DataBuffer(const int max_capacity, const std::shared_ptr<Logger> logger)
    {
      _capacity = max_capacity;
      _refs.reserve(_capacity);
      _logger = logger;
    }

    int Add(const T &element)
    {
      const auto id = GetId();
      if (id < 0) { return id; }

      _refs[id] = element;
      return id;
    }

    T Remove(int index)
    {
      // TODO [issues/2] setup a way so that multiple removals are not possible
      const auto out = _refs[index];
      _deleted_ids.push(_index);
      return out;
    }

    T& operator[](int index)
    {
      return _refs[index];
    }

    const T& operator[](int index) const
    {
      return _refs[index];
    }

  private:
    int GetId()
    {
      if (!_deleted_ids.empty())
      {
        const auto id = _deleted_ids.front();
        _deleted_ids.pop();
        return id;
      }

      if (_index < _capacity)
      {
        return _index++;
      }

      _logger->Error("Could not create id for buffer with since it is at capacity {} elements)", _capacity);
      return -1;
    }
  };
} // core

#endif //DATA_BUFFER_HPP
