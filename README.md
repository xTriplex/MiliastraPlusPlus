# Miliastra++

Miliastra++ is a C++23 static library for constructing, validating, and lowering typed node graphs through a staged GIA export pipeline.

## What is Miliastra++?

The project provides a backend-neutral graph model for Miliastra Wonderland-style node graphs. `GraphIR` is the canonical representation; descriptor registries and catalogues provide validated node metadata; `GraphBuilder` constructs typed graphs; and deterministic validation and persistence keep graph behavior reproducible.

The core remains a static library. Test executables are separate targets, and the repository keeps vendored Protobuf and Abseil dependencies local for offline builds.

## Highlights

- Descriptor-backed nodes with catalogue identity, provenance, deterministic allocation, specialization, and snapshot materialization.
- Typed data flow with literals, graph variables, lists, cardinality checks, and owner-scoped generic validation.
- Structured execution with entries, sequences, branches, joins, nested loops, `Break`, `Continue`, and valueless `Return`.
- Deterministic GraphIR JSON v3 with v1/v2 read compatibility.
- A staged GIA backend that separates semantic lowering, target resolution/layout, and protobuf encoding.
- Dedicated test-access seams that keep failure injection and protobuf inspection out of the production archive.

## Current Status

| Area | Status |
| --- | --- |
| Phase 1 | Complete: original graph, node, pin, link, and compiler foundation |
| Phase 2 | Complete through Milestone 8: GraphIR, descriptors, validation, JSON, adapters, and fixtures |
| Phase 3 | Complete through Milestone 3: typed GraphBuilder, bindings, variables, and generic validation |
| Phase 4 | Complete through Milestone 4: structured control-flow construction and validation |
| Phase 5 | Complete through Milestone 5: catalogue ingestion, specialization, snapshots, and registry materialization |
| Phase 6 | Complete through Milestone 5: deterministic resolved-model protobuf encoding, fresh validation, and bounded in-memory `.gia` export |

The current implementation boundary is Phase 6 Milestone 5. The bounded representative Client `bool_filter` / `Beyond` path now produces owned in-memory `.gia` bytes. Filesystem and application integration remain deferred.

## Phase 6 / GIA Export

The implemented P6.4 production path is:

```text
validated GraphIR
    -> semantic GIA backend lowering
    -> resolved target-semantic GIA model
    -> deterministic bare Root protobuf bytes
    -> fresh protobuf decode
    -> structural validation and model/connection comparison
    -> deterministic owned in-memory `.gia` framing
```

Phase 6 Milestone 1 establishes target, configuration, context, and mapping contracts. The first-fixture bool-filter coverage prerequisite freezes the representative `ClientBooleanFilter`/`Beyond` target. Milestones 2 and 3 own semantic lowering and resolved pin, connection, and layout decisions. Milestone 4 mechanically encodes the resolved model into the pinned protobuf schema and validates a fresh decode.

Phase 6 Milestone 5 wraps that validated bare `Root` protobuf payload in the deterministic GIA header/tail and exposes the result through an owning in-memory export facade for the bounded representative fixture. Filesystem output, filename policy, CLI/GIL integration, editor/runtime integration, broader target profiles, and broader pin-family support remain deferred.

## Building / Project Generation

Run from the repository root with the pinned local dependencies available:

```bat
GenerateProjectFiles.bat
```

The script regenerates the existing Visual Studio 2022 solution and project tree. It places `Miliastra++.sln` at the repository root and keeps generated files under the existing `Miliastra++` CMake binary tree.

Build a configuration and run the complete configured test suite with:

```bat
cmake --build Miliastra++ --config Debug -- /m:4
ctest --test-dir Miliastra++ -C Debug --output-on-failure

cmake --build Miliastra++ --config Release -- /m:4
ctest --test-dir Miliastra++ -C Release --output-on-failure

cmake --build Miliastra++ --config Dist -- /m:4
ctest --test-dir Miliastra++ -C Dist --output-on-failure
```

The configured suite currently contains 27 tests. Builds use the vendored Protobuf and Abseil trees; no package-manager or network dependency is required by the project configuration.

## Project Structure

```text
Miliastra++/
├─ Source/Public/       public project headers
├─ Source/Private/      project implementation sources
└─ Vendor/              pinned dependencies and GIA schema
Tests/                  milestone test executables
CMakeLists.txt          offline targets and build configuration
GenerateProjectFiles.bat
```

The generated solution groups dependency projects under `Dependencies` and project tests under `Tests`; the main `Miliastra++` static-library target remains at the solution root.

## Scope / Deferred Work

The current bounded implementation provides owned in-memory `.gia` bytes for the representative Client `bool_filter` / `Beyond` fixture.

Filesystem `.gia` writing, filename policy, CLI/GIL integration, broader target support, editor/runtime integration, and automatic editor loading remain later work. This milestone does not claim filesystem export or broad GIA target support.
