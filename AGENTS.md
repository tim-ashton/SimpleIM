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