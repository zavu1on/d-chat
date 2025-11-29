![Demo](d-chat.mp4)

# d-chat — Decentralized TCP Chat with Blockchain Messages

d-chat is a lightweight, Windows-targeted decentralized chat application implemented in modern C++. Nodes communicate over TCP sockets and exchange messages encoded as blockchain blocks. The project was completed as a coursework project at Peter the Great St. Petersburg Polytechnic University (SPbPU), Applied Computer Science program.

**Tech stack & notable libraries**
- **Language**: C++17/C++20 (project uses modern C++ idioms)
- **Build system**: CMake (CMakePresets provided), Ninja or MSVC generator supported
- **Dependencies**: `nlohmann::json`, OpenSSL (crypto), `vcpkg` manifest included

**Design overview**
- **Purpose**: Provide a decentralized chat where each message is stored and transferred inside blockchain-style blocks. The chain enables message integrity checks and basic replication between peers.
- **Scope**: Peer discovery and management, sending/receiving text messages, storing messages and blocks in a local DB, simple block validation and block range synchronization.
- **Platform**: Windows (console UTF-8 aware; `ChatApplication` sets console code page accordingly).

**Key modules**
- **chat**: Implements `chat::ChatService` — the protocol handler for incoming/outgoing JSON messages (text, connection, disconnection, peer lists, block ranges, errors). Responsible for routing messages between network layer and application services.
- **peer**: `peer::PeerService` manages known hosts and active peers, chat peers list, and peer lookups. Persists peers via `IPeerRepo` implementations.
- **blockchain**: `blockchain::Block`, `blockchain::BlockchainService` — block structure, hashing, signature verification and validation routines, storage and range retrieval. New messages may be packaged into blocks and broadcast to peers.
- **network**: Low-level TCP networking implemented in `infra/network` (`TCPServer`, `TCPClient`, `SocketServer`, `SocketClient`). `TCPServer` accepts and dispatches incoming JSON messages to `ChatService`. `TCPClient` connects to other peers, sends messages, and supports batch sync (peers/blocks).
- **message**: Message types (`TextMessage`, `PeerListMessage`, `BlockRangeMessage`, etc.) with (de)serialization used across chat and blockchain layers.
- **service / infra**: Repositories and persistence layers (DB file), socket wrappers, and glue code that connect domain logic with runtime behavior.
- **ui**: `ui::ConsoleUI` — simple console-based interface used by `app::ChatApplication` for user commands and logging.
- **utils**: Small helpers: `uuid`, `timestamp`, `sha256`, hex utils.

Repository layout (high level)
```
./
  README.md
  build_and_run.bat
  CMakeLists.txt
  CMakePresets.json
  d-chat_config.json  # runtime config
  d-chat.db           # application store 
  src/
    app/              # entrypoint: ChatApplication, main
    domain/           # domain models and services (chat, peer, blockchain, message)
    infra/            # platform/network implementations (TCPServer/TCPClient)
    service/          # lower-level socket wrappers and helpers
    ui/               # console UI
    utils/            # uuid, timestamp, sha256
```

What is implemented (high level)
- Console application with an interactive prompt and commands (`/help`, `/peers`, `/chats`, `/chat`, `/send`, `/exit`).
- Peer management: load trusted peers from `d-chat_config.json`, maintain active peers, add/remove peers at runtime.
- Message sending: send to a single peer or broadcast to all known peers.
- Local persistence: simple DB file (`d-chat.db`) used by repositories for peers, messages and chain.
- Blockchain primitives: `Block` structure with canonical stringization and SHA256 hashing; `BlockchainService` provides basic validation, storing and broadcasting of blocks.
- Networking: TCP server and client implementation with JSON messages and simple request/response handling.
- Basic chain sync: request peer lists and block ranges on startup, store and validate received blocks.
- Test coverage: unit tests, integration tests, and end-to-end tests of all modules.

Planned / next tasks
- Enhance consensus and fork handling: stronger validation, reorg handling, conflict resolution, partial restoration of the blockchain.
- Make the server multithreaded: handle multiple clients in parallel, provide thread-safe access to the blockchain.
- Use RVO/NRVO approach to return objects from functions.
```cpp
// ❌ C-style: current implementation
void createBlock(Block& block)
{
    block.hash = "...";
}

// ✅ C++17: return by value
Block createBlock()
{
    Block block;
    block.hash = "...";
    return block;  // object is not recreated (Named Return Value Optimization)
}
```
- Add a lightweight GUI.

