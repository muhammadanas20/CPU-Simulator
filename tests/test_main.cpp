// ============================================================================
// tests/test_main.cpp  -  Phase 18: self-contained unit/integration tests.
// No external framework: CHECK() records pass/fail, main() prints a summary
// and returns non-zero if anything failed.
// Build & run:   make test     or   g++ -std=c++17 tests/test_main.cpp -o t && ./t
// ============================================================================
#include <iostream>
#include <string>
#include "../src/algorithms/Searching.h"
#include "../src/algorithms/Sorting.h"
#include "../src/data_structures/DynamicArray.h"
#include "../src/data_structures/HashTable.h"
#include "../src/data_structures/LinkedList.h"
#include "../src/data_structures/Queue.h"
#include "../src/data_structures/Stack.h"
#include "../src/filesystem/FileManager.h"
#include "../src/filesystem/ReportManager.h"
#include "../src/filesystem/SnapshotManager.h"
#include "../src/program/Assembler.h"
#include "../src/program/ProgramManager.h"
#include "../src/simulator/CPU.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond)                                                                        \
    do {                                                                                   \
        if (cond) ++g_pass;                                                                \
        else { ++g_fail; std::cout << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; } \
    } while (0)
template <typename F>
static bool throws(F f) { try { f(); } catch (...) { return true; } return false; }
static void section(const char* s) { std::cout << "[" << s << "]\n"; }

static const std::string TMP = "tests/tmp";

// helper: assemble + run source text, return the CPU
static CPU runSource(const std::string& src, bool* ok = nullptr, std::string* firstErr = nullptr) {
    DynamicArray<std::string> lines;
    std::string cur;
    for (char c : src) { if (c == '\n') { lines.pushBack(cur); cur.clear(); } else cur += c; }
    if (!cur.empty()) lines.pushBack(cur);
    AssemblyResult a = Assembler::assemble(lines);
    if (ok) *ok = a.ok();
    if (firstErr && !a.ok()) *firstErr = a.errors[0];
    CPU cpu;
    if (a.ok()) { cpu.load("test.asm", a.instructions, a.symbols); cpu.run(); }
    return cpu;
}

static void testDynamicArray() {
    section("DynamicArray");
    DynamicArray<int> a(2);
    a.pushBack(1); a.pushBack(2);
    CHECK(a.capacity() == 2);
    a.pushBack(3);                       // forces resize
    CHECK(a.capacity() == 4 && a.size() == 3 && a.resizeCount() == 1);
    a.insert(0, 0);                      // beginning
    a.insert(2, 99);                     // middle -> 0 1 99 2 3
    CHECK(a[0] == 0 && a[2] == 99 && a[4] == 3);
    a.removeAt(2);
    CHECK(a.size() == 4 && a[2] == 2);
    a.update(3, 30);
    CHECK(a.get(3) == 30);
    CHECK(a.search(2) == 2 && a.search(12345) == -1);
    CHECK(throws([&] { a.get(10); }));
    CHECK(throws([&] { a.insert(99, 1); }));
    CHECK(throws([&] { a.removeAt(4); }));
    DynamicArray<int> b = a;             // deep copy
    b[0] = 777;
    CHECK(a[0] == 0);
    for (int i = 0; i < 1000; ++i) a.pushBack(i);
    CHECK(a.size() == 1004 && a[1003] == 999);
    while (a.size() > 2) a.removeAt(a.size() - 1);   // shrinking
    CHECK(a.capacity() < 64);
    DynamicArray<int> e;
    CHECK(throws([&] { e.popBack(); }));
}

