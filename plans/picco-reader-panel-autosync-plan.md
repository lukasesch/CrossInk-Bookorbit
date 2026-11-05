# Picco reader panel gesture and BookOrbit auto-sync on first open

**Target agent:** `implementer` (Opus 5.5 medium). Orchestrator does all git steps and the final review.

## Goal

1. In the reader on the BOOX Picco, a down-swipe starting at the top edge opens the frontlight quick panel (brightness,
   warmth, dark mode, BookOrbit sync), as on the X4 Pro. A down-swipe anywhere else still opens the reader menu.
2. A book downloaded from the BookOrbit catalog takes the server's reading position on its first open, without the user
   opening any menu. On every board.

## Facts (verified 2026-11-05)

- Tree: `CrossInk-Bookorbit`, branch `picco`, HEAD `d89a25a1`. Do not edit `freeink-sdk`. Repo rules in `AGENTS.md` apply
  (capability gating, `tr()`, no generated-file edits, C3 memory limits).
- Picco: touch + frontlight + slide switch, **no home key**, multi-touch works. `Display > Frontlight > Brightness & Warmth`
  already opens the panel from Settings (`d89a25a1`, `ActivityManager::openFrontlightPanelFrom`).
- Reader drawer rows for sync exist only on button devices: `src/activities/reader/EpubReaderMenuModel.h:224`
  (`if (available.buttonDevice)`). Touch boards reach sync through the panel: `FrontlightPanelActivity::openSyncDialog()`
  (`src/activities/util/FrontlightPanelActivity.cpp:247`).
- Gestures, `src/MappedInputManager.cpp`: `wasTopEdgeDownSwipe()` `:741`; `wasMenuGesture()` `:773` (no home key → top edge);
  `wasReaderMenuGesture()` `:777` (no home key, not Sticky → any `SwipeDir::Down`); `wasReaderLightPanelGesture()` `:802`
  (needs home key unless Sticky); `wasLightPanelGesture()` `:843` (needs home key). `ActivityManager.cpp:563-575` checks the
  light-panel gesture before the activity loop and returns. `src/activities/reader/ReaderUtils.h:214` also gates on
  `hasHomeKey()`.
- BookOrbit: `src/BookOrbitDownloadIndex.{h,cpp}` records catalog downloads (book id → path, size-checked).
  Smart-sync rules are pure in `src/util/BookOrbitSmartSync.h` with tests in `test/bookorbit_smart_sync`. Sync runs in
  `src/activities/reader/BookOrbitSyncActivity.{h,cpp}`; Sync on Sleep runs it without UI (see `main.cpp`, `SilentRestart.h`,
  README "Syncing when the device goes to sleep"). Per-book sync marker sits next to the book cache.
- README "Smart sync": the first sync of a book on a device always asks, by design, so a position is never overwritten
  unnoticed.
- X3/X4 (`default`) flash is at 99.4% (~36 KB free). Shared code must stay small.
- Native tests: `cmake -S test -B build/test` is configured; `cmake --build build/test --target <T>` then run the binary.

## Steps

### 1. Reader top-edge panel gesture

- On touch boards **without** a home key that have a frontlight (today: the Picco), in the reader: top-edge down-swipe →
  `openFrontlightPanel`; any other down-swipe → reader menu, as now. The top-edge swipe must not also open the menu.
- Gate by capability (touch, no home key, frontlight present, not the Sticky's own scheme), not by `FREEINK_DEVICE_PICCO`.
  X4 Pro, Sticky, X4 Classic, X3/X4 behaviour unchanged.
- Outside the reader, leave the gestures as they are; report whether Home also has a reachable panel route on the Picco.
- If the gesture choice lives in a pure function, add a test case; otherwise reason it out in the report.

### 2. Auto-sync on first open of a catalog download

- Trigger: opening an EPUB that `BookOrbitDownloadIndex` knows as a catalog download, with no BookOrbit sync marker yet
  for this device, and an account configured. Runs once per book; any completed sync (manual, sleep, auto) ends it.
- Decision: a pure rule in `BookOrbitSmartSync.h`: with no marker and the local position at the book's start (first
  spine item, first page, or no saved progress), apply the server position when the server has one. Anything else falls
  back to the existing rules. Add test cases in `test/bookorbit_smart_sync`, including the seeded-bug check (rule removed →
  red). This rule may also apply to Smart sync and Sync on Sleep, since applying remote over an untouched book loses
  nothing; say in the report whether you wired it there.
- Mechanism: reuse the non-interactive path Sync on Sleep uses (or the sync activity in an auto mode) instead of a second
  network client. It joins saved Wi-Fi, syncs (progress, highlights, bookmarks), jumps to the server position, and returns
  to the book. Show a brief status the way existing syncs do; never block reading on a failure: no Wi-Fi, server down or
  no server progress → open the book at the start, silently except a log line, and retry on the next open while the book
  still has no marker and is still at its start.
- Battery gate: same as Sync on Sleep (skip below 10% without charger).
- Settings: no new setting unless needed; if you add one, default on, under `Settings > System > BookOrbit Sync`.

### 3. Docs

- README "Syncing reading progress": the auto-sync on first open of catalog downloads.
- `docs/controls.md`: the Picco reader top-edge gesture.
- `CHANGELOG.md` `[Unreleased]` entries.

### 4. Build and test

- `cmake --build build/test --target <touched test targets>` and run them.
- One build command, foreground, timeout 600000 ms, log outside the repo: `~/.local/bin/pio run -e picco -e x4-pro -e default`;
  at most 3 runs. Report flash size per env; `default` must keep at least 30 KB free.
- Format only the changed lines: `git clang-format --diff --extensions h,cpp | git apply` (local clang-format is 23, CI 21;
  this is a read of git state plus a working-tree patch, allowed). Do not stage or commit.

## Out of scope

Reader drawer row changes, Home-screen gesture changes, SDK edits, release/CI, any flash or upload command.

## Stop and report instead of guessing

A cache/marker format change, a needed SDK change, `default` dropping below 30 KB free, or a UX decision not covered here.

## Report

Findings, the gesture matrix per board, the auto-sync flow (trigger, decision, failure handling), test results, build sizes
per env, changed files. No diff.
