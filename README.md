# Miliastra++
A C++ library for building node graphs for Miliastra Wonderland.

## Progress

- Phase 1 — completed: the original graph, node, pin, link, and compiler prototype.
- Phase 2 — completed through Milestone 8: backend-neutral GraphIR, descriptors, validation, JSON support, adapters, and representative fixtures.
- Phase 3 Milestone 1 — completed: GraphBuilder ownership, descriptor-backed nodes, deterministic IDs, safe NodeHandles, and consuming finalization.
- Phase 3 Milestone 2 — completed: typed outputs, explicit data-flow bindings, literals, graph variables, and list/cardinality checks.
- Phase 3 Milestone 3 — completed: deterministic generic unification, persisted typed-output use constraints, and final typed validation.
- Phase 4 Milestone 1 — completed: persisted execution mode, entry and region ownership, trusted control-role descriptors, structural validation, and GraphIR JSON v3 with v1/v2 read compatibility.
- Phase 4 Milestone 2 — completed: structured Entry and Sequence construction, binary BranchArm scopes, explicit Join and one-live-arm continuation, branch reachability, and branch data dominance/provenance validation.
- Phase 4 Milestone 3 — completed: nested LoopBody construction, Conditional and Unconditional loops, explicit nearest-loop Break/Continue transfers, Repeat backedges, loop Exit continuation, loop-aware reachability/cycle validation, and loop-aware data dominance/provenance.
- Phase 4 Milestone 4 — completed: execution-only Return construction, terminal path validation, complete structured reachability and per-entry data provenance validation, with GraphIR JSON v3 and legacy v1/v2 Unstructured behavior preserved.
- Phase 5 Milestone 1 — completed: source-independent descriptor catalogue identity, opaque external node identity, logical provenance, catalogue binding, deterministic catalogue-local descriptor-ID allocation, and compatibility diagnostics.
- Phase 5 Milestone 2 — completed: normalized backend-neutral descriptor records, current descriptor semantic validation, deterministic SHA-256 content identity, immutable in-memory catalogues, canonical ordering, provenance-preserving diagnostics, and P5.1 allocation integration.
- Phase 5 Milestone 3 — completed: bounded ingestion of pinned upstream-derived Genshin client boolean-filter metadata through an isolated source adapter, P5.2 normalized records, and deterministic catalogue integration.
- Phase 5 Milestone 4 — completed: bounded reflected descriptor-family specialization for the pinned Genshin client boolean-filter source, producing deterministic concrete backend-neutral normalized variants through the existing catalogue boundary.

- Phase 5 Milestone 5 — completed: deterministic descriptor catalogue snapshots and validated NodeDescriptorRegistry materialization with catalogue-local identity preservation.
- Post-Phase 5 infrastructure — completed: Miliastra++ is built as a static C++ core library, all Phase test executables link against it, and the placeholder Main.cpp application entry point has been removed.
- Phase 6 Milestone 1 — completed: validated GIA target/configuration contracts, opaque-identity backend mapping packages, exact catalogue compatibility enforcement, and immutable export context; GraphIR lowering, enum/source coverage, protobuf encoding, byte export, CLI, and GIL integration remain out of scope.

The first-fixture coverage prerequisite adds the bounded client bool_filter result-node descriptor, backend-neutral enum identity/literal semantics, catalogue semantic schema 2, snapshot format 2 with legacy v1 reading, and additive GraphIR v3 Enum JSON forms. P6.2 GraphIR-to-GIA lowering, protobuf, GIA bytes, CLI, and GIL remain out of scope.

- Phase 6 Milestone 2 — completed: validated GraphIR can be lowered through GiaExportContext into an owned deterministic protobuf-independent semantic backend model with exact descriptor/catalogue/external-identity/mapping resolution and first-target default/literal materialization. Graph-variable target support, final GIA pin coordinates, effective i2/remaps, final connection coordinates, layout, protobuf, bytes, file output, CLI, and GIL remain unimplemented.

The current builder constructs typed data flow from descriptor-backed nodes and structured execution through explicit Entry, Sequence, BranchArm, Join, nested Loop, and Return operations. Branch continuation requires an explicit Join for two live arms or an explicit one-live-arm continuation. Loop Break and Continue target the nearest active loop and persist as ControlEdges to descriptor-declared inputs. Return is execution-only: it consumes one explicit Flow predecessor and terminates that path without producing a continuation. Natural path completion remains valid, while root-only structured Entries are rejected. Inputs can be bound explicitly to supported literals, typed node outputs, or graph-variable references. Typed output assertions are stored per binding in canonical GraphIR and checked by a separate validation pass using scoped, deterministic constraints. GraphIR JSON v3 persists execution mode and structured ownership while preserving typed output assertions; v1/v2 graphs remain readable as Unstructured. Descriptor pin metadata remains the source of truth for pin direction, category, type, cardinality, and declared control roles.

Labeled transfers, phi/SSA behavior, GIA/GIL integration, broader vendor metadata ingestion, backend compilation, and runtime/editor integration remain out of scope. No implicit Join, Repeat edge, or phi/SSA behavior is provided.
