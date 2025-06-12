# Dott

_A minimal dotfile tracker with version controlling and management capabilities._

---

> **Note:**  
> This project is currently in an early development phase. The implementation is not yet production grade and is missing several features, robustness, and error handling improvements. PRs and suggestions are welcome!

---

## Overview

**DotTracker** (`dott`) is a CLI tool designed to help you efficiently track, version, and manage your dotfiles. Inspired by version control concepts, Dott allows you to initialize repositories, stage files, and keep your configuration files organized and portable.

- **Track your dotfiles** in a dedicated repository.
- **Stage and version** configuration files easily.
- **Minimal, Unix-like CLI** for managing your dotfiles.

## Features

- Initialize a new dotfile repository or reinitialize an existing one.
- Add files to an index (stage them for versioning).
- Show which files are currently staged and awaiting commit.
- Tracks repositories in `~/.dottracker/dottfiles`.
- Stores objects using hashed object names (like git).
- Simple file hash calculation using SHA1.

## Usage

### Installation

> **Manual Build (Linux/Unix):**
>
> 1. Clone the repository:
>    ```sh
>    git clone https://github.com/messi10tom/DotTracker.git
>    cd DotTracker
>    ```
> 2. Build with your preferred C++ compiler (requires C++17+, OpenSSL for SHA1):
>    ```sh
>    g++ -std=c++17 -lssl -lcrypto -o dott src/main.cpp src/dott/dott.cpp src/ob/cli.cpp
>    ```
> 3. Optionally, add `dott` to your `$PATH`.

### Basic Commands

- **Initialize a repository**
  ```sh
  dott init <directory>
  ```
  Creates a `.dott` directory in the specified location, or reinitializes if already present.

- **Add (stage) a file**
  ```sh
  dott add <pathspec>
  ```
  Stages a file or files for tracking by hashing and storing them in the object store.

- **Show staged files**
  ```sh
  dott status <directory>
  ```
  Lists all files currently staged for commit in the repository.

### Example

```sh
# Initialize dotfile repo in your $HOME
dott init $HOME

# Stage your .bashrc
dott add $HOME/.bashrc

# See which files are staged
dott status $HOME
```

## Planned Improvements

- [ ] Commit and restore functionality (currently only staging/index is implemented)
- [ ] Robust error handling and user feedback
- [ ] Cross-platform support (currently Linux/Unix focused)
- [ ] Integration with remote storage or Git
- [ ] More comprehensive CLI help and documentation
- [ ] Automated tests and CI pipeline

## Limitations

- No commit/restore, merge, or history management yet.
- No advanced conflict resolution or change tracking.
- Minimal input validation—use with caution!
- Only basic CLI commands implemented (`init`, `add`, `status`).

## Contributing

Contributions are highly encouraged! Please open issues for bugs, suggestions, or feature requests.

1. Fork and clone the repository.
2. Create a feature branch.
3. Make your changes and ensure code builds.
4. Open a Pull Request with a clear description of your changes.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

**Author:** Messi Tom  
**Repository:** https://github.com/messi10tom/DotTracker