**Build & run (Windows) — detailed, machine-independent instructions**

This project is intended for Windows. Below are two supported toolchain workflows (recommended):

- **Visual Studio (MSVC)** — recommended for easiest toolchain integration.
- **MSYS2 / MinGW-w64 (Ninja)** — the project includes a helper script (`build_and_run.bat`) that uses MinGW by default; use this if you prefer a portable Unix-like toolchain.

Prerequisites (choose one toolchain path)

- Common prerequisites
  - `git` (to clone vcpkg if needed)
  - `CMake` (>= 3.20) — add to `PATH`
  - `vcpkg` (package manager) — used to build and provide `nlohmann::json`, OpenSSL and other deps
  - `ninja` (optional; used in the Ninja generator flow)

- For Visual Studio (recommended)
  - Visual Studio 2022 or later with "Desktop development with C++" workload (MSVC toolchain)

- For MSYS2 / MinGW (alternative)
  - Install MSYS2 (https://www.msys2.org/). Use the `MSYS2 MinGW 64-bit` shell and install the MinGW toolchain.

Step-by-step: prepare `vcpkg` and dependencies (works for both toolchains)

PowerShell (run as a normal user from an elevated or non-elevated prompt as you prefer):

```powershell
# Create a tools folder and clone vcpkg
New-Item -ItemType Directory -Path C:\tools -Force | Out-Null
git clone https://github.com/microsoft/vcpkg.git C:\tools\vcpkg

# Bootstrap vcpkg (Windows)
&C:\tools\vcpkg\bootstrap-vcpkg.bat

# Integrate vcpkg with your user-wide environment (optional)
C:\tools\vcpkg\vcpkg integrate install

# Install required libraries for MSVC (x64)
C:\tools\vcpkg\vcpkg install nlohmann-json openssl:x64-windows

# If you plan to use MinGW (MSYS2) with the bundled helper, install the MinGW triplet libs instead:
# C:\tools\vcpkg\vcpkg install nlohmann-json openssl:x64-mingw-static
```

Build using Visual Studio (MSVC)

Open an ordinary PowerShell in the repository root and run:

```powershell
mkdir build; cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DDEBUG=OFF
cmake --build build --config Release
```

After a successful build the executable should be under `build\src\app\` (look for `d-chat_main.exe`).

Build using MSYS2 / MinGW (Ninja) — matches the included `build_and_run.bat`

1. Install MSYS2 from https://www.msys2.org/ and open the `MSYS2 MinGW 64-bit` shell.
2. In the MinGW64 shell install toolchain components (one-time):

```bash
# inside MSYS2 MinGW64 shell
pacman -Syu
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

3. Bootstrap vcpkg and install MinGW-triplet dependencies (from PowerShell or cmd):

```powershell
C:\tools\vcpkg\bootstrap-vcpkg.bat
C:\tools\vcpkg\vcpkg install nlohmann-json openssl:x64-mingw-static
```

4. Use the bundled helper (convenient):

```powershell
# From repository root
.\build_and_run.bat release
```

Or run CMake manually (PowerShell from repo root):

```powershell
mkdir build
cmake -S . -B build -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static -DDEBUG=OFF
cmake --build build --config Release
```

Notes & troubleshooting
- If `vcpkg` is on a different path, replace `C:/tools/vcpkg` with your clone path.
- For MSVC use the `:x64-windows` vcpkg triplet; for MinGW use `:x64-mingw-static` as shown above.
- If CMake cannot find OpenSSL or other libs, ensure the `-DCMAKE_TOOLCHAIN_FILE` option points to the exact vcpkg toolchain file and that the requested triplet is installed.
- The helper script `build_and_run.bat` included in the repo attempts to use MinGW at `C:\msys64\mingw64\bin` and the `x64-mingw-static` triplet. Edit the script if your MSYS2 is installed elsewhere.

Runtime
- `d-chat_config.json` holds runtime settings (host, port, trusted peers). On first run a default config may be generated.
- The produced executable is a console app and expects UTF-8 capable console (the app sets the console code page to UTF-8 at startup).


Notes
- The app config file `d-chat_config.json` contains host/port and trusted peers. On first run a default config may be generated automatically.
- The console expects UTF-8 output; `ChatApplication` sets the Windows console codepage to UTF-8 at startup.

Academic note
- This project was implemented as a coursework project at Peter the Great St. Petersburg Polytechnic University (SPbPU), Applied Computer Science.
