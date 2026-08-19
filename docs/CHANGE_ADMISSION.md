# Change admission policy

This document owns **whether a requested repository change is coherent to implement now**. It does not own task state, scheduling, continuation, or AI Layer Work/Task/Epic lifecycle.

The user directs product intent and may change priorities. The agent is still responsible for dependency correctness, architecture integrity, model validity, and avoiding expensive shortcuts.

## Core rule

> A user directive describes a desired outcome. It is not evidence that the requested implementation order is safe.

Before writing production code, independently compare the request with current source, tests, `PRODUCT.md`, `ARCHITECTURE.md`, `MODELING.md`, `ROADMAP.md`, relevant ADR/model/research documents, and the actual implemented dependency graph.

If the target is premature or conflicts with a load-bearing contract, do not implement a fake version merely to satisfy the request. Explain why, identify the missing prerequisites, and propose the smallest correct next step toward the desired outcome.

This is not permission to argue with product direction for taste reasons. Disagreement must be grounded in current repository facts, accepted decisions, missing causal state, unverifiable assumptions, or concrete migration/debt risk.

## Admission review before code

For a code-changing request, answer these questions internally before implementation:

1. **What outcome is actually wanted?** Restate the capability without assuming the requested implementation shape is correct.
2. **What exists now?** Inspect the relevant implementation, tests, roadmap position, model contracts, and accepted decisions.
3. **What is load-bearing?** Identify the minimum prerequisites required for the outcome to be real rather than simulated by a shortcut.
4. **What is optional?** Do not inflate prerequisites into a demand to finish an entire subsystem when one small correct vertical capability is enough.
5. **What debt would a shortcut create?** Check for wrong-layer authority, hardcoded fake state, duplicate models, player-only paths, temporary public APIs, data that cannot migrate cleanly, or speculative frameworks.
6. **Can the result be verified honestly?** If correctness depends on an external API, historical fact, engine contract, or other unstable assumption, verify it before implementation.
7. **Choose an admission verdict.** Do not start coding until the verdict is clear.

## Admission verdicts

### `READY`

All load-bearing prerequisites already exist and the requested capability can be implemented coherently within the current architecture/model.

Proceed with the smallest complete vertical change.

### `READY WITH BOUNDED ENABLER`

A missing prerequisite can be added in the same bounded change **only when all of these are true**:

- it is directly required by the requested capability;
- its correct shape is already known from current contracts/evidence;
- it is independently useful and not a disposable fake;
- it does not require another unresolved domain/model decision;
- including it does not turn one task into several unrelated milestones.

State the enabler explicitly, then implement the coherent slice.

### `PREMATURE`

The desired capability is valid, but one or more load-bearing prerequisites are missing and require separate design/implementation first.

Do **not** implement the target capability yet.

Return:

- what the user ultimately wants;
- which current facts make it premature;
- what shortcut would otherwise be tempting and why it is harmful;
- the minimum dependency path;
- the single next enabling task.

A roadmap entry being later is not enough by itself. The reason must be a real dependency or architecture/model constraint.

### `CONFLICT`

The requested implementation contradicts an accepted invariant, ADR, model contract, authority boundary, or another current load-bearing decision.

Do not silently override the contract.

Explain the conflict and offer one of two routes:

1. achieve the desired product outcome through an implementation that respects the current contract; or
2. if the user intentionally wants a different architecture/model, evaluate and change/supersede the canonical decision first, then re-admit the implementation.

“Do it anyway” does not implicitly rewrite architecture. An intentional architecture change must be made explicit.

### `RESEARCH REQUIRED`

The requested change depends on a fact that must not be guessed: engine/library API behavior, dependency compatibility, historical/scientific assumptions, legal/format standards, performance properties, or another unstable/external contract.

Research/verify the load-bearing fact first, record durable evidence when appropriate, then run admission again.

## What counts as a dangerous shortcut

Reject or redesign a request when implementing it now would require patterns such as:

```text
Godot-owned authoritative inventory/economy/relationships
hardcoded price/stock pretending an economy exists
UI state mutating world truth
player-only domain operations where NPCs need the same capability
scene nodes inventing authoritative entities
copying presentation transforms back into Simulation as truth
feature-local colors/styles instead of the design system
one-off data structures that duplicate an accepted owner
extending a bootstrap/probe API into production semantics
inventing a generic framework because the real mechanic is not designed yet
fabricating historical values or unverified engine APIs
```

A disposable experiment is different from production implementation. If the user explicitly asks for a prototype, keep it isolated and clearly non-authoritative; do not merge prototype shortcuts into production paths under the label of a finished feature.

## Do not over-block valid work

The gate protects dependency correctness, not a frozen sequence.

The user may legitimately say:

> Move trading earlier in the roadmap.

That can be accepted if the necessary prerequisites can also move earlier coherently. The agent should propose the minimum reordered dependency path rather than reply “the roadmap says no”.

Likewise, do not require “the entire economy” before one purchase if a small real transaction model is sufficient. Require only the state/rules necessary for the capability to be authoritative and extensible.

## Example: “make this NPC a merchant and let me buy an apple”

Assume the project currently has an NPC and a location, but does **not** yet have authoritative items/ownership, merchant stock, money/value, transaction rules, shop/inventory projections, or trade UI.

Verdict:

```text
PREMATURE
```

The desired outcome is valid. Implementing it immediately would likely produce a fake path such as:

```gdscript
price = 5
merchant_apples -= 1
player_money -= price
player_inventory.append("apple")
```

That would put authoritative economy/inventory in Godot, create a player-only transaction, and establish throwaway state that later has to be painfully migrated.

A correct dependency path is closer to:

```text
authoritative item / ownership / stock state
  -> minimum value/currency/offer model needed by this trade
  -> shared actor transaction rule with atomic validation/transfer
  -> ShopProjection + InventoryProjection
  -> GDExtension translation
  -> design-system trade UI
  -> bounded playtest
```

The next task is the **first missing load-bearing primitive**, not the final merchant screen.

If the user wants trading earlier, reorder those prerequisites earlier. Do not skip them.

## Example response when refusing a premature request

Keep the response direct and useful:

> The goal is valid, but it is premature in the current repository state. We have X, but not A/B/C. Implementing it now would force Y, which violates Z or creates migration debt. The shortest correct path is A → B → target. The next bounded task should be A. If you want this capability earlier, we can reprioritize that path; we should not fake the end state first.

Do not be patronizing, vague, or obstructionist. Name the repository facts and the dependency path.

## Relationship to AI Layer

This policy is repository-owned because it protects product/architecture/modeling correctness for every coding host.

It does **not** store or manage:

- current task state;
- task queues;
- Work/Epic lifecycle;
- continuation state;
- project registry/database state;
- another STOP/`продолжай` protocol.

Those workflow concerns remain external to this repository under AI Layer. Change admission is simply a mandatory engineering judgment immediately before production implementation.

## Relationship to source-of-truth documents

Admission does not invent new product or architecture truth. It routes to existing owners:

- desired gameplay outcome → `PRODUCT.md`;
- runtime ownership/dependency constraints → `ARCHITECTURE.md`;
- simulation causality/fidelity/history/magic assumptions → `MODELING.md` + relevant model/research/ADR;
- current sequencing → `ROADMAP.md`;
- implementation details → current source/tests + relevant engineering guide;
- evidence requirements → `VERIFICATION.md`.

If a user intentionally changes a durable product/model/architecture decision, update the corresponding canonical owner and re-run admission against the new decision rather than bypassing it locally.