static void testLinkedList() {
    section("LinkedList");
    LinkedList<int> l;
    l.pushBack(2);          // end
    l.pushFront(1);         // beginning
    l.pushBack(4);
    l.insertAt(2, 3);       // middle -> 1 2 3 4
    CHECK(l.size() == 4);
    for (int i = 0; i < 4; ++i) CHECK(l.get(i) == i + 1);
    long cmp = 0;
    CHECK(l.find([](int v) { return v == 3; }, &cmp) != nullptr && cmp == 3);
    CHECK(l.find([](int v) { return v == 9; }) == nullptr);
    CHECK(l.removeIf([](int v) { return v == 4; }));   // delete tail
    l.pushBack(5);                                      // tail pointer still valid?
    CHECK(l.get(3) == 5);
    l.removeAt(0);                                      // delete head
    CHECK(l.get(0) == 2 && l.size() == 3);
    l.update(1, 30);
    CHECK(l.get(1) == 30);
    CHECK(throws([&] { l.removeAt(10); }));
    CHECK(!l.removeIf([](int v) { return v == 1234; }));
    DynamicArray<int> arr; l.toArray(arr);
    Sorting::insertionSort(arr, [](int a, int b) { return a > b; });
    l.fromArray(arr);
    CHECK(l.get(0) == 30 && l.get(2) == 2);
    l.clear();
    CHECK(l.empty() && l.head() == nullptr);
}

static void testStack() {
    section("Stack");
    Stack<int> s(3);
    CHECK(s.isEmpty());
    s.push(10); s.push(20); s.push(30);
    CHECK(s.isFull() && s.peek() == 30);
    CHECK(throws([&] { s.push(40); }));           // overflow
    CHECK(s.pop() == 30 && s.pop() == 20 && s.pop() == 10);
    CHECK(throws([&] { s.pop(); }));               // underflow
    CHECK(throws([&] { s.peek(); }));
    bool rightType = false;
    try { s.pop(); } catch (const StackUnderflowError&) { rightType = true; }
    CHECK(rightType);
}

static void testQueue() {
    section("Queue");
    Queue<int> q;
    CHECK(q.isEmpty());
    q.enqueue(1); q.enqueue(2); q.enqueue(3);
    CHECK(q.peek() == 1 && q.size() == 3);
    CHECK(q.dequeue() == 1 && q.dequeue() == 2);
    q.enqueue(4);
    CHECK(q.dequeue() == 3 && q.dequeue() == 4 && q.isEmpty());
    CHECK(throws([&] { q.dequeue(); }));          // underflow
    CHECK(throws([&] { q.peek(); }));
    q.enqueue(5);                                 // usable after emptying
    CHECK(q.peek() == 5);
}

static void testHashTable() {
    section("HashTable");
    HashTable<int> h(3);
    CHECK(h.insert("START", 0) && h.insert("LOOP", 2) && h.insert("END", 7));
    CHECK(!h.insert("LOOP", 99));                 // duplicate rejected
    int v = -1;
    CHECK(h.search("LOOP", v) && v == 2);
    CHECK(!h.search("MISSING", v));
    CHECK(h.remove("START") && !h.contains("START") && !h.remove("START"));
    CHECK(h.size() == 2);
    // collision: with 1 bucket every key collides but all remain retrievable
    HashTable<int> one(1);
    one.insert("A", 1);
    one.insert("B", 2);                           // triggers rehash to 3 buckets
    for (int i = 0; i < 20; ++i) one.insert("K" + std::to_string(i), i);
    CHECK(one.size() == 22 && one.collisions() > 0);
    bool all = true;
    for (int i = 0; i < 20; ++i) all = all && one.search("K" + std::to_string(i), v) && v == i;
    CHECK(all);
    CHECK(one.bucketCount() > 1);
    // explicit collision without rehash
    HashTable<int> big(7);
    std::string k1 = "A", k2;
    for (char c = 'B'; c <= 'Z'; ++c) if (big.hash(std::string(1, c)) == big.hash(k1)) { k2 = std::string(1, c); break; }
    CHECK(!k2.empty());
    big.insert(k1, 1); big.insert(k2, 2);
    CHECK(big.collisions() == 1 && big.search(k2, v) && v == 2);
    CHECK(big.remove(k1) && big.search(k2, v));   // delete first node of chain
}

