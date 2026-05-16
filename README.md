# HearthTracker

Minimal Hearthstone deck tracker in Qt 6 / C++17. Tails `Power.log` in real time,
parses zone changes, and shows an always-on-top overlay with cards remaining in
your deck and cards your opponent has revealed.

## Build

Requirements: Qt 6.x, CMake 3.16+, a C++17 compiler.

```
cmake -B build -DCMAKE_PREFIX_PATH=<path/to/Qt6>
cmake --build build --config Release
```

On Windows with Qt installed at e.g. `C:/Qt/6.7.0/msvc2019_64`:

```
cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
cmake --build build --config Release
```

## Run

1. Make sure `log.config` is in place and Hearthstone is writing to
   `Logs/Hearthstone_*/Power.log` (you've already done this).
2. Launch `HearthTracker.exe`.

3. Start a match in Hearthstone. The overlay updates as cards leave your deck
   and as your opponent reveals cards.

## Architecture

```
SessionManager       polls Logs/ for new Hearthstone_YYYY_MM_DD_HH_MM_SS folders
       │
       ▼
LogWatcher           tails Power.log, emits one line at a time
       │
       ▼
PowerLogParser       regex over lines → ParsedEntity, zoneChanged, gameCreated
       │
       ▼
GameState            decrements deck counts, accumulates opponent reveals
       │
       ▼
OverlayWindow        frameless, always-on-top, draggable title bar
```

`CardDatabase` fetches/caches `cards.collectible.json` from HearthstoneJSON for
card names, costs, etc.

