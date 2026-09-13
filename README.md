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

The current builder constructs typed data flow from descriptor-backed nodes and structured execution through explicit Entry, Sequence, BranchArm, Join, and nested Loop operations. Branch continuation requires an explicit Join for two live arms or an explicit one-live-arm continuation. Loop Break and Continue target the nearest active loop and persist as ControlEdges to descriptor-declared inputs. Inputs can be bound explicitly to supported literals, typed node outputs, or graph-variable references. Typed output assertions are stored per binding in canonical GraphIR and checked by a separate validation pass using scoped, deterministic constraints. GraphIR JSON v3 persists execution mode and structured ownership while preserving typed output assertions; v1/v2 graphs remain readable as Unstructured. Descriptor pin metadata remains the source of truth for pin direction, category, type, cardinality, and declared control roles.

Return construction and complete terminal validation, labeled transfers, phi/SSA behavior, GIA/GIL integration, metadata ingestion, backend compilation, and runtime/editor integration remain planned for later milestones. No implicit Join, Repeat edge, or phi/SSA behavior is provided.
