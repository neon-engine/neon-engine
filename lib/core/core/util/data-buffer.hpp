#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include <iostream>
#include <optional>
#include <queue>
#include <vector>

namespace core
{
  template<typename T>
  class DataBuffer
  {
    int _index = 0;
    int _capacity = 0;
    std::vector<std::optional<T> > _refs;
    std::queue<int> _deleted_ids{};

  public:
    explicit DataBuffer(const int max_capacity)
    {
      _capacity = max_capacity;
      _refs.reserve(_capacity);
    }

    int Add(const T &element)
    {
      const auto id = GetId();
      if (id < 0) { return id; }

      _refs[id] = element;
      return id;
    }

    bool Remove(int index, T &out)
    {
      out = _refs[index];
      if (out == std::nullopt)
      {
        std::cerr << "Nothing to remove at index " << index << "!" << std::endl;
        return false;
      }
      _refs[_index] = std::nullopt;
      _deleted_ids.push(_index);

      return true;
    }

    bool Get(int index, T &out)
    {
      out = _refs[index];
      if (out == std::nullopt)
      {
        std::cerr << "Element at index " << index << " not found!" << std::endl;
      }

      return true;
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

      std::cerr <<
          "Could not create id for buffer with since it is at capacity (" << _capacity << " elements)"
          << std::endl;
      return -1;
    }
  };
} // core

#endif //DATA_BUFFER_HPP
