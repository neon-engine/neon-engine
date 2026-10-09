# Reflection

This note records how the engine knows what is inside a component, why it
needs to, and what is still open.

**Current decision:** a type is described once, next to itself, in C++. How it
is written in a scene recipe follows from the description. So will what the
editor shows and what a script can reach. The description is the engine's
own. It does not come from Flecs.

## The problem

C++ does not know the fields of a struct while a program runs. Without help,
everything that works with a component from outside has to be told its fields
by hand, each in its own way.

| Who | Needs |
|---|---|
| A scene recipe | The name of every field, to read and to write it |
| The editor | The name, what it holds, what it may hold, and what it is for |
| A script | A way to read and change a field by its name |

Before this, a component was written out three times: as a struct, as a
function that reads it, and as a function that writes it. The two functions
had to be kept in line with the struct and with each other.

## A description

```cpp
struct Spectator
{
  float move_speed = 2.5f;
  float look_speed = 0.1f;
};

inline void Describe(TypeBuilder<Spectator> &type)
{
  type.Named("Spectator", "Lets the input move an entity freely through the world");

  type.Field("move_speed", &Spectator::move_speed)
      .Describe("Units per second");

  type.Field("look_speed", &Spectator::look_speed)
      .Describe("Degrees per unit the mouse moved");
}
```

The function is called `Describe` and sits next to the type, in the same
header and the same namespace. That is how it is found.

`ComponentFormat::Of<Spectator>()` makes the format for scene recipes from it.

### What a field holds

It is deduced from the member.

| Member in C++ | Kind | In a scene recipe |
|---|---|---|
| `bool` | Boolean | `true` or `false` |
| `int` | Integer | A number without a fraction |
| `float` | Float | A number |
| `double` | Double | A number, kept with every digit a `double` holds, for what adds up over time such as seconds |
| `std::string` | String | Text |
| `glm::vec3` | Vector3 | `[x, y, z]` |
| `Color` | Color | `[red, green, blue]`, or with alpha as a fourth |
| `std::vector<std::string>` | StringList | A list of texts |
| `std::vector<float>` | FloatList | A list of numbers, such as a size `[width, height]` |
| An enum, with `Choice()` | Choice | One of its words |
| `std::uint32_t`, with `Layers()` | Layers | One layer from 1 to 32, or a list of them. `[]` is none |
| Several, with `Group()` | Group | A map |
| `FieldLength` | Length | A number of pixels, or text as a style sheet writes a length: `"50%"`, `auto`, `"calc(100% + -20px)"` |
| `std::uint8_t` | Byte | A whole number from 0 to 255 |
| `char` | Char | One character |
| `std::int16_t`, `std::uint16_t` | Short, UnsignedShort | A whole number in the range of the type |
| `std::uint32_t` | UnsignedInteger | A whole number from 0 |
| `std::int64_t`, `std::uint64_t` | Long, UnsignedLong | A whole number up to 2^53 either way, the digits a double keeps, since a recipe reads every number as one |
| `glm::vec2`, `glm::vec4` | Vector2, Vector4 | `[x, y]`, `[x, y, z, w]` |
| `glm::ivec2`, `glm::ivec3` | IntegerVector2, IntegerVector3 | `[x, y]`, `[x, y, z]` of whole numbers |
| `std::vector<int>` | IntegerList | A list of whole numbers |
| `std::vector<glm::vec3>` | Vector3List | A list of lists of three numbers |
| `glm::quat` | Quaternion | `[x, y, z, w]`. A `Transform` keeps its rotation as degrees, see below; this is for a component with a quaternion of its own |
| `glm::mat3`, `glm::mat4` | Matrix3, Matrix4 | A list of rows, each a list of three or four numbers |

A `Transform` holds its rotation as a quaternion in code and writes it as
pitch, yaw, and roll in degrees, through a `Field<glm::vec3>` with a get and
a set. That stays: degrees are what a person writes in a recipe. The
Quaternion kind is for a component that holds a turn of its own and wants
it written as the four numbers.

### Ways to describe a field

