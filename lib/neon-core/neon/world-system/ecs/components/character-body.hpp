#ifndef CHARACTER_BODY_HPP
#define CHARACTER_BODY_HPP

#include <cstdint>

#include <glm/glm.hpp>

#include <neon/physics/physics-types.hpp>
#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// Makes an entity something that is moved by code, stops at what is in
  /// its way, and slides along it. It pushes dynamic bodies and is not
  /// pushed by them. Its shape is what the Collider of the entity says,
  /// which is a capsule for most.
  ///
  /// A game writes the velocity. In every step of the world the engine
  /// moves the entity by it, and writes where that led to into the
  /// Transform. The shape stays upright whatever the rotation of the
  /// Transform is, which is free for the game to turn.
  struct CharacterBody
  {
    /// Units per second the entity is meant to move with. It is kept, so an
    /// entity that is stopped by a wall moves on once the wall is gone.
    glm::vec3 velocity{0.0f};

    /// What falling has added to the velocity. The engine lets it grow
    /// while the entity is in the air, and ends it on the ground. A game
    /// writes it to jump.
    glm::vec3 fall_velocity{0.0f};

    /// 0 for an entity that does not fall, such as one that flies.
    float gravity_scale = 1.0f;

    /// Steeper ground than this, in degrees, is a wall.
    float max_slope = 45.0f;

    /// A step up to this height is walked up as if it were a ramp.
    float step_height = 0.25f;

    /// What dynamic bodies feel of the entity, and the most force it pushes
    /// them with.
    float mass = 70.0f;
    float push_strength = 100.0f;

    std::uint32_t layers = 1;
    std::uint32_t mask = 1;

    /// What the last step led to. Written by the engine.
    bool on_floor = false;
    bool on_wall = false;
    bool on_ceiling = false;
    glm::vec3 floor_normal{0.0f, 1.0f, 0.0f};
    Entity floor = No_Entity;

    /// What the entity moved with in the last step, which is less than what
    /// was asked for when something was in the way.
    glm::vec3 real_velocity{0.0f};

    /// What the physics knows the entity as. Filled in by the engine.
    CharacterId character = No_Character;

    /// Set by the engine when the character could not be created. It is not
    /// tried again.
    bool failed = false;
  };
} // neon

#endif //CHARACTER_BODY_HPP