static void testSearchSort() {
    section("Searching & Sorting");
    auto make = [] { DynamicArray<int> a; int v[] = {5, 2, 9, 1, 5, 6, 0, -3}; for (int x : v) a.pushBack(x); return a; };
    auto less = [](int a, int b) { return a < b; };
    auto idk = [](int x) { return x; };
    DynamicArray<int> a = make(), b = make(), c = make();
    SortStats s1 = Sorting::bubbleSort(a, less), s2 = Sorting::selectionSort(b, less), s3 = Sorting::insertionSort(c, less);
    CHECK(Searching::isSorted(a, idk) && Searching::isSorted(b, idk) && Searching::isSorted(c, idk));
    CHECK(a[0] == -3 && a[7] == 9 && b[3] == 2 && c[5] == 5);
    CHECK(s2.comparisons == 28);                  // n(n-1)/2 always for selection
    CHECK(s1.comparisons > 0 && s3.swaps > 0);
    DynamicArray<int> sorted = a;                 // already sorted -> best case
    SortStats best = Sorting::bubbleSort(sorted, less);
    CHECK(best.comparisons == 7 && best.swaps == 0);
    SortStats bestIns = Sorting::insertionSort(sorted, less);
    CHECK(bestIns.comparisons == 7);
    DynamicArray<int> empty; Sorting::bubbleSort(empty, less); Sorting::selectionSort(empty, less); Sorting::insertionSort(empty, less);
    CHECK(empty.size() == 0);
    SearchResult r = Searching::binarySearch(a, 6, idk);
    CHECK(r.index >= 0 && a[r.index] == 6 && r.comparisons <= 4);
    CHECK(Searching::binarySearch(a, 4, idk).index == -1);
    SearchResult lr = Searching::linearSearch(make(), [](int x) { return x == 0; });
    CHECK(lr.index == 6 && lr.comparisons == 7);
    // stability check for bubble / insertion (sort pairs by first only)
    struct P { int k; int tag; };
    DynamicArray<P> ps; ps.pushBack({2, 0}); ps.pushBack({1, 1}); ps.pushBack({2, 2}); ps.pushBack({1, 3});
    DynamicArray<P> ps2 = ps;
    Sorting::bubbleSort(ps, [](const P& x, const P& y) { return x.k < y.k; });
    Sorting::insertionSort(ps2, [](const P& x, const P& y) { return x.k < y.k; });
    CHECK(ps[0].tag == 1 && ps[1].tag == 3 && ps[2].tag == 0 && ps[3].tag == 2);
    CHECK(ps2[0].tag == 1 && ps2[1].tag == 3);
}

static void testFiles() {
    section("File handling");
    std::string err;
    CHECK(FileManager::ensureDirectory(TMP, err));
    std::string p = FileManager::join(TMP, "f.txt");
    FileManager::removeFile(p, err);
    DynamicArray<std::string> lines, back;
    lines.pushBack("line one"); lines.pushBack("line two");
    CHECK(FileManager::writeLines(p, lines, err));                    // create
    CHECK(FileManager::readLines(p, back, err) && back.size() == 2 && back[1] == "line two");   // read
    CHECK(FileManager::appendLine(p, "line three", err));             // append
    FileManager::readLines(p, back, err);
    CHECK(back.size() == 3 && back[2] == "line three");
    DynamicArray<std::string> one; one.pushBack("only");
    CHECK(FileManager::writeLines(p, one, err));                      // overwrite
    FileManager::readLines(p, back, err);
    CHECK(back.size() == 1 && back[0] == "only");
    CHECK(FileManager::fileSize(p) == 5);
    CHECK(!FileManager::readLines(FileManager::join(TMP, "nope.txt"), back, err) && err.find("does not exist") != std::string::npos);  // missing
    std::string e = FileManager::join(TMP, "empty.txt");
    CHECK(FileManager::writeText(e, "", err));
    CHECK(FileManager::readLines(e, back, err) && back.empty());     // empty file
    CHECK(!FileManager::writeLines(TMP + "/no_such_dir/x.txt", one, err));   // cannot open
    std::string cp = FileManager::join(TMP, "copy.txt");
    CHECK(FileManager::copyFile(p, cp, err) && FileManager::readLines(cp, back, err) && back[0] == "only");
    CHECK(FileManager::removeFile(cp, err) && !FileManager::exists(cp));
    CHECK(!FileManager::removeFile(cp, err));
    // CRLF handling
    FileManager::writeText(e, "MOV R1, 1\r\nHALT\r\n", err);
    FileManager::readLines(e, back, err);
    CHECK(back[0] == "MOV R1, 1" && back[1] == "HALT");
}

