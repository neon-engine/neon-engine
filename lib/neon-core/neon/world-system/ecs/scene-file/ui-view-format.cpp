#include "ui-view-format.hpp"

#include <format>

#include <neon/world-system/ecs/components/ui-view.hpp>
#include <neon/world-system/ecs/systems/ui-view-loading.hpp>

namespace neon
{
  ComponentFormat UiViewFormat()
  {
    return ComponentFormat::Of<UiView>(
      UiViewLoading::kComponent_Name,
      [](const DataReader &reader, UiView &view)
      {
        if (!reader.Read("file", view.file) && !reader.Has("file"))
        {
          reader.Report(std::format(
            "{} has no 'file', where the virtual path of a user interface was expected", reader.GetWhere()));
        }
      },
      [](const UiView &view, DataValue &map)
      {
        map.Set("file", DataValue::Text(view.file));
      });
  }
} // neon
