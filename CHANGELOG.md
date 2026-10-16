# Changelog

## Modernized Edition

The project was restored and substantially modernized in 2026:

- Upgraded to Visual Studio 2022, x64, and C++20.
- Replaced manual tagged unions and reference counting with `std::variant` and
  tracing mark-and-sweep garbage collection.
- Replaced packed raw bytecode with typed instructions.
- Replaced stack-encoded frame metadata with explicit call frames.
- Added externally rooted native values and safe cyclic arrays.
- Added source-located compile and runtime diagnostics.
- Added booleans, scoped locals, numeric `for`, `repeat/until`, and `do/end`.
- Added short-circuit logic, forward calls, and multidimensional indexing.
- Replaced untyped native callbacks with `std::function` and `std::span`.
- Added a reusable static library, CLI, automated tests, CI, examples, and
  documentation.

## Graduate Project

Solstice Script began in 2012 as a graduate-school project inspired by GDC
talks about Lua in game development. The original implementation became
compatible enough to parse and run a Level Design classmate's Lua coursework.