static void testAssembler() {
    section("Assembler / symbol table");
    bool ok; std::string e;
    runSource("START:\nMOV R1, 10\nLOOP: ADD R1, R2\nJMP LOOP\n", &ok);
    CHECK(ok);
    DynamicArray<std::string> l; l.pushBack("START:"); l.pushBack("MOV R1, 10"); l.pushBack(""); l.pushBack("LOOP:");
    l.pushBack("ADD R1, R2"); l.pushBack("JMP LOOP"); l.pushBack("; comment: with colon");
    AssemblyResult a = Assembler::assemble(l);
    int addr;
    CHECK(a.ok() && a.instructions.size() == 3);
    CHECK(a.symbols.lookup("START", addr) && addr == 0);
    CHECK(a.symbols.lookup("LOOP", addr) && addr == 1);
    CHECK(a.instructions[2].a.value == 1);                  // JMP resolved
    runSource("A:\nA:\nHALT", &ok, &e);          CHECK(!ok && e.find("duplicate label") != std::string::npos);
    runSource("JMP NOWHERE\nHALT", &ok, &e);     CHECK(!ok && e.find("undefined label 'NOWHERE'") != std::string::npos);
    runSource("MOV R8, 1", &ok, &e);             CHECK(!ok && e.find("invalid register") != std::string::npos);
    runSource("MOV R1, 12a", &ok, &e);           CHECK(!ok && e.find("invalid number") != std::string::npos);
    runSource("FOO R1", &ok, &e);                CHECK(!ok && e.find("invalid instruction") != std::string::npos);
    runSource("ADD R1", &ok, &e);                CHECK(!ok && e.find("expects 2") != std::string::npos);
    runSource("MOV 5, R1", &ok, &e);             CHECK(!ok);
    runSource("; nothing\n\n", &ok, &e);         CHECK(!ok && e.find("Empty program") != std::string::npos);
    runSource("HALT\nEND:", &ok);                CHECK(ok);                  // unused trailing label is fine
    runSource("JMP END\nEND:", &ok, &e);         CHECK(!ok && e.find("invalid jump") != std::string::npos);
    runSource("mov r1, 0x10\nhalt", &ok);        CHECK(ok);                  // case-insensitive + hex
    runSource("R1:\nHALT", &ok, &e);             CHECK(!ok && e.find("reserved") != std::string::npos);
}

