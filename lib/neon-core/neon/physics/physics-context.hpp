#ifndef PHYSICS_CONTEXT_HPP
#define PHYSICS_CONTEXT_HPP

#include <string>
#include <vector>

#include "physics-types.hpp"

namespace neon
{
  /// What the rest of the engine sees of the physics.
  ///
  /// A game does not create bodies here. It gives an entity a RigidBody, a
  /// Trigger, or a CharacterBody, and the engine creates what belongs to it.
  /// What a game does here is ask and push: cast a ray, look for overlaps,
  /// read the events, apply an impulse to the body a component names.
  ///
  /// **Layers and masks.** A body is in layers and has a mask of layers it
  /// looks for. Two bodies collide when one of them looks for a layer the
  /// other is in. A trigger reports a body when the trigger looks for a
  /// layer the body is in. What the body looks for does not count.
  ///
  /// **Events.** The physics reports when two bodies begin and end to touch.
  /// GetStepEvents() holds what the last step reported, and is what
  /// FixedUpdate of a system reads. GetFrameEvents() holds what every step
  /// of the frame reported, and is what Update reads. A frame without a step
  /// has none. Each is handed over in the same order for the same run.
  ///
  /// Everything is called from the thread the world is updated on.
  class PhysicsContext
  {
  protected:
    ~PhysicsContext() = default;

  public:
    /// Creates a body. Returns false and says why in `error` when it cannot
    /// be created, such as a dynamic body with a mesh.
    virtual bool CreateBody(const BodyInfo &info, BodyId &body, std::string &error) = 0;

    /// Destroys a body. One that is not known is ignored.
    virtual void DestroyBody(BodyId body) = 0;

    [[nodiscard]] virtual bool HasBody(BodyId body) = 0;

    /// Number of bodies, those of characters not counted.
    [[nodiscard]] virtual std::size_t GetBodyCount() = 0;

    /// Returns false when the body is not known.
    virtual bool GetBodyState(BodyId body, BodyState &state) = 0;

    /// Puts a body somewhere at once. Nothing on the way is hit.
    virtual void SetBodyPlace(BodyId body, const glm::vec3 &position, const glm::quat &rotation) = 0;

    /// Moves a kinematic body so that it arrives after `seconds`, which is
    /// the length of the step that follows. What is on the way is pushed.
    virtual void MoveBody(BodyId body, const glm::vec3 &position, const glm::quat &rotation, double seconds) = 0;

    virtual void SetLinearVelocity(BodyId body, const glm::vec3 &velocity) = 0;

    /// Radians per second around each axis.
    virtual void SetAngularVelocity(BodyId body, const glm::vec3 &velocity) = 0;

    /// A force acts for the step that follows, and is gone after it. Apply
    /// it in every step for as long as it is meant to act, from FixedUpdate.
    virtual void AddForce(BodyId body, const glm::vec3 &force) = 0;

    virtual void AddTorque(BodyId body, const glm::vec3 &torque) = 0;

    /// An impulse changes the velocity at once, as a kick does.
    virtual void AddImpulse(BodyId body, const glm::vec3 &impulse) = 0;

    /// An impulse at a point of the world, which also turns the body.
    virtual void AddImpulseAt(BodyId body, const glm::vec3 &impulse, const glm::vec3 &point) = 0;

    virtual void SetGravity(const glm::vec3 &gravity) = 0;

    [[nodiscard]] virtual glm::vec3 GetGravity() = 0;

    /// Creates what moves and slides. See CharacterBody.
    virtual bool CreateCharacter(const CharacterInfo &info, CharacterId &character, std::string &error) = 0;

    virtual void DestroyCharacter(CharacterId character) = 0;

    [[nodiscard]] virtual std::size_t GetCharacterCount() = 0;

    /// Puts a character somewhere at once.
    virtual void SetCharacterPosition(CharacterId character, const glm::vec3 &position) = 0;

    /// Moves a character with a velocity for `seconds`. It stops at what is
    /// in its way and slides along it. Returns false when the character is
    /// not known.
    virtual bool MoveCharacter(
      CharacterId character,
      const glm::vec3 &velocity,
      double seconds,
      CharacterState &state) = 0;

    /// Advances the simulation by `seconds`. The engine calls this once per
    /// step of the world, always with the same length.
    virtual void Step(double seconds) = 0;

    /// What the last step reported.
    [[nodiscard]] virtual const std::vector<PhysicsEvent> &GetStepEvents() const = 0;

    /// What the steps of this frame reported.
    [[nodiscard]] virtual const std::vector<PhysicsEvent> &GetFrameEvents() const = 0;

    /// Forgets the events of the frame. The engine calls this once per frame,
    /// after every system was updated.
    virtual void EndFrame() = 0;

    /// Finds the first body along a ray. Returns false when there is none.
    virtual bool CastRay(const Ray &ray, const QueryFilter &filter, RayHit &hit) = 0;

    /// Finds every body that overlaps a shape that is held somewhere. `hits`
    /// holds each body once, ordered by its id.
    virtual void Overlap(
      const ShapeInfo &shape,
      const glm::vec3 &position,
      const glm::quat &rotation,
      const QueryFilter &filter,
      std::vector<OverlapHit> &hits) = 0;
  };
} // neon

#endif //PHYSICS_CONTEXT_HPP
