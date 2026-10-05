#include "pool-manager-format.hpp"

#include <format>

#include <neon/world-system/ecs/components/pool-manager.hpp>

namespace neon
{
  ComponentFormat PoolManagerFormat()
  {
    return ComponentFormat::Of<PoolManager>(
      "PoolManager",
      [](const DataReader &reader, PoolManager &pool)
      {
        const DataValue *entries = reader.ReadValue("_entries");
        if (entries == nullptr) { return; }
        if (!entries->IsList())
        {
          reader.Report(*entries, std::format(
                          "'_entries' of {} is {}, where a list of what the pool holds was expected, each with a "
                          "'source' and a 'count'",
                          reader.GetWhere(), DataValue::Describe(entries->GetKind())));
          return;
        }

        pool.entries.clear();
        for (const DataValue &item : entries->GetItems())
        {
          PoolEntry entry;
          float count = 0.0f;
          const DataValue *source = item.IsMap() ? item.Find("source") : nullptr;
          const DataValue *number = item.IsMap() ? item.Find("count") : nullptr;
          if (source == nullptr || !source->GetText(entry.source) || entry.source.empty() || number == nullptr ||
              !number->GetNumber(count) || count < 1.0f)
          {
            reader.Report(item, std::format(
                            "An entry of '_entries' of {} needs a 'source', a prefab as "
                            "assets://prefabs/nail.prefab.yml or an entity as instance://nail, and a 'count' of 1 or more",
                            reader.GetWhere()));
            continue;
          }
          entry.count = static_cast<int>(count);
          pool.entries.push_back(entry);
        }
      },
      [](const PoolManager &pool, DataValue &map)
      {
        auto entries = DataValue::List();
        for (const PoolEntry &entry : pool.entries)
        {
          auto item = DataValue::Map();
          item.Set("source", DataValue::Text(entry.source));
          item.Set("count", DataValue::Number(entry.count));
          entries.Add(item);
        }
        map.Set("_entries", entries);
      });
  }
} // neon
