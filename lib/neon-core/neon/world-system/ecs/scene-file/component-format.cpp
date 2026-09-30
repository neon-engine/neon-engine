#include "component-format.hpp"

namespace neon
{
  void ComponentFormats::Add(const ComponentFormat &format)
  {
    for (auto &known : _formats)
    {
      if (known.name == format.name)
      {
        known = format;
        return;
      }
    }

    _formats.push_back(format);
  }

  const ComponentFormat *ComponentFormats::Find(const std::string &name) const
  {
    for (const auto &format : _formats)
    {
      if (format.name == name) { return &format; }
    }
    return nullptr;
  }

  const std::vector<ComponentFormat> &ComponentFormats::GetAll() const
  {
    return _formats;
  }
} // neon
