#ifndef FAKE_PHYSICS_CONTEXT_HPP
#define FAKE_PHYSICS_CONTEXT_HPP

#include <iterator>
#include <map>
#include <string>
#include <vector>

#include <neon/physics/physics-context.hpp>

namespace neon::testing
{
  /// A physics that is as simple as one can be, for testing what works with
  /// the physics without a backend.
  ///
  /// Nothing collides. A step moves every dynamic body by its velocity and
  /// lets it fall, and puts every kinematic body where it was moved to. A
  /// character moves by the velocity it is moved with. Everything that was
  /// asked of it is written down, so that a test can look at it.
  class FakePhysicsContext : public PhysicsContext
  {
  public:
    struct Body
    {
      BodyInfo info;
      BodyState state;

      /// Where a kinematic body arrives with the next step.
      bool moving = false;
      glm::vec3 target_position{0.0f};
      glm::quat target_rotation{1.0f, 0.0f, 0.0f, 0.0f};

      glm::vec3 force{0.0f};
    };

    struct Character
    {
      CharacterInfo info;
      CharacterState state;
    };

    struct Reshape
    {
      BodyId body = No_Body;
      std::vector<ShapeInfo> shapes;
    };

    struct Move
    {
      BodyId body = No_Body;
      glm::vec3 position{0.0f};
      glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
      double seconds = 0.0;
    };

    struct CharacterMove
    {
      CharacterId character = No_Character;
      glm::vec3 velocity{0.0f};
      double seconds = 0.0;
    };

    /// When not empty, nothing can be created, and this is the reason.
    std::string fail_with;

    /// What is alive.
    std::map<BodyId, Body> bodies;
    std::map<CharacterId, Character> characters;
    std::map<JointId, JointInfo> joints;

    /// What a joint reads as. A joint that is not here reads as zeros.
    std::map<JointId, JointState> joint_states;

    /// What was asked, in the order it was asked in.
    std::vector<BodyInfo> created;
    std::vector<BodyId> destroyed;
    std::vector<Reshape> reshaped;
    std::vector<CharacterId> destroyed_characters;
    std::vector<JointInfo> created_joints;
    std::vector<JointId> destroyed_joints;
    std::vector<Move> placed;
    std::vector<Move> moved;
    std::vector<CharacterMove> character_moves;
    std::vector<double> steps;
    int create_attempts = 0;
    int end_frame_count = 0;

    /// What the next step reports.
    std::vector<PhysicsEvent> next_events;

    /// What is said about the ground of every character that is moved.
    bool on_floor = false;

    glm::vec3 gravity{0.0f, -9.81f, 0.0f};

    bool CreateBody(const BodyInfo &info, BodyId &body, std::string &error) override
    {
      create_attempts++;
      if (!fail_with.empty())
      {
        error = fail_with;
        return false;
      }

      body = _next_body++;
      created.push_back(info);

      Body record;
      record.info = info;
      record.state.position = info.position;
      record.state.rotation = info.rotation;
      record.state.linear_velocity = info.linear_velocity;
      record.state.angular_velocity = info.angular_velocity;
      record.state.active = info.kind == BodyKind::Dynamic;
      bodies[body] = record;
      return true;
    }

    void DestroyBody(const BodyId body) override
    {
      if (bodies.erase(body) > 0) { destroyed.push_back(body); }

      // a joint goes with either of its bodies
      for (auto it = joints.begin(); it != joints.end();)
      {
        it = it->second.body == body || it->second.other == body ? joints.erase(it) : std::next(it);
      }
    }

    bool CreateJoint(const JointInfo &info, JointId &joint, std::string &error) override
    {
      if (!fail_with.empty())
      {
        error = fail_with;
        return false;
      }

      joint = _next_joint++;
      joints[joint] = info;
      created_joints.push_back(info);
      return true;
    }

    void DestroyJoint(const JointId joint) override
    {
      if (joints.erase(joint) > 0) { destroyed_joints.push_back(joint); }
    }

    bool HasJoint(const JointId joint) override { return joints.contains(joint); }

    std::size_t GetJointCount() override { return joints.size(); }

    bool GetJointState(const JointId joint, JointState &state) override
    {
      if (!joints.contains(joint)) { return false; }

      const auto it = joint_states.find(joint);
      state = it == joint_states.end() ? JointState{} : it->second;
      return true;
    }

    bool SetShape(const BodyId body, const std::vector<ShapeInfo> &shapes, std::string &error) override
    {
      const auto it = bodies.find(body);
      if (it == bodies.end())
      {
        error = "the body is not known";
        return false;
      }

      if (!fail_with.empty())
      {
        error = fail_with;
        return false;
      }

      it->second.info.shapes = shapes;
      reshaped.push_back({body, shapes});
      return true;
    }

    bool HasBody(const BodyId body) override { return bodies.contains(body); }

    std::size_t GetBodyCount() override { return bodies.size(); }

    bool GetBodyState(const BodyId body, BodyState &state) override
    {
      const auto it = bodies.find(body);
      if (it == bodies.end()) { return false; }

      state = it->second.state;
      return true;
    }

    void SetBodyPlace(const BodyId body, const glm::vec3 &position, const glm::quat &rotation) override
    {
      placed.push_back({body, position, rotation, 0.0});

      const auto it = bodies.find(body);
      if (it == bodies.end()) { return; }

      it->second.state.position = position;
      it->second.state.rotation = rotation;
      it->second.moving = false;
    }

