# Miliastra++
A C++ library for building node graphs for Miliastra Wonderland.

## Progress

- Phase 1 — completed: the original graph, node, pin, link, and compiler prototype.
- Phase 2 — completed through Milestone 8: backend-neutral GraphIR, descriptors, validation, JSON support, adapters, and representative fixtures.
- Phase 3 Milestone 1 — completed: GraphBuilder ownership, descriptor-backed nodes, deterministic IDs, safe NodeHandles, and consuming finalization.
- Phase 3 Milestone 2 — completed: typed outputs, explicit data-flow bindings, literals, graph variables, and list/cardinality checks.
- Phase 3 Milestone 3 — completed: deterministic generic unification, persisted typed-output use constraints, and final typed validation.

The current builder can construct typed data flow from descriptor-backed nodes. Inputs can be bound explicitly to supported literals, typed node outputs, or graph-variable references. Typed output assertions are stored per binding in canonical GraphIR and checked by a separate validation pass using scoped, deterministic constraints. GraphIR JSON version 2 persists those assertions while remaining able to read version 1 graphs. Descriptor pin metadata remains the source of truth for pin direction, category, type, and cardinality.

Structured control flow, GIA/GIL integration, metadata ingestion, and runtime/editor integration are planned for later milestones.
