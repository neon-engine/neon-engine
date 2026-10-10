#ifndef SETTINGS_SUBSCRIPTION_HPP
#define SETTINGS_SUBSCRIPTION_HPP

#include <memory>

namespace neon
{
  class SettingsStore;

  /// What SettingsStore::OnChange() hands back: the callback is called for
  /// as long as this lives, and no more once it is destroyed or ended.
  /// Movable, not copyable. One whose store is gone ends quietly.
  class SettingsSubscription final
  {
    SettingsStore *_store = nullptr;
    std::weak_ptr<int> _token;
    int _id = 0;

  public:
    SettingsSubscription() = default;

    /// For the store: `token` says whether the store is still there.
    SettingsSubscription(SettingsStore *store, const std::weak_ptr<int> &token, int id);

    SettingsSubscription(const SettingsSubscription &) = delete;

    SettingsSubscription &operator=(const SettingsSubscription &) = delete;

    SettingsSubscription(SettingsSubscription &&other) noexcept;

    SettingsSubscription &operator=(SettingsSubscription &&other) noexcept;

    ~SettingsSubscription();

    /// Whether the callback is still called.
    [[nodiscard]] bool IsActive() const;

    /// Stops the callback from being called. Nothing happens when it was
    /// ended already.
    void End();
  };
} // neon

#endif //SETTINGS_SUBSCRIPTION_HPP
