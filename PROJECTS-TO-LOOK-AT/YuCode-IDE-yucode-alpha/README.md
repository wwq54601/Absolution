# YuCode IDE

AI-Native C++ Development Environment powered by a fully local agent runtime.

YuCode is a VSCode-based IDE focused on autonomous code editing, planning, verification, and local AI execution.

## Features

### AI Agent

* Multi-step planning
* Tool-based execution
* File editing
* Symbol search
* Reference search
* Semantic search
* Impact analysis

### Code Modification

* Pending Changes
* Multi-file edits
* Unified Diff generation
* AST Patch system
* Apply / Reject workflow

### Verification

* Build verification
* Test verification
* Automatic error parsing
* Fix loop

Supported toolchains:

* CMake
* C++
* Node.js
* Python
* Rust
* Go

### Local AI Runtime

YuCode runs completely locally.

Supported backends:

* NVIDIA CUDA 13
* NVIDIA CUDA 12
* AMD Vulkan
* CPU

Runtime selection is automatic.

## Architecture

```text
YuCode IDE
      ↓
YuCode Extension
      ↓
YuCode Agent Runtime
      ↓
Embedded llama.cpp Runtime
```

Execution flow:

```text
Prompt
↓
Plan
↓
Tool Calls
↓
Pending Changes
↓
Diff
↓
Apply
↓
Verify
↓
Fix Loop
```

## Current Status

Alpha 0.1

Implemented:

* Agent Runtime
* Planning System
* Streaming
* Pending Changes
* Multi-file Editing
* AST Patch
* Verify System
* Fix Loop
* CUDA/Vulkan Detection
* Embedded Runtime

In Progress:

* Right Sidebar UI
* Pending Change UI Improvements
* Packaging
* Installer

## License

MIT

Based on Visual Studio Code.
