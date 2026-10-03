# Curves

This note records how the engine describes a curve with a few points: for a
rope that hangs between two bodies, for a pipe built in a recipe, and for
what is to come, a track a camera follows and a trail particles are sent
along. How a curve becomes a mesh is in [geometry.md](geometry.md#tube), and
the rope that uses one in [physics.md](physics.md#ropes).

**Current decision:** a curve is a cubic Bézier curve, or a path of them
laid end to end. The pieces are templates over what a point is, so one
piece of code carries a `glm::vec3` in the world, a `glm::vec2` on a screen,
and a float or a colour that eases over time. They are plain values in
neon-core, `neon/curves/`, that know nothing of meshes, of the physics, or
of the renderer; what draws or follows a curve asks it for points.

## The pieces

| Piece | Location | Role |
|---|---|---|
| `BezierPoint`, `BernsteinWeight` | `neon/curves/bezier.hpp` | The point of a Bézier curve of any order, as the weighted average of its control points |
| `CubicBezier<Point>` | `neon/curves/cubic-bezier.hpp` | Four points. `At`, `Tangent`, `Split`, `Flatness`, `Flatten`, `Length` |
| `BezierPath<Point>` | `neon/curves/bezier-path.hpp` | `3n + 1` points as `n` cubic pieces. `At`, `Tangent`, `Piece`, `IsSmooth`, `Flatten`, `Length` |
| `PolylineLength`, `ResamplePolyline` | `neon/curves/polyline.hpp` | A flattened curve measured, and walked in even steps of distance |
| `HangingCurve` | `neon/curves/hanging-curve.hpp` | The curve a rope of a length hangs as between two points |

## A cubic curve

Four points: the curve starts at the first, ends at the last, leaves the
first towards the second and arrives at the last from the third. The point
at `t`, from 0 to 1, is the average of the four, weighted by the Bernstein
polynomials:

    x(t) = (1 - t)³ p0 + 3 (1 - t)² t p1 + 3 (1 - t) t² p2 + t³ p3

The weights add up to 1 at every `t`, from which two things follow that the
engine leans on: the curve never leaves the hull of its four points, and
moving, turning, or scaling the points does the same to the curve, so a
curve is placed by placing its points.

`Split` cuts a curve in two at `t` by de Casteljau's construction, three
rounds of interpolation between neighbours; the points it passes through are
the control points of the two halves. Everything that needs a curve to a
tolerance is built on it: `Flatten` halves a curve until each piece is
within the tolerance of a straight line, which gives few pieces where the
curve is straight and many where it bends, and `Length` halves it until the
line between a piece's ends and the line through its control points, which
the curve lies between, are nearly as long.

## A path

A curve of more than four points is not one curve of a higher order, where
every point pulls on all of it, but cubic pieces end to end: `3n + 1`
points, every third on the curve, the two between as handles. A point then
moves its own piece alone. The parameter `u` of a path counts pieces: 0 to
1 is the first, 1 to 2 the second.

The pieces always meet. They meet without a corner when the handles on
either side of a shared point mirror each other, `p3 - p2 = p4 - p3`, which
`IsSmooth` checks and which a recipe or, later, the handles of an editor
keep.

## Even steps along a curve

The parameter of a curve does not move along it at an even speed: it is
fast where the control points are far apart. What moves along a curve at a
speed in metres, or wants points every so many centimetres, flattens the
curve and walks the line: `ResamplePolyline` hands back as many points as
asked for, each as far along from the one before as every other.

## A rope that hangs

`HangingCurve(from, to, length)` is a cubic curve between two points with
both handles lowered by the same amount, which is found by halving so that
the curve is as long as the rope. A rope whose ends are as far apart as it
is long is a straight line. This is the shape a rope comes to rest in, and
close to a catenary; it is worked out again from the two ends every frame,
so it follows them without swinging or whipping of its own.

## Open questions

- **Rope that moves by itself.** A rope that swings, whips, and lies over
  an edge is a chain of points held at a distance from each other, stepped
  with the physics. The hanging curve would then be drawn through those
  points as a path.
- **Particles on a trail.** A particle system is to take a `BezierPath`
  and send particles along it at a speed, with `ResamplePolyline` or a
  table of distances; float curves are to ease a size or a colour over a
  particle's life. Nothing reads a curve from a recipe for that yet.
- **A component that names a path.** A `Geometry` of shape `tube` holds its
  points itself. A path that several things share, a track and the camera
  on it, wants a component of its own.
- **Weights and knots.** Rational curves, which draw a true circle, and
  B-splines are left out until something needs them.
