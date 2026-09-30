#include "vk-shader-values.hpp"

#include <algorithm>
#include <map>

namespace neon
{
  namespace
  {
    // what SPIR-V starts with
    constexpr std::uint32_t magic = 0x07230203;
    constexpr std::size_t header_words = 5;

    // the instructions that are looked at, by their numbers in the
    // specification of SPIR-V
    constexpr std::uint32_t op_name = 5;
    constexpr std::uint32_t op_member_name = 6;
    constexpr std::uint32_t op_type_int = 21;
    constexpr std::uint32_t op_type_float = 22;
    constexpr std::uint32_t op_type_vector = 23;
    constexpr std::uint32_t op_type_struct = 30;
    constexpr std::uint32_t op_member_decorate = 72;

    constexpr std::uint32_t decoration_offset = 35;

    /// Text as SPIR-V holds it: bytes in words, the first byte lowest, up
    /// to a byte that is 0.
    std::string TextFrom(const std::vector<std::uint32_t> &words, const std::size_t first, const std::size_t end)
    {
      std::string text;

      for (std::size_t i = first; i < end; i++)
      {
        for (int byte = 0; byte < 4; byte++)
        {
          const char letter = static_cast<char>((words[i] >> (8 * byte)) & 0xFF);
          if (letter == '\0') { return text; }
          text += letter;
        }
      }

      return text;
    }

    struct Type
    {
      std::uint32_t count = 0;
      bool is_integer = false;
    };
  }

  const VK_ShaderValue *VK_ShaderValues::Find(const std::string &name) const
  {
    for (const auto &value : values)
    {
      if (value.name == name) { return &value; }
    }
    return nullptr;
  }

  bool VK_ShaderValues::Read(const std::vector<std::uint32_t> &words, VK_ShaderValues &values)
  {
    values = VK_ShaderValues{};

    if (words.size() < header_words || words[0] != magic) { return false; }

    std::map<std::uint32_t, std::string> names;
    std::map<std::uint32_t, Type> types;
    std::map<std::uint32_t, std::vector<std::uint32_t>> structs;
    std::map<std::uint32_t, std::map<std::uint32_t, std::string>> member_names;
    std::map<std::uint32_t, std::map<std::uint32_t, std::uint32_t>> member_offsets;

    for (std::size_t at = header_words; at < words.size();)
    {
      const std::uint32_t length = words[at] >> 16;
      const std::uint32_t operation = words[at] & 0xFFFF;

      // an instruction that is cut off, or has no length
      if (length == 0 || at + length > words.size()) { return false; }

      const std::size_t end = at + length;

      switch (operation)
      {
        case op_name:
          if (length >= 3) { names[words[at + 1]] = TextFrom(words, at + 2, end); }
          break;
        case op_member_name:
          if (length >= 4) { member_names[words[at + 1]][words[at + 2]] = TextFrom(words, at + 3, end); }
          break;
        case op_member_decorate:
          if (length >= 5 && words[at + 3] == decoration_offset)
          {
            member_offsets[words[at + 1]][words[at + 2]] = words[at + 4];
          }
          break;
        case op_type_float:
          if (length >= 3 && words[at + 2] == 32) { types[words[at + 1]] = {1, false}; }
          break;
        case op_type_int:
          if (length >= 3 && words[at + 2] == 32) { types[words[at + 1]] = {1, true}; }
          break;
        case op_type_vector:
          if (length >= 4)
          {
            if (const auto of = types.find(words[at + 2]); of != types.end() && of->second.count == 1)
            {
              types[words[at + 1]] = {words[at + 3], of->second.is_integer};
            }
          }
          break;
        case op_type_struct:
          if (length >= 2) { structs[words[at + 1]] = {words.begin() + at + 2, words.begin() + end}; }
          break;
        default:
          break;
      }

      at = end;
    }

    for (const auto &[id, members] : structs)
    {
      if (const auto name = names.find(id); name == names.end() || name->second != kBlock_Name) { continue; }

      for (std::uint32_t member = 0; member < members.size(); member++)
      {
        const auto type = types.find(members[member]);
        const auto name = member_names[id].find(member);
        const auto offset = member_offsets[id].find(member);

        if (type == types.end() || type->second.count == 0 || type->second.count > 4) { continue; }
        if (name == member_names[id].end() || offset == member_offsets[id].end()) { continue; }

        VK_ShaderValue value;
        value.name = name->second;
        value.offset = offset->second;
        value.count = type->second.count;
        value.is_integer = type->second.is_integer;

        values.size = std::max(values.size, value.offset + value.count * 4);
        values.values.push_back(value);
      }

      // a block is as large as its rules of layout say, which round up to
      // 16 bytes
      values.size = (values.size + 15) / 16 * 16;
      break;
    }

    return true;
  }
} // neon
