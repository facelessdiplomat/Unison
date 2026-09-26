# LAN test

Definition of Done item 2 (`docs/DESIGN.md` §2): two machines on a LAN run `unison_relay` and two
`unison_console` clients and play the Arena at 60 Hz for ten minutes with rollback and zero desyncs. Item 10 asks
the same of a console on Windows and a console on macOS, once through a relay on each, which
`docs/CROSS_PLATFORM.md` plays as X.8.2 and X.8.3. This file is the procedure and the record of every run. A run
passes when both consoles end on their own with exit code 0 after at least ten minutes of play together.

## Machines

- **Machine A** runs the relay and the first console.
- **Machine B**, on the same network, runs the second console.

Either may be a Windows 10 or 11 x64 machine or a Mac with Apple silicon on macOS 27, and every step says what to
type on each. Between Windows and a Mac the test runs twice: first with Windows as machine A, then with the Mac.

Both machines play the same commit, since consoles built from different commits may never meet in one match, or
meet and desync. This prints the same hash on both:

```sh
git log -1 --format=%h
```

## Before the run

1. On every machine that builds, build and test everything; the last line reads `ci: ok`. On Windows:

   ```powershell
   .\tools\ci.ps1
   ```

   On a Mac, in Terminal, in the repository's folder:

   ```sh
   tools/ci.sh
   ```

   The run needs only the Release binaries: `cmake --workflow --preset clang-release` builds and tests them
   alone on a Mac, `cmake --workflow --preset msvc-release` on Windows after `.\tools\env.ps1`.

2. On machine A, find its address on the LAN. On Windows it is the `IPv4 Address` of the adapter that faces the
   network:

   ```powershell
   ipconfig
   ```

   On a Mac it is the address of its Wi-Fi, `en0` on a MacBook; for a wired adapter,
   `networksetup -listallhardwareports` names the device to ask in place of `en0`:

   ```sh
   ipconfig getifaddr en0
   ```

3. Machine B runs a console built for its own platform. A Mac, or a Windows machine that builds, uses its build
   from step 1. A Windows machine B that builds nothing gets a copy of
   `build\msvc-release\tools\console\unison_console.exe` from a Windows machine A, for example at
   `C:\unison\unison_console.exe`. The binaries link the Microsoft C++ runtime as DLLs: if the copy does not
   start and names `VCRUNTIME140.dll` or `MSVCP140.dll`, install the latest x64 package from Microsoft's page
   "Latest supported Visual C++ Redistributable downloads", then try again.
4. On a Mac, give each console a Terminal window of at least 120 columns and 30 rows. Its screen is 27 rows, the
   status line, 24 rows of map and a line for each player, and the status line runs to about 116 columns; in a
   smaller window every redraw scrolls. `stty size` prints the window's rows and then its columns.
5. On a Mac, the console reads the keys through `CGEventSourceKeyState`, which macOS may keep behind the Input
   Monitoring permission of the terminal app. When it stops, the console says if its terminal lacks it; if the
   keys did nothing, allow the terminal app, Terminal or another, in System Settings, Privacy & Security, Input
   Monitoring, reopen it and start the console again.

## The run

On Windows the binaries are `.\build\msvc-release\tools\relay\unison_relay.exe` and
`.\build\msvc-release\tools\console\unison_console.exe`. On a Mac they are
`build/clang-release/tools/relay/unison_relay` and `build/clang-release/tools/console/unison_console`, started
from the repository's folder.

1. On machine A, start the relay in a window of its own. It prints `unison_relay: listening on 0.0.0.0:7777`.
   The relay needs inbound UDP on port 7777 from machine B. On Windows, the first time, Windows Firewall asks
   whether `unison_relay` may accept connections; which networks to allow it on is the owner's call: if the
   LAN's profile is Private, allowing Private networks is enough. On a Mac whose firewall is on, in System
   Settings, Network, Firewall, macOS asks whether `unison_relay` may accept incoming network connections; allow
   it. It may ask again after the relay is built anew.

   ```powershell
   .\build\msvc-release\tools\relay\unison_relay.exe --bind 0.0.0.0 --port 7777
   ```

   ```sh
   build/clang-release/tools/relay/unison_relay --bind 0.0.0.0 --port 7777
   ```

2. On machine A, in a second window, start the first console. It talks to the relay over loopback, so
   `--from 127.0.0.1` keeps it off the LAN and out of the firewall's way. `--run-for 660` gives eleven
   minutes, so that ten minutes of play together survive starting the two consoles up to a minute apart.

   ```powershell
   .\build\msvc-release\tools\console\unison_console.exe --host 127.0.0.1 --from 127.0.0.1 --name a --run-for 660
   ```

   ```sh
   build/clang-release/tools/console/unison_console --host 127.0.0.1 --from 127.0.0.1 --name a --run-for 660
   ```

3. On machine B, start the second console, with machine A's address from step 2 of the preparation in place
   of `192.168.1.10`; a copied console on Windows starts as `C:\unison\unison_console.exe` with the same options.
   If a firewall asks about `unison_console` there, which networks to allow it on is the owner's call as well.

   ```powershell
   .\build\msvc-release\tools\console\unison_console.exe --host 192.168.1.10 --name b --run-for 660
   ```

   ```sh
   build/clang-release/tools/console/unison_console --host 192.168.1.10 --name b --run-for 660
   ```

4. Play until the consoles end on their own. W and S move forward and back, A and D to the sides, Q and E turn,
   Space jumps and F fires. A console on Windows reads the keyboard only while its window has focus; a console
   on a Mac reads the keys whichever window is in front, so keys typed anywhere on that Mac move its player.
   Keep the players moving, turning and firing, one machine at a time if one person plays both, so that
   predictions miss and the status line shows rollbacks in the last second above nought from time to time; a
   match nobody plays never rolls back.
5. Watch the status line at the top of each screen: the state reads `playing`, the round trip stays at a few
   milliseconds on a wired LAN, and the verified frame follows the predicted one closely.
6. When each console has ended, read its exit code in its window, as the first command after it ended: on
   Windows with the first line, on a Mac with the second.

   ```powershell
   $LASTEXITCODE
   ```

   ```sh
   echo $?
   ```

7. Stop the relay with Ctrl+C; it prints `unison_relay: stopped`.

## Reading the result

- **Pass**: both consoles print `0`. The last line of each reads
  `unison_console: <name> playing in slot <n>, verified <v>, ...` with nothing about a desync at its end, and
  `<v>` is at least 36000, ten minutes at 60 Hz.
- **Desync**: the console prints `2`, and its last line ends with `desync on frame <f> by slot <s>`. Record
  the frame, the slots and both windows' last lines; the run fails. Each console has written the snapshot of that
  frame into `desync_<f>_<slot>.snapshot` in the folder it was started from; bring both files to one machine and
  `unison_replay diff` names the component, the entity and the field they first differ in:

  ```sh
  build/clang-release/tools/replay/unison_replay diff desync_<f>_0.snapshot desync_<f>_1.snapshot
  ```
- **Lost the relay**: the console prints `1` and its last line reads `<name> disconnected`. The network or the
  firewall stood between the machines; fix that and run again. It is no verdict on determinism. On a Mac with a
  VPN on, `route -n get` with the other machine's address names the device that carries the traffic; it should
  be the Wi-Fi's or the wired adapter's, not a `utun` one.

## Record

| Date | Machine A | Machine B | Link | Round trip | Rollbacks a second | Verified at the end, A / B | Exit codes, A / B | Result |
|------|-----------|-----------|------|------------|--------------------|----------------------------|-------------------|--------|
