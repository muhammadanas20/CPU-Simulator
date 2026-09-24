# File Handling

All file access goes through `src/filesystem/`. There are no hard-coded program contents: everything
the simulator runs is read from disk.

## Streams used
| Stream | Mode | Where | Purpose |
|---|---|---|---|
| `std::ifstream` | read | `FileManager::readLines`, `SnapshotManager::load` | read `.asm`, CSV index, snapshots |
| `std::ofstream` | `out \| trunc` | `FileManager::writeLines/writeText`, `ReportManager`, `SnapshotManager::save` | create / **overwrite** |
| `std::fstream` | `out \| app` | `FileManager::appendLine` | **append** a line |
| `std::fstream` | `in \| binary` + `out \| trunc \| binary` | `FileManager::copyFile` | byte-exact copy |

`std::filesystem` is used only for `exists`, `create_directories`, `file_size`, `remove` and
`directory_iterator`. These are OS services, not DS concepts.

## Directories
```
programs/    *.asm source programs
data/        program_index.csv (created automatically if missing)
reports/     <name>_report.txt, <name>_history.txt
snapshots/   snapshot_001.dat, snapshot_002.dat, ...
```
All four are created at startup if missing. The program looks for them in the current directory, or in the directory given as `argv[1]`.

## 1. Assembly program files (`.asm`)
Plain text with one instruction or label per line; `;` starts a comment. Windows line endings (`\r\n`) are accepted.
| Operation | Menu | Implementation |
|---|---|---|
| Create | Program Mgmt → 1 | `ProgramManager::create` (in memory until saved; refuses an existing name) |
| Open / Read | → 2 | `readLines` (ifstream) |
| Write / Save | → 4 or editor → 6 | `writeLines` (ofstream, trunc) |
| Overwrite | Save As onto an existing name | asks `Overwrite? (y/n)` |
| Save As | → 5 | new name, new path, then `save` |
| Delete | → 6 | `removeFile` + remove from index (asks for confirmation) |

## 2. Program metadata — `data/program_index.csv`
```
id,name,path,instructions,executions,size,status,last_execution
1,addition.asm,./programs/addition.asm,4,1,91,OK,2026-09-24 15:35:32
```
* Loaded at startup into the repository linked list. Missing file → a new one is created with just the header.
* After loading, `syncWithDirectory` adds any `.asm` file in `programs/` that is not yet in the index, and marks entries whose file has disappeared as `MISSING`.
* **Corrupted rows** (wrong column count, non-numeric counts, empty name, id ≤ 0, duplicates) are skipped with a warning such as
  `WARNING: Corrupted metadata at ./data/program_index.csv line 4 - row skipped: "garbage,row"`. The file is rewritten cleanly on the next save.
* Commas in names are replaced with `_` when saving, so they cannot break the CSV format.
* Updated whenever a program is saved, deleted, sorted or executed (execution count + timestamp).

## 3. Execution reports — `reports/<name>_report.txt`
Contains the program name, date/time, number of instructions, number of steps, final registers (R0–R7, PC, SP),
flags, non-zero memory, stack contents, every executed step with its effect, and a status/errors section.
Written with `ofstream` by traversing the history linked list.
`<name>_history.txt` (CPU menu → 14) holds the full before/after register state of every step.

## 4. Memory snapshots — `snapshots/snapshot_NNN.dat`
```
# CPU-DS memory snapshot
PROGRAM array_sum.asm
REG R0 0 ... REG R7 0       <- registers ARE included
PC 19
SP 1000
FLAGS 1 0 0 0               <- ZF SF CF OF
MEM 200 10                  <- address value (only non-zero cells, sparse format)
MEM 300 150
END
```
* **Save:** picks the next free number (`nextPath`), so snapshots are never overwritten.
* **Load:** parses into *temporary* registers and memory. The CPU is changed only if the whole file is valid
  (atomic load). It rejects unknown records, bad numbers, addresses outside 0–1023, a missing `END`
  (truncated file) and a PC beyond the loaded program. After loading, the instruction queue is refilled from the restored PC.
* The CPU stack contents are **not** stored (only SP), so a restored machine starts with an empty stack.
  This is documented as a known limitation and would be easy to add as `STACK v` lines.

## Error handling summary
| Situation | Message (example) |
|---|---|
| File doesn't exist | `File './programs/x.asm' does not exist.` |
| Cannot open | `File '...' cannot be opened for writing.` |
| Empty file / program | `Empty program: no instructions found.` |
| Corrupted metadata | warning plus row skipped |
| Corrupted snapshot | `Corrupted snapshot line 2: invalid memory address -> "MEM 5000 1"` |
| Truncated snapshot | `Corrupted snapshot: missing END marker (file truncated?).` |
| Existing file on Save As | overwrite confirmation |
