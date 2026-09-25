# LAN test

Definition of Done item 2 (`docs/DESIGN.md` §2): two machines on a LAN run `unison_relay` and two
`unison_console` clients and play the Arena at 60 Hz for ten minutes with rollback and zero desyncs. This file
is the procedure and the record of every run. A run passes when both consoles end on their own with exit code
0 after at least ten minutes of play together.

## Machines

- **Machine A**, the development machine: builds the Release binaries and runs the relay and the first console.
- **Machine B**, a second Windows 10 or 11 x64 machine on the same network: runs the second console.

The binaries link the Microsoft C++ runtime as DLLs. If `unison_console.exe` does not start on machine B and
names `VCRUNTIME140.dll` or `MSVCP140.dll`, install the latest x64 package from Microsoft's page "Latest
supported Visual C++ Redistributable downloads", then try again.

On a Mac, the console reads the keys through `CGEventSourceKeyState`, which macOS may keep behind the Input
Monitoring permission of the terminal app. The console says at start whether its terminal holds it; if the keys
do nothing, allow the terminal app, Terminal or another, in System Settings, Privacy & Security, Input Monitoring,
and start the console again.

## Before the run

1. On machine A, build and test everything; the last line reads `ci: ok`.

   ```powershell
   .\tools\ci.ps1
   ```

2. On machine A, find its address on the LAN: the `IPv4 Address` of the adapter that faces the network.

   ```powershell
   ipconfig
   ```

3. Copy `build\msvc-release\tools\console\unison_console.exe` from machine A to machine B, for example to
   `C:\unison\unison_console.exe`.

## The run

1. On machine A, start the relay in a window of its own. It prints `unison_relay: listening on 0.0.0.0:7777`.
   The first time, Windows Firewall asks whether `unison_relay` may accept connections; the relay needs inbound
   UDP on port 7777 from machine B. Which networks to allow it on is the owner's call: if the LAN's profile is
   Private, allowing Private networks is enough.

   ```powershell
   .\build\msvc-release\tools\relay\unison_relay.exe --bind 0.0.0.0 --port 7777
   ```

2. On machine A, in a second window, start the first console. It talks to the relay over loopback, so
   `--from 127.0.0.1` keeps it off the LAN and out of the firewall's way. `--run-for 660` gives eleven
   minutes, so that ten minutes of play together survive starting the two consoles up to a minute apart.

   ```powershell
   .\build\msvc-release\tools\console\unison_console.exe --host 127.0.0.1 --from 127.0.0.1 --name a --run-for 660
   ```

3. On machine B, start the second console, with machine A's address from step 2 of the preparation in place
   of `192.168.1.10`. If Windows Firewall asks about `unison_console` there, which networks to allow it on is
   the owner's call as well.

   ```powershell
   C:\unison\unison_console.exe --host 192.168.1.10 --name b --run-for 660
   ```

4. Play until the consoles end on their own. A console reads the keyboard only while its window has focus:
   W and S move forward and back, A and D to the sides, Q and E turn, Space jumps and F fires. Keep the
   players moving, turning and firing, one machine at a time if one person plays both, so that predictions
   miss and the status line shows rollbacks in the last second above nought from time to time; a match
   nobody plays never rolls back.
5. Watch the status line at the top of each screen: the state reads `playing`, the round trip stays at a few
   milliseconds on a wired LAN, and the verified frame follows the predicted one closely.
6. When each console has ended, read its exit code in its window.

   ```powershell
   $LASTEXITCODE
   ```

7. Stop the relay with Ctrl+C; it prints `unison_relay: stopped`.

## Reading the result

- **Pass**: both consoles print `0`. The last line of each reads
  `unison_console: <name> playing in slot <n>, verified <v>, ...` with nothing about a desync at its end, and
  `<v>` is at least 36000, ten minutes at 60 Hz.
- **Desync**: the console prints `2`, and its last line ends with `desync on frame <f> by slot <s>`. Record
  the frame, the slots and both windows' last lines; the run fails.
- **Lost the relay**: the console prints `1` and its last line reads `<name> disconnected`. The network or the
  firewall stood between the machines; fix that and run again. It is no verdict on determinism.

## Record

| Date | Machine A | Machine B | Link | Round trip | Rollbacks a second | Verified at the end, A / B | Exit codes, A / B | Result |
|------|-----------|-----------|------|------------|--------------------|----------------------------|-------------------|--------|
