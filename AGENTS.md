# AGENTS.md

## Project overview

This is a C++ project built with CMake.

Use the repository's existing CMake presets. Do not invent alternate build commands unless the presets are missing or broken.

## Environment

Codex Cloud should use the environment configured in the Codex web interface.

Do not install packages during normal tasks unless explicitly asked. Dependency installation should be handled by the Codex Cloud environment setup.

## Build commands

Configure the build with:

```bash
cmake --preset linux
```
Compile the build with:

```bash
cmake --build --preset linux
```

## Development rules
- Keep changes focused and minimal, ask and discuss if larger changes become required.
- Preserve existing code style.
- Do not reformat unrelated files without asking.
- ask when requirements are ambiguous
- do not “guess and implement” when there are multiple valid designs

## Discussion and approval rules

Codex should make small, low-risk fixes directly when the requested change is clear.

Before making larger or ambiguous changes, Codex must stop and discuss the proposed approach first.

Ask for clarification before proceeding when:

- The requested behavior is ambiguous.
- There are multiple reasonable implementation approaches.
- The change affects architecture, public APIs, file formats, protocols, build structure, or persistent data.
- The change may break compatibility with existing users, saved data, configuration files, or tests.
- The task requires adding a new third-party dependency.
- The task requires changing CMake presets, toolchain files, CI configuration, or environment setup.
- The task requires deleting, renaming, or moving significant files.
- The task requires broad refactoring across unrelated modules.
- The task cannot be validated with the available build/test commands.

For these cases, Codex should first provide:

1. A short summary of the problem.
2. The proposed approach.
3. Any alternatives considered.
4. The expected files to change.
5. Risks or compatibility concerns.
6. The exact commands it intends to run for build/test validation.

Do not implement until the user approves the approach.

## Changes Codex may make without prior discussion

Codex may proceed directly for clearly scoped, low-risk changes, including:

- Fixing obvious compile errors.
- Fixing formatting or spelling in comments and documentation.
- Adding or updating small unit tests for existing behavior.
- Making localized bug fixes within a single component.
- Updating implementation details without changing public APIs.
- Improving error handling where behavior is already clearly intended.

Even for these changes, keep the diff focused and explain what was changed.

## Build-system changes

The project uses CMake Presets.

Codex may use existing presets to configure, build, and test the project.

Codex must ask before:

- Adding, removing, or renaming presets.
- Changing the generator.
- Changing the build directory layout.
- Changing compiler/toolchain selection.
- Adding new required system packages.
- Adding new package managers or dependency mechanisms.
- Replacing the existing build flow with a different one.

Default build flow:

```bash
cmake --preset linux
cmake --build --preset linux
```

## Change size and delivery rules

Prefer small, reviewable changes.

For normal tasks, keep changes focused to one logical concern. Avoid mixing refactoring, formatting, behavior changes, and build-system changes in the same change unless explicitly requested.

Before making a large change, Codex must stop and propose a plan.

A change is considered large if it:

- Touches more than about 5 source files.
- Changes public APIs or exported interfaces.
- Changes build system structure, CMake presets, CI, or dependency setup.
- Moves, renames, or deletes files.
- Refactors code across multiple modules.
- Changes behavior in a way that may affect users or saved data.
- Cannot be validated with the normal build/test commands.

For large changes, first provide:

1. Goal of the change.
2. Proposed implementation approach.
3. Files likely to be changed.
4. Risks or compatibility concerns.
5. Build/test commands that will be run.
6. Suggested split into smaller PRs if appropriate.

Do not implement the large change until the user approves the plan.