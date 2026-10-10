#include "settings-subscription.hpp"

#include "settings-store.hpp"

namespace neon
{
  SettingsSubscription::SettingsSubscription(SettingsStore *store, const std::weak_ptr<int> &token, const int id)
  {
    _store = store;
    _token = token;
    _id = id;
  }

  SettingsSubscription::SettingsSubscription(SettingsSubscription &&other) noexcept
  {
    _store = other._store;
    _token = std::move(other._token);
    _id = other._id;
    other._store = nullptr;
    other._id = 0;
  }

  SettingsSubscription &SettingsSubscription::operator=(SettingsSubscription &&other) noexcept
  {
    if (this == &other) { return *this; }
    End();
    _store = other._store;
    _token = std::move(other._token);
    _id = other._id;
    other._store = nullptr;
    other._id = 0;
    return *this;
  }

  SettingsSubscription::~SettingsSubscription()
  {
    End();
  }

  bool SettingsSubscription::IsActive() const
  {
    return _id != 0 && !_token.expired();
  }

  void SettingsSubscription::End()
  {
    if (_id != 0 && !_token.expired()) { _store->Unsubscribe(_id); }
    _id = 0;
    _store = nullptr;
    _token.reset();
  }
} // neon