| Way | For | Example |
|---|---|---|
| `Field(name, &T::member)` | A member | `type.Field("fov", &Camera::fov)` |
| `Field(name, function)` | A member of a member | `type.Field("model", [](Renderable &r) -> std::string & { return r.render_info.model_path; })` |
| `Field<V>(name, get, set)` | What is kept as something else than it is seen as | The rotation of a `Transform`, which is seen as three numbers |
| `OptionalField(name, function, standard)` | A `std::optional` that may be left out, and comes to something that is decided elsewhere then. Seen as `standard` while it is left out, and written whenever it is given, also when it holds `standard`, so that what was written reads back as written. It has no one address, as a field with a get and a set has none | The `metallic` and `roughness` of a `Renderable`, which take the model file's factors when they are left out |
| `Choice(name, member, words)` | An enum | `type.Choice("target", &Camera::target, {"window", "texture"})` |
| `Group(name, function)` | Fields under a name of their own | The material of a `Renderable` |
| `Layers(name, member)` | One bit for each of 32 layers, seen as the numbers of the layers | `type.Layers("mask", &RigidBody::mask)` |
| `Rule(field, function)`, `Rule(function)` | What the fields have to be together. See [rules](#rules) | A capsule at least as high as it is wide |

A name need not be that of the member. `Camera::near_plane` is `near` from
outside.

### What can be said about a field

| Call | Meaning |
|---|---|
| `.Describe(text)` | What the field is for. For the editor |
| `.Required()` | A text that must not be empty |
| `.Above(n)`, `.AtLeast(n)`, `.AtMost(n)` | What a number may be. For a vector and a list of numbers, every number |
| `.Unit(text)` | What a number counts, in the plural, such as `degrees`. A message names it instead of a number |
| `.OnlyWhen(choice, words)` | The field belongs only while a choice holds one of the words, such as the radius of a sphere. See below |
| `.Count(n)` | How many numbers a list of numbers holds, when it is always as many |
| `.OneNumberForAll()` | One number may be written for a vector, as for a scale |
| `.AlwaysWritten()` | Written even when it holds its default, as the type of a light |

A number that is refused is told what was written and what was expected, as
everything else that reads a file says it:

```
physics.scene.yml:12: 'mass' of RigidBody of entity 'crate' is 0, where a number above 0 was expected
physics.scene.yml:14: 'max_slope' of CharacterBody of entity 'player' is 120, where degrees from 0 to 90 were expected
```

### What belongs to what a choice holds

```cpp
type.Choice("shape", &Collider::shape, {"box", "sphere", ...}).AlwaysWritten();

type.Field("size", &Collider::size)
    .OnlyWhen("shape", {"box"});

type.Field("radius", &Collider::radius)
    .Above(0)
    .OnlyWhen("shape", {"sphere", "capsule", "cylinder"});
```

The choice is described before the field, in the same type or group, so it is
read first. A field that does not belong is neither read nor written. A file
that gives a radius to a box is told that the name is not known, instead of
the radius being taken without a word. A field that is required and belongs
says what it is needed for: `Collider of entity 'rock' needs a 'model' for
the shape mesh`.

`TypeInfo::Belongs()` tells whether a field belongs to an object, which is
what an editor would hide a field by.

### Rules

Some of what is wrong lies in no one field. A rule says it:

```cpp
type.Rule([](const Collider &collider, const std::string &where) -> std::string
{
  if (collider.shape != ShapeKind::Capsule || collider.height >= 2.0f * collider.radius) { return {}; }

  return std::format(
    "'height' of {} is {}, which is less than twice its 'radius' of {}",
    where, collider.height, collider.radius);
});
```

A rule returns what is wrong, or nothing. It is checked once the fields of an
object were read from a document. A rule that names a field, as
`Rule("top_radius", ...)`, is reported at the line of that field. What breaks
a rule is kept as it was read, since no one field is to blame for it.

A rule is also how a limit that depends on a choice is said. The radii of a
tapered cylinder may be 0, and those of a tapered capsule may not, so the
field says `AtLeast(0)` and a rule says the rest.

### What is left out

A member that is not described is not seen from outside. That is how what the
engine keeps for itself stays out of a scene recipe: the id the renderer knows
an entity by, and where a `Transform` puts an entity in the world.

### Values as text

An inspector shows a value in a box that is typed into, and a script may know
a field by its name alone. `FormatField` and `ParseField` of
`neon/reflection/field-text.hpp` turn the value of every kind into text and
back.

| Kind | As text |
|---|---|
| Boolean | `true`, `false` |
| Integer, Float | `12`, `0.5` |
| Double | `1234.56789012345`, with every digit. A file may write it as `2.0d` to say so; a plain number is read all the same, and the engine never writes the suffix |
| String, Choice | The text itself |
| Vector3 | `1 2 3` |
| Color | In the notation of CSS: `#ff8000`, `#ff800080`. `rgb()` and `rgba()` are read as well |
| StringList | `one, two` |
| Length | `12px`, `50%`, `auto` |
| FloatList | `1 2 3 4` |
| Layers | `1 3` |
| Byte, Short, UnsignedShort, UnsignedInteger, Long, UnsignedLong | `12`, refused outside the range of the type |
| Char | `q` |
| Vector2, Vector4 | `1 2`, `1 2 3 4` |
| IntegerVector2, IntegerVector3 | `1 2`, `1 2 3` |
| IntegerList | `1 2 3` |
| Vector3List | `1 2 3, 4 5 6`, the vectors apart by commas |
| Quaternion | `0 0 0 1`, as x y z w |
| Matrix3, Matrix4 | `1 0 0, 0 1 0, 0 0 1`, the rows apart by commas |

## Reaching a field by name

```cpp
const auto camera = neon::TypeInfo::Of<neon::Camera>();

neon::FieldValue value;
std::string error;

if (neon::GetField(store, entity, camera, "fov", value, error))
{
  const float fov = std::get<float>(value);
}

if (!neon::SetField(store, entity, camera, "fov", 90.0f, error))
{
  // error says why, such as: 'fov' of Camera takes a number
}
```

A field of a group is named with a dot: `material.color`.

`SetField` checks a value before it takes it: its kind, the words of a choice,
and what a number may be. A value that is refused leaves the component as it
was.

This is what the inspector of the editor and the bindings for Lua are meant to
be built on. Neither exists yet.

## What exists today

| Piece | Location | Role |
|---|---|---|
| `TypeInfo`, `FieldInfo` | `neon/reflection/type-info.hpp` | The description of a type and of a field |
| `TypeBuilder` | `neon/reflection/type-builder.hpp` | Writes a description |
| `FieldValue`, `FieldKind` | `neon/reflection/field-value.hpp` | The value of a field, whatever its type is in C++ |
| `ReadFields`, `WriteFields` | `neon/reflection/field-documents.hpp` | Reads and writes described fields from and to a document |
| `FormatField`, `ParseField` | `neon/reflection/field-text.hpp` | The value of a field as text, and back |
| `GetField`, `SetField` | `neon/reflection/entity-fields.hpp` | Reaches a field of a component of an entity by name |
| `ComponentFormat::Of<T>()` | `neon/world-system/ecs/scene-file/` | The format of a described component |

All of it is in neon-core and depends on no library.

`Transform`, `Renderable`, `Camera`, `Light`, `Spectator`, `SoundSource`,
`SoundListener`, `RigidBody`, `Trigger`, `CharacterBody`, `Collider`, and
`Joint` are described. Every component of the engine is.
The functions that read and wrote them by hand are removed. For the first
seven that took 232 lines out and put 149 in. For the four of the physics it
took 409 out and put 238 in, the descriptions included.

What is left of `physics-component-formats.cpp` is what sets the components of
the physics apart: a world without physics is told that a component needs it.

A format can still be written by hand, with
`ComponentFormat::Of<T>(name, read, write)`, for what a description cannot
say.

## Decisions

| Decision | Reason |
|---|---|
| The engine's own, not that of Flecs | One description serves scene recipes, the editor, and scripts, whatever keeps the entities. Flecs is started without its addons, and stays replaceable |
| Written in C++ by hand, not generated | No step in the build and no tool to keep. C++ gains reflection of its own with C++26, which no compiler of the toolchain offers yet |
| Next to the type | It is changed when the type is changed, by whoever changes it |
| A field is reached through functions, not through its place in memory | It works for a member of a member and for a value that is kept as something else. It does not depend on how the compiler lays out a struct |
| A value is one of eleven types | Code that works with any component needs a closed set to handle. A new kind is added in one place |
| A scene recipe is the same as before | Descriptions replace how components are read and written, not what is read and written |
| A refused number is told as the rest of the engine tells it | `is 0, where a number above 0 was expected` says what was written. `has to be above 0`, which descriptions said at first, did not, and the physics said it the first way |
| Layers are a kind of their own | One layer may be written without a list, and a message names layers. A list of numbers with limits could say neither |
| What breaks a rule is kept | No one field is to blame, so none is set back to its default |

One message changed. A renderable that lacks a model or a shader is told which
of the two it lacks. Before, it was told that it needs both.

When the physics was described, the messages of every limit changed to what
the physics said: `'pitch' of SoundSource of entity 'hum' is 0, where a number
above 0 was expected`, where it was `has to be above 0`. The messages of the
physics did not change.

## How it was checked

| Check | Result |
|---|---|
| Tests of how the components of the engine are read and written, written before anything was changed | 30 passed before. 30 of 31 pass unchanged after. The one that changed is the message above |
| The demo scene | The same image as before, byte for byte |
| Tests of descriptions, of reading and writing fields, and of reaching them by name | 69 pass |
| Tests of the document format for YAML and of scenes in files | 98 pass. They were checks outside of the repository before |
| All tests | 1077 pass |

When the physics was described:

| Check | Result |
|---|---|
| Tests of how the components of the physics are read and written, written against the code by hand | 43 pass, without a change |
| The physics demo, 120 steps without a window | The same image as before, byte for byte |
| Every scene of the runtime | Loads without a message |
| Tests of limits on vectors, units, fields that belong to a choice, rules, and layers | 25 new, and pass |
| All tests | 3674 pass |

## Open questions

- References to other entities, such as a door to its switch. A kind of its
  own, written as a path.
- Lists of groups. Lists of texts and of numbers exist.
- A quaternion as a kind, once a rotation is kept as one.
- What else the editor wants to know: a step for a slider, a unit, whether a
  text is a path to a file and of which kind.
- Components that are declared by a script have no C++ type. A description
  can be built without one, since a field is reached through functions. How a
  script says what it declares is open.
- Whether `EntityStore::Register` should take the name from the description,
  so that it is written once.
- Rules are checked when a document is read. `SetField` checks a field on
  its own, so it can set a radius that leaves a capsule lower than it is
  wide.
- Whether `ComponentFormat::Of<T>()` should tell every world that lacks a
  component so, as the physics does now for its own.
