#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include <stdexcept>
#include <string>
#include <vector>

namespace neon
{
  /// Stores elements in a fixed number of slots and hands out the index of
  /// the slot as an id. Ids stay valid until the element is removed, and the
  /// slot of a removed element is given out again.
  ///
  /// Elements sit next to each other in memory, so going through many of them
  /// is fast. Nothing moves once stored: a reference from operator[] stays
  /// valid until its element is removed.
  ///
  /// T has to be copyable. It does not need a default constructor.
  template<typename T>
  class DataBuffer
  {
    int _capacity = 0;
    std::vector<T> _elements;
    std::vector<bool> _occupied;
    std::vector<int> _free_ids;

    void RequireOccupied(const int id) const
    {
      if (!Contains(id))
      {
        throw std::out_of_range("DataBuffer holds no element with id " + std::to_string(id));
      }
    }

  public:
    explicit DataBuffer(const int max_capacity)
    {
      _capacity = max_capacity > 0 ? max_capacity : 0;

      // all the room is set aside once, so that adding never moves elements
      _elements.reserve(_capacity);
      _occupied.reserve(_capacity);
    }

    /// Stores a copy of the element. Returns its id, or -1 when every slot is
    /// taken.
    int Add(const T &element)
    {
      if (!_free_ids.empty())
      {
        const int id = _free_ids.back();
        _free_ids.pop_back();

        _elements[id] = element;
        _occupied[id] = true;
        return id;
      }

      if (Size() + static_cast<int>(_free_ids.size()) >= _capacity) { return -1; }

      _elements.push_back(element);
      _occupied.push_back(true);
      return static_cast<int>(_elements.size()) - 1;
    }

    /// Takes the element out and returns it. Its id can be given out again
    /// afterwards. Throws std::out_of_range if the id holds nothing, which
    /// includes removing the same id twice.
    T Remove(const int id)
    {
      RequireOccupied(id);

      T element = _elements[id];
      _occupied[id] = false;
      _free_ids.push_back(id);
      return element;
    }

    /// Whether the id currently holds an element.
    [[nodiscard]] bool Contains(const int id) const
    {
      return id >= 0 && id < static_cast<int>(_elements.size()) && _occupied[id];
    }

    /// Number of elements currently stored.
    [[nodiscard]] int Size() const
    {
      return static_cast<int>(_elements.size() - _free_ids.size());
    }

    /// Number of elements that can be stored at most.
    [[nodiscard]] int Capacity() const
    {
      return _capacity;
    }

    /// Throws std::out_of_range if the id holds nothing.
    T &operator[](const int id)
    {
      RequireOccupied(id);
      return _elements[id];
    }

    /// Throws std::out_of_range if the id holds nothing.
    const T &operator[](const int id) const
    {
      RequireOccupied(id);
      return _elements[id];
    }
  };
} // neon

#endif //DATA_BUFFER_HPP
