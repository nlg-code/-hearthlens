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
2. Launch `HearthTracker.exe`. On first run it'll ask for the Logs folder
   if it can't auto-detect it.
3. Click **Load Deck (.txt)** to import a decklist. Simple format:

   ```
   # comment
   CS2_029 x2     # Fireball, two copies
   EX1_277 x2     # Arcane Missiles
   ...
   ```

   Use Hearthstone card IDs from https://api.hearthstonejson.com/.
   A real version would decode deckstrings — left as an exercise.

4. Start a match in Hearthstone. The overlay updates as cards leave your deck
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

## Known limitations / next steps

- **Deckstring decoding**: real trackers paste-import deckstrings (base64,
  varint-encoded). Add a decoder so users can paste the in-game export string.
- **Local player detection**: assumes playerId=1 is you. Power.log includes a
  `PLAYER_ID` tag change you can parse to be sure (especially when going second).
- **Mulligan handling**: cards in your mulligan briefly leave then re-enter the
  deck. Current code treats both events as draws/returns — usually nets out, but
  flickers in the UI. Track mulligan zone explicitly.
- **Block tracking**: many effects (Webspinner, Discover, etc.) need block_start/
  block_end context. Skipped here.
- **Battlegrounds / Arena / Mercenaries**: different log patterns. Add modes.
- **Click-through**: right now the overlay blocks clicks on the game underneath.
  Add `Qt::WindowTransparentForInput` with a hover-to-interact toggle.
- **Card art**: download card images from HearthstoneJSON's `/render/latest/`
  endpoints and show tile previews.
- **Draw probability**: count remaining deck size, compute P(draw next turn) for
  each card.
- **Opponent archetype prediction**: maintain a small DB of known meta lists,
  match revealed cards against them.