static void testCPU() {
    section("CPU instructions");
    CPU c = runSource("MOV R1, 10\nMOV R2, 20\nADD R1, R2\nHALT");
    CHECK(c.registers().R[1] == 30 && c.steps() == 4 && c.state() == CpuState::HALTED && c.registers().PC == 3);
    CHECK(c.history().size() == 4 && c.history().get(2).instruction == "ADD R1, R2");
    c = runSource("MOV R1, 50\nMOV R2, 20\nSUB R1, R2\nHALT");            CHECK(c.registers().R[1] == 30);
    c = runSource("MOV R1, 10\nMOV R2, 20\nPUSH R1\nPUSH R2\nPOP R3\nHALT");
    CHECK(c.registers().R[3] == 20 && c.stack().size() == 1 && c.stack().peek() == 10 && c.registers().SP == 999);
    c = runSource("MOV R1, 5\nLOOP:\nDEC R1\nCMP R1, R0\nJNZ LOOP\nHALT");
    CHECK(c.registers().R[1] == 0 && c.steps() == 17 && c.registers().flags.ZF);
    c = runSource("MOV R2, R1\nMOV R1, 7\nMOV R3, R1\nHALT");              CHECK(c.registers().R[3] == 7);
    c = runSource("MOV R1, 42\nSTORE R1, 100\nLOAD R2, 100\nHALT");
    CHECK(c.memory().read(100) == 42 && c.registers().R[2] == 42);
    c = runSource("MOV R1, 5\nMOV R2, 300\nSTORE R1, [R2]\nLOAD R3, [R2]\nHALT");
    CHECK(c.memory().read(300) == 5 && c.registers().R[3] == 5);
    c = runSource("INC R1\nINC R1\nDEC R2\nHALT");
    CHECK(c.registers().R[1] == 2 && c.registers().R[2] == -1 && c.registers().flags.SF);
    c = runSource("MOV R1, 3\nCMP R1, 3\nHALT");                            CHECK(c.registers().flags.ZF && !c.registers().flags.CF);
    c = runSource("MOV R1, 2\nCMP R1, 3\nHALT");                            CHECK(!c.registers().flags.ZF && c.registers().flags.SF && c.registers().flags.CF);
    c = runSource("MOV R1, 2147483647\nADD R1, 1\nHALT");                   CHECK(c.registers().flags.OF && c.registers().flags.SF);
    c = runSource("MOV R1, -1\nADD R1, 1\nHALT");                           CHECK(c.registers().flags.CF && c.registers().flags.ZF && !c.registers().flags.OF);
    c = runSource("JMP SKIP\nMOV R1, 1\nSKIP: MOV R2, 2\nHALT");            CHECK(c.registers().R[1] == 0 && c.registers().R[2] == 2);
    c = runSource("MOV R1, 1\nCMP R1, 1\nJZ Y\nMOV R5, 9\nY: HALT");        CHECK(c.registers().R[5] == 0);
    c = runSource("MOV R1, 1\nCMP R1, 2\nJZ Y\nMOV R5, 9\nY: HALT");        CHECK(c.registers().R[5] == 9);
    c = runSource("MOV R1, 1\nCMP R1, 2\nJNZ Y\nMOV R5, 9\nY: HALT");       CHECK(c.registers().R[5] == 0);
    c = runSource("NOP\nNOP\nHALT");                                        CHECK(c.steps() == 3 && c.state() == CpuState::HALTED);
    c = runSource("MOV R1, 1\nHALT\nMOV R1, 2");                            CHECK(c.registers().R[1] == 1);
    c = runSource("MOV R1, 1");                                              // no HALT
    CHECK(c.state() == CpuState::HALTED && c.statusMessage().find("without HALT") != std::string::npos);
    // runtime errors
    c = runSource("POP R1\nHALT");
    CHECK(c.state() == CpuState::ERROR && c.lastError().find("underflow") != std::string::npos);
    c = runSource("LOAD R1, 5000\nHALT");
    CHECK(c.state() == CpuState::ERROR && c.lastError().find("Invalid memory address") != std::string::npos);
    c = runSource("L: PUSH R1\nJMP L");
    CHECK(c.state() == CpuState::ERROR && c.lastError().find("overflow") != std::string::npos && c.stack().size() == CPU::STACK_CAPACITY);
    c = runSource("L: JMP L");                                              // infinite loop -> step limit
    CHECK(c.state() == CpuState::PAUSED && c.steps() == CPU::DEFAULT_STEP_LIMIT);
    CPU empty; CHECK(!empty.step() && empty.lastError().find("Empty program") != std::string::npos);
}

static void testQueueDuringExecution() {
    section("Instruction queue during execution");
    DynamicArray<std::string> l; l.pushBack("MOV R1, 2"); l.pushBack("L: DEC R1"); l.pushBack("JNZ L"); l.pushBack("HALT");
    AssemblyResult a = Assembler::assemble(l);
    CPU c; c.load("q.asm", a.instructions, a.symbols);
    CHECK(c.queue().size() == 4 && c.queue().peek().address == 0);
    c.step();                                           // MOV dequeued
    CHECK(c.queue().size() == 3 && c.queue().peek().address == 1);
    c.step(); c.step();                                 // DEC, JNZ taken -> flush + refill from 1
    CHECK(c.registers().PC == 1 && c.queue().peek().address == 1 && c.queue().size() == 3 && c.queueFlushes() == 1);
    c.setBreakpoint(3);
    std::string why = c.run();
    CHECK(why.find("breakpoint") != std::string::npos && c.registers().PC == 3);
    c.run();
    CHECK(c.state() == CpuState::HALTED);
    c.reset();
    CHECK(c.steps() == 0 && c.history().empty() && c.queue().size() == 4 && c.registers().R[1] == 0);
}

