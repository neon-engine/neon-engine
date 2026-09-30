#ifndef UI_BINDING_HPP
#define UI_BINDING_HPP

#include <string>

#include <neon/data/data-value.hpp>

namespace neon
{
  /// A value of an element that may follow a value of the game, and change
  /// it: `value: 75`, or `value: "{volume}"`. What is typed and chosen goes
  /// to the value of the game, and what the game sets is shown.
  namespace UiBinding
  {
    /// Reads what is written into either the text it is, or the name in
    /// brackets. Returns false for something that is neither text, a
    /// number, nor a flag.
    inline bool Read(const DataValue &value, std::string &text, std::string &binding)
    {
      std::string written;

      if (bool flag = false; value.GetBool(flag))
      {
        written = flag ? "true" : "false";
      } else if (double number = 0.0; value.GetNumber(number))
      {
        // as a file writes it: without a point unless it has a fraction
        const auto whole = static_cast<long long>(number);
        written = static_cast<double>(whole) == number ? std::to_string(whole) : std::to_string(number);
      } else if (!value.GetText(written))
      {
        return false;
      }

      if (written.size() > 2 && written.front() == '{' && written.back() == '}' &&
          written.find(' ') == std::string::npos)
      {
        binding = written.substr(1, written.size() - 2);
        text.clear();
        return true;
      }

      binding.clear();
      text = written;
      return true;
    }
  }
} // neon

#endif //UI_BINDING_HPP