    void MoveBody(
      const BodyId body,
      const glm::vec3 &position,
      const glm::quat &rotation,
      const double seconds) override
    {
      moved.push_back({body, position, rotation, seconds});

      const auto it = bodies.find(body);
      if (it == bodies.end()) { return; }

      it->second.moving = true;
      it->second.target_position = position;
      it->second.target_rotation = rotation;
    }

    void SetLinearVelocity(const BodyId body, const glm::vec3 &velocity) override
    {
      if (const auto it = bodies.find(body); it != bodies.end()) { it->second.state.linear_velocity = velocity; }
    }

    void SetAngularVelocity(const BodyId body, const glm::vec3 &velocity) override
    {
      if (const auto it = bodies.find(body); it != bodies.end()) { it->second.state.angular_velocity = velocity; }
    }

    void AddForce(const BodyId body, const glm::vec3 &force) override
    {
      if (const auto it = bodies.find(body); it != bodies.end()) { it->second.force += force; }
    }

    void AddTorque(BodyId, const glm::vec3 &) override {}

    void AddImpulse(const BodyId body, const glm::vec3 &impulse) override
    {
      const auto it = bodies.find(body);
      if (it == bodies.end()) { return; }

      it->second.state.linear_velocity += impulse / it->second.info.mass;
    }

    void AddImpulseAt(const BodyId body, const glm::vec3 &impulse, const glm::vec3 &) override
    {
      AddImpulse(body, impulse);
    }

    void SetGravity(const glm::vec3 &value) override { gravity = value; }

    glm::vec3 GetGravity() override { return gravity; }

    bool CreateCharacter(const CharacterInfo &info, CharacterId &character, std::string &error) override
    {
      create_attempts++;
      if (!fail_with.empty())
      {
        error = fail_with;
        return false;
      }

      character = _next_character++;

      Character record;
      record.info = info;
      record.state.position = info.position;
      characters[character] = record;
      return true;
    }

    void DestroyCharacter(const CharacterId character) override
    {
      if (characters.erase(character) > 0) { destroyed_characters.push_back(character); }
    }

    std::size_t GetCharacterCount() override { return characters.size(); }

    void SetCharacterPosition(const CharacterId character, const glm::vec3 &position) override
    {
      if (const auto it = characters.find(character); it != characters.end())
      {
        it->second.state.position = position;
      }
    }

    bool MoveCharacter(
      const CharacterId character,
      const glm::vec3 &velocity,
      const double seconds,
      CharacterState &state) override
    {
      character_moves.push_back({character, velocity, seconds});

      const auto it = characters.find(character);
      if (it == characters.end()) { return false; }

      auto &own = it->second.state;
      own.position += velocity * static_cast<float>(seconds);
      own.velocity = velocity;
      own.on_floor = on_floor;

      state = own;
      return true;
    }

    void Step(const double seconds) override
    {
      steps.push_back(seconds);
      const auto time = static_cast<float>(seconds);

      for (auto &[id, body] : bodies)
      {
        auto &state = body.state;

        if (body.info.kind == BodyKind::Kinematic && body.moving)
        {
          state.position = body.target_position;
          state.rotation = body.target_rotation;
          body.moving = false;
        }

        if (body.info.kind == BodyKind::Dynamic && !body.info.trigger)
        {
          state.linear_velocity += (gravity * body.info.gravity_scale + body.force / body.info.mass) * time;
          state.position += state.linear_velocity * time;
        }

        body.force = glm::vec3{0.0f};
      }

      _step_events = next_events;
      _frame_events.insert(_frame_events.end(), next_events.begin(), next_events.end());
      next_events.clear();
    }

    const std::vector<PhysicsEvent> &GetStepEvents() const override { return _step_events; }

    const std::vector<PhysicsEvent> &GetFrameEvents() const override { return _frame_events; }

    void EndFrame() override
    {
      end_frame_count++;
      _frame_events.clear();
    }

    /// Hits nothing.
    bool CastRay(const Ray &, const QueryFilter &, RayHit &) override { return false; }

    void Overlap(
      const ShapeInfo &,
      const glm::vec3 &,
      const glm::quat &,
      const QueryFilter &,
      std::vector<OverlapHit> &hits) override
    {
      hits.clear();
    }

    /// Meets nothing.
    bool CastShape(
      const ShapeInfo &,
      const glm::vec3 &,
      const glm::quat &,
      const glm::vec3 &,
      const QueryFilter &,
      ShapeCastHit &) override
    {
      return false;
    }

    /// The body that was created for an entity, or nullptr.
    [[nodiscard]] Body *BodyOf(const Entity entity)
    {
      for (auto &[id, body] : bodies)
      {
        if (body.info.entity == entity) { return &body; }
      }
      return nullptr;
    }

    [[nodiscard]] Character *CharacterOf(const Entity entity)
    {
      for (auto &[id, character] : characters)
      {
        if (character.info.entity == entity) { return &character; }
      }
      return nullptr;
    }

  private:
    BodyId _next_body = 1;
    CharacterId _next_character = 1;
    JointId _next_joint = 1;
    std::vector<PhysicsEvent> _step_events;
    std::vector<PhysicsEvent> _frame_events;
  };
} // neon::testing

#endif //FAKE_PHYSICS_CONTEXT_HPP
