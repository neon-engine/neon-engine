#ifndef JOLT_PHYSICS_SYSTEM_HPP
#define JOLT_PHYSICS_SYSTEM_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/physics/physics-system.hpp>

namespace neon
{
  /// The physics, built on Jolt Physics.
  ///
  /// Nothing of Jolt shows in this header. What Jolt keeps is behind a
  /// pointer to a type that only the source file knows.
  ///
  /// The same calls in the same order lead to the same result, whatever
  /// number of threads the machine has. The events of a step are found on
  /// several threads, and are handed over in order once the step is done.
  // ReSharper disable once CppInconsistentNaming
  class Jolt_PhysicsSystem final : public PhysicsSystem
  {
    struct State;

    std::unique_ptr<State> _state;

    /// Sets up what Jolt shares between every physics of a process, with
    /// the first physics that is initialized.
    static void AcquireShared();

    /// Takes down what Jolt shares, with the last physics that is cleaned
    /// up.
    static void ReleaseShared();

    [[nodiscard]] static bool IsFinite(const glm::vec3 &value);

    [[nodiscard]] static std::string Name(ShapeKind kind);

    [[nodiscard]] static std::string Name(JointKind kind);

    /// Whether shapes can be on a body. Says why not in `error`.
    static bool ShapesFit(const std::vector<ShapeInfo> &shapes, BodyKind kind, bool trigger, std::string &error);

  public:
    Jolt_PhysicsSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger);

    /// Cleans up.
    ~Jolt_PhysicsSystem();

    Jolt_PhysicsSystem(const Jolt_PhysicsSystem &) = delete;

    Jolt_PhysicsSystem &operator=(const Jolt_PhysicsSystem &) = delete;

    void Initialize() override;

    void CleanUp() override;

    bool CreateBody(const BodyInfo &info, BodyId &body, std::string &error) override;

    void DestroyBody(BodyId body) override;

    bool SetShape(BodyId body, const std::vector<ShapeInfo> &shapes, std::string &error) override;

    [[nodiscard]] bool HasBody(BodyId body) override;

    [[nodiscard]] std::size_t GetBodyCount() override;

    bool GetBodyState(BodyId body, BodyState &state) override;

    void SetBodyPlace(BodyId body, const glm::vec3 &position, const glm::quat &rotation) override;

    void MoveBody(BodyId body, const glm::vec3 &position, const glm::quat &rotation, double seconds) override;

    void SetLinearVelocity(BodyId body, const glm::vec3 &velocity) override;

    void SetAngularVelocity(BodyId body, const glm::vec3 &velocity) override;

    void AddForce(BodyId body, const glm::vec3 &force) override;

    void AddTorque(BodyId body, const glm::vec3 &torque) override;

    void AddImpulse(BodyId body, const glm::vec3 &impulse) override;

    void AddImpulseAt(BodyId body, const glm::vec3 &impulse, const glm::vec3 &point) override;

    void SetGravity(const glm::vec3 &gravity) override;

    [[nodiscard]] glm::vec3 GetGravity() override;

    bool CreateCharacter(const CharacterInfo &info, CharacterId &character, std::string &error) override;

    void DestroyCharacter(CharacterId character) override;

    [[nodiscard]] std::size_t GetCharacterCount() override;

    void SetCharacterPosition(CharacterId character, const glm::vec3 &position) override;

    bool MoveCharacter(
      CharacterId character,
      const glm::vec3 &velocity,
      double seconds,
      CharacterState &state) override;

    bool CreateJoint(const JointInfo &info, JointId &joint, std::string &error) override;

    void DestroyJoint(JointId joint) override;

    [[nodiscard]] bool HasJoint(JointId joint) override;

    [[nodiscard]] std::size_t GetJointCount() override;

    void Step(double seconds) override;

    bool CastRay(const Ray &ray, const QueryFilter &filter, RayHit &hit) override;

    void Overlap(
      const ShapeInfo &shape,
      const glm::vec3 &position,
      const glm::quat &rotation,
      const QueryFilter &filter,
      std::vector<OverlapHit> &hits) override;

    bool CastShape(
      const ShapeInfo &shape,
      const glm::vec3 &from,
      const glm::quat &rotation,
      const glm::vec3 &to,
      const QueryFilter &filter,
      ShapeCastHit &hit) override;
  };
} // neon

#endif //JOLT_PHYSICS_SYSTEM_HPP