static void testProgramManagerUndoRedo() {
    section("Repository, ProgramManager, undo/redo");
    std::string err, dir = TMP + "/programs", idx = TMP + "/data/index.csv";
    FileManager::ensureDirectory(dir, err);
    FileManager::removeFile(idx, err);
    DynamicArray<std::string> old = FileManager::listFiles(dir, ".asm");
    for (std::size_t i = 0; i < old.size(); ++i) FileManager::removeFile(FileManager::join(dir, old[i]), err);

    ProgramRepository repo(idx, dir);
    CHECK(repo.load() && FileManager::exists(idx));   // missing index created
    ProgramManager pm(repo, dir);
    CHECK(pm.create("alpha", err) && pm.current().name == "alpha.asm");
    pm.addLine("MOV R1, 1");
    pm.addLine("HALT");
    CHECK(pm.insertLine(1, "INC R1", err));
    CHECK(pm.current().lines.size() == 3 && pm.current().lines[1] == "INC R1");
    CHECK(pm.undo(err) && pm.current().lines.size() == 2);      // undo insert
    CHECK(pm.undo(err) && pm.current().lines.size() == 1);      // undo add HALT
    CHECK(pm.redo(err) && pm.current().lines.size() == 2 && pm.current().lines[1] == "HALT");
    CHECK(pm.modifyLine(0, "MOV R1, 5", err));                  // new edit clears redo
    CHECK(!pm.redo(err) && err == "Nothing to redo.");
    CHECK(!pm.modifyLine(9, "x", err) && !pm.deleteLine(9, err));
    CHECK(pm.save(err) && FileManager::exists(dir + "/alpha.asm"));
    ProgramRecord* r = repo.find("alpha.asm");
    CHECK(r && r->instructionCount == 2 && r->status == "OK" && r->fileSize > 0);
    CHECK(!pm.create("alpha.asm", err));                        // already exists
    CHECK(!pm.create("bad/name", err));
    CHECK(pm.saveAs("beta", false, err) && FileManager::exists(dir + "/beta.asm"));
    pm.addLine("NOP"); pm.addLine("NOP"); pm.save(err);
    CHECK(!pm.saveAs("alpha", false, err));                     // overwrite needs confirmation
    for (int i = 0; i < 60; ++i) pm.addLine("NOP");            // undo capacity is 50
    CHECK(pm.undoStack().size() == ProgramManager::MAX_UNDO);
    pm.save(err);
    repo.recordExecution("alpha.asm", "2026-01-01 10:00:00");
    // persistence round trip
    ProgramRepository repo2(idx, dir);
    CHECK(repo2.load() && repo2.size() == 2);
    ProgramRecord* a2 = repo2.find("alpha.asm");
    CHECK(a2 && a2->executionCount == 1 && a2->lastExecution == "2026-01-01 10:00:00");
    // sorting repository
    SortStats s = repo2.sort(SortMethod::SELECTION, SortKey::INSTRUCTIONS, true);
    CHECK(repo2.list().get(0).name == "beta.asm" && s.comparisons == 1);
    repo2.sort(SortMethod::BUBBLE, SortKey::NAME);
    CHECK(repo2.list().get(0).name == "alpha.asm");
    DynamicArray<ProgramRecord> sorted; SortStats ss;
    CHECK(repo2.binarySearchByName("beta.asm", sorted, ss).index == 1);
    CHECK(repo2.binarySearchByName("gamma.asm", sorted, ss).index == -1);
    long cmp = 0;
    CHECK(repo2.searchByName("ET", cmp).size() == 1 && cmp == 2);
    // corrupted metadata is skipped, not fatal
    FileManager::appendLine(idx, "garbage,row", err);
    FileManager::appendLine(idx, "x,y,z,notnumber,1,2,OK,never", err);
    ProgramRepository repo3(idx, dir);
    CHECK(repo3.load() && repo3.size() == 2 && repo3.warnings.size() == 2);
    // delete
    ProgramManager pm3(repo3, dir);
    CHECK(pm3.remove("beta", err) && !FileManager::exists(dir + "/beta.asm") && repo3.size() == 1);
    CHECK(!pm3.open("beta", err) && err.find("does not exist") != std::string::npos);
    // directory sync picks up new files
    DynamicArray<std::string> g; g.pushBack("HALT");
    FileManager::writeLines(dir + "/gamma.asm", g, err);
    CHECK(repo3.syncWithDirectory() == 1 && repo3.find("gamma.asm"));
}

