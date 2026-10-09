#ifndef PHYSICS_CONTEXT_HPP
#define PHYSICS_CONTEXT_HPP

#include <string>
#include <vector>

#include "joint-info.hpp"
#include "joint-state.hpp"
#include "physics-types.hpp"
#include "shape-cast-hit.hpp"

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

    /// Takes a body out of what is simulated, or puts it back. Out of it,
    /// the body stays what it is, with its shapes, and nothing touches it,
    /// moves it, or finds it with a ray. It is what a body whose component
    /// is turned off does, which costs far less than destroying it and
    /// creating another. Nothing happens for a body that is not there, and
    /// for one that is where it is asked to be already.
    virtual void SetBodyInWorld(BodyId body, bool in_world) = 0;

    /// Gives a body other shapes while it lives. What the body is, its
    /// mass included, stays. Returns false and says why in `error` when the
    /// shapes cannot be made, or when the body is not known or belongs to
    /// a character. The body keeps its shapes then.
    virtual bool SetShape(BodyId body, const std::vector<ShapeInfo> &shapes, std::string &error) = 0;

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

    /// Takes a character out of what is simulated, or puts it back, as
    /// SetBodyInWorld() does a body: out of it, the character is kept as
    /// it is, and nothing touches it or finds it.
    virtual void SetCharacterInWorld(CharacterId character, bool in_world) = 0;

    [[nodiscard]] virtual std::size_t GetCharacterCount() = 0;

    /// Puts a character somewhere at once.
    virtual void SetCharacterPosition(CharacterId character, const glm::vec3 &position) = 0;

    /// Where a character is. Returns false when the character is not known.
    virtual bool GetCharacterPosition(CharacterId character, glm::vec3 &position) = 0;

    /// Moves a character with a velocity for `seconds`. It stops at what is
    /// in its way and slides along it. Returns false when the character is
    /// not known.
    virtual bool MoveCharacter(
      CharacterId character,
      const glm::vec3 &velocity,
      double seconds,
      CharacterState &state) = 0;

    /// Joins two bodies, or a body and the world. Returns false and says why
    /// in `error` when it cannot be made, such as when neither body is
    /// dynamic. The joint is gone when either of its bodies is destroyed.
    virtual bool CreateJoint(const JointInfo &info, JointId &joint, std::string &error) = 0;

    /// Takes a joint apart. One that is not known is ignored.
    virtual void DestroyJoint(JointId joint) = 0;

    /// Lets a joint go on holding, or stop: one that is off is kept as it
    /// is and holds nothing, and what it held wakes up.
    virtual void SetJointEnabled(JointId joint, bool enabled) = 0;

    /// Whether a joint still holds. It stops holding when it is destroyed,
    /// and when one of its bodies is.
    [[nodiscard]] virtual bool HasJoint(JointId joint) = 0;

    [[nodiscard]] virtual std::size_t GetJointCount() = 0;

    /// Reads where a hinge or a slider is: its angle or its position, and
    /// how fast that changes. A fixed joint and a point joint have none,
    /// and read as zeros, which is said in the log once. Returns false
    /// when the joint is not known.
    virtual bool GetJointState(JointId joint, JointState &state) = 0;

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

    /// Moves a shape, turned by `rotation`, from `from` to `to`, and finds
    /// the first body it meets on the way. Returns false when it meets none.
    /// A ray finds what a line hits, this finds what a body of that shape
    /// would hit: whether a crate fits where it is about to be put, or what
    /// a swung sword touches. The shape is any but a mesh and a plane.
    virtual bool CastShape(
      const ShapeInfo &shape,
      const glm::vec3 &from,
      const glm::quat &rotation,
      const glm::vec3 &to,
      const QueryFilter &filter,
      ShapeCastHit &hit) = 0;
  };
} // neon

#endif //PHYSICS_CONTEXT_HPP
