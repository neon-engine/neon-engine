#ifndef DATA_BUFFER_HPP
#define DATA_BUFFER_HPP

#include <optional>
#include <vector>

namespace neon
{
  /// Stores elements in a fixed number of slots and hands out the index of
  /// the slot as an id. Ids stay valid until the element is removed, and the
  /// slot of a removed element is given out again.
  ///
  /// Elements sit next to each other in memory, so going through many of them
  /// is fast. Nothing moves once stored: a pointer from Find() stays valid
  /// until its element is removed.
  ///
  /// T has to be copyable. It does not need a default constructor.
  ///
  /// An id that holds nothing, such as one that was removed, gives nothing:
  /// Find() nullptr and Remove() an empty optional. Nothing is thrown, the
  /// caller decides what it does without the element (#179).
  template<typename T>
  class DataBuffer
  {
    int _capacity = 0;
    std::vector<T> _elements;
    std::vector<bool> _occupied;
    std::vector<int> _free_ids;

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

    /// Takes the element out and returns it, or nothing when the id holds
    /// nothing, which includes removing the same id twice. Its id can be
    /// given out again afterwards.
    std::optional<T> Remove(const int id)
    {
      if (!Contains(id)) { return std::nullopt; }

      std::optional<T> element(_elements[id]);
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

    /// The element of an id, or nullptr when the id holds nothing. The
    /// pointer stays valid until the element is removed.
    [[nodiscard]] T *Find(const int id)
    {
      return Contains(id) ? &_elements[static_cast<std::size_t>(id)] : nullptr;
    }

    [[nodiscard]] const T *Find(const int id) const
    {
      return Contains(id) ? &_elements[static_cast<std::size_t>(id)] : nullptr;
    }
  };
} // neon

#endif //DATA_BUFFER_HPP