static void testReportsSnapshots() {
    section("Reports & snapshots");
    std::string err, path;
    CPU c = runSource("MOV R1, 10\nMOV R2, 20\nADD R1, R2\nSTORE R1, 100\nHALT");
    CHECK(ReportManager::saveReport(c, TMP + "/reports", path, err));
    DynamicArray<std::string> lines;
    FileManager::readLines(path, lines, err);
    std::string all;
    for (std::size_t i = 0; i < lines.size(); ++i) all += lines[i] + "\n";
    CHECK(all.find("R1 = 30") != std::string::npos && all.find("[100] = 30") != std::string::npos &&
          all.find("Program completed successfully") != std::string::npos && all.find("Step 5") != std::string::npos);
    CHECK(ReportManager::saveHistory(c, TMP + "/reports", path, err));
    CPU none; CHECK(!ReportManager::saveReport(none, TMP + "/reports", path, err));

    FileManager::ensureDirectory(TMP + "/snapshots", err);
    std::string snap = TMP + "/snapshots/s.dat";
    CHECK(SnapshotManager::save(c, snap, err));
    CPU d = runSource("MOV R1, 1\nMOV R2, 2\nADD R1, R2\nNOP\nHALT");   // same length, different state
    CHECK(SnapshotManager::load(d, snap, err));
    CHECK(d.registers().R[1] == 30 && d.memory().read(100) == 30 && d.registers().PC == 4);
    CHECK(!SnapshotManager::load(d, TMP + "/snapshots/missing.dat", err));
    FileManager::writeText(TMP + "/snapshots/bad.dat", "REG R1 5\nMEM 5000 1\nEND\n", err);
    CHECK(!SnapshotManager::load(d, TMP + "/snapshots/bad.dat", err) && err.find("invalid memory address") != std::string::npos);
    CHECK(d.registers().R[1] == 30);          // nothing applied on failure
    FileManager::writeText(TMP + "/snapshots/trunc.dat", "REG R1 5\n", err);
    CHECK(!SnapshotManager::load(d, TMP + "/snapshots/trunc.dat", err) && err.find("END") != std::string::npos);
    std::string n1 = SnapshotManager::nextPath(TMP + "/snapshots");
    CHECK(n1.find("snapshot_001.dat") != std::string::npos);
}

static void testDemoPrograms() {
    section("Shipped demo programs");
    struct Case { const char* file; int reg; int value; };
    Case cases[] = {{"addition.asm", 1, 30}, {"subtraction.asm", 1, 30}, {"stack_demo.asm", 3, 20},
                    {"loop.asm", 1, 0}, {"factorial.asm", 2, 120}, {"array_sum.asm", 4, 150}, {"demo.asm", 7, 4}};
    for (const Case& k : cases) {
        DynamicArray<std::string> lines; std::string err;
        if (!FileManager::readLines(std::string("programs/") + k.file, lines, err)) { std::cout << "  (skipped: run tests from project root) " << err << "\n"; CHECK(false); continue; }
        AssemblyResult a = Assembler::assemble(lines);
        CHECK(a.ok());
        CPU c; c.load(k.file, a.instructions, a.symbols); c.run();
        CHECK(c.state() == CpuState::HALTED && c.registers().R[k.reg] == k.value);
    }
    DynamicArray<std::string> lines; std::string err;
    FileManager::readLines("programs/errors_demo.asm", lines, err);
    CHECK(Assembler::assemble(lines).errors.size() == 5);
}

int main() {
    testDynamicArray();
    testLinkedList();
    testStack();
    testQueue();
    testHashTable();
    testSearchSort();
    testFiles();
    testAssembler();
    testCPU();
    testQueueDuringExecution();
    testProgramManagerUndoRedo();
    testReportsSnapshots();
    testDemoPrograms();
    std::cout << "\n" << g_pass << " checks passed, " << g_fail << " failed.\n";
    return g_fail ? 1 : 0;
}
