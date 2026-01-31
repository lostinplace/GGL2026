

# Chess Plugin Design Guide (Unreal Engine)

## 1) Goals and non-goals

### Goals

* Generic system to represent **board**, **pieces**, and **moves** for standard chess.
* **Headless game logic** (no dependency on Actors/meshes/UI).
* Piece appearance is **overrideable via inheriting Blueprints**.
* Deterministic move generation (supports replay, AI, multiplayer later).

### Non-goals (initial phase)

* AI search, networking, clocks, PGN export (can be added later).
* Non-standard variants (architecture should not block them).

---

## 2) Architecture overview

### Layered design

1. **Logic Layer (Headless, UObject/Struct)**

   * Board state + piece registry
   * Move generation + validation
   * Apply/undo
   * Game-end checks

2. **Presentation Layer (Actors)**

   * Board actor: spawns pieces, maps coords → world transforms, handles input
   * Piece actors: purely visual, overrideable in child BPs



**Hard rule:** legality and rules live in the **logic layer only**.

---

## 3) Data model

### 3.1 Coordinates

Create a BlueprintType struct:

**`FBoardCoord`**

* `int32 File` (0–7)
* `int32 Rank` (0–7)

Utility functions (BlueprintCallable):

* `ToIndex()` => `Rank * 8 + File`
* `FromIndex(int32)`
* `IsValid()`
* (Optional) `ToAlgebraic()` / `FromAlgebraic()`

### 3.2 Enumerations

BlueprintType enums:

* `EPieceType` = King, Queen, Rook, Bishop, Knight, Pawn
* `EPieceColor` = White, Black
* `ESpecialMoveType` = Normal, Promotion, Castle, EnPassant

### 3.3 Piece instance

BlueprintType struct:

**`FPieceInstance`**

* `int32 PieceId`
* `EPieceType Type`
* `EPieceColor Color`
* `bool bHasMoved`

(Optionals for later)

* `bool bIsCaptured`
* `FBoardCoord CurrentCoord` (can be derived from Squares[]; keep optional)

### 3.4 Board state

BlueprintType struct or UObject (choose based on BP complexity; recommended: UObject for behavior)

**`FChessBoardState`** (or `UChessBoardState`)

* `TArray<int32> Squares` length 64

  * stores PieceId or `-1` if empty
* `TArray<FPieceInstance> Pieces` indexed by PieceId
* `EPieceColor SideToMove`

* En-passant:

  * `bool bHasEnPassantTarget`
  * `FBoardCoord EnPassantTarget`
* Move counters (optional initially):

  * `int32 HalfmoveClock`
  * `int32 FullmoveNumber`

Board access helpers:

* `GetPieceIdAt(FBoardCoord)`
* `SetPieceIdAt(FBoardCoord, int32 PieceId)`
* `FindCoordOfPiece(int32 PieceId)` (optional; can scan Squares)

---

## 4) Move representation

BlueprintType struct:

**`FChessMove`**

* `FBoardCoord From`
* `FBoardCoord To`
* `int32 MovingPieceId`
* `int32 CapturedPieceId` (or -1)
* `ESpecialMoveType SpecialType`

**Required for undo (store inside an “undo record”):**

**`FMoveUndoRecord`**

* Previous en-passant target
* Previous bHasMoved of moving piece (and rook if castling)
* Captured piece state + coord (or enough to restore)
* Any other derived state needed to rollback

---

## 5) Rules and game model

### 5.1 Rule set object

Blueprintable UObject:

**`UChessRuleSet`**
Responsibilities:

* Provide starting position (spawn pieces into state)
* Generate pseudo-legal moves
* Filter to legal moves (king safety)
* Detect end state: checkmate, stalemate, insufficient material (later)

Recommended API:

* `SetupInitialPosition(UChessGameModel* Model)` or `SetupInitialBoardState(BoardState)`
* `GeneratePseudoLegalMoves(BoardState, PieceId, OutMoves)`
* `IsMoveLegal(BoardState, Move)` (optional if filtering via simulation)
* `IsKingInCheck(BoardState, Color)`

### 5.2 Move generators (pluggable)

Blueprintable UObjects:

* `UMoveGenerator_Sliding` (rook/bishop/queen)
* `UMoveGenerator_Knight`
* `UMoveGenerator_King`
* `UMoveGenerator_Pawn`

`UChessRuleSet` holds mapping:

* `TMap<EPieceType, UMoveGeneratorBase*>`

Each generator:

* `GeneratePseudoMoves(BoardState, FromCoord, Piece, OutMoves)`

**Pawn generator must handle:**

* forward move (1)
* initial 2-step (if not moved + squares clear)
* diagonal captures
* en-passant (based on EnPassantTarget)
* promotion (moves reaching back rank generate Promotion moves)

**King generator must handle:**

* normal king moves
* castling pseudo-moves (then legality filter ensures squares aren’t attacked)

### 5.3 Legal move filtering (king safety)

Approach:

1. Generate pseudo-legal moves for a piece.
2. For each move:

   * Apply move to a temp state (or apply + undo on same state)
   * If own king is in check after move ⇒ discard
3. Remaining moves are legal.

This avoids embedding king-safety rules in every generator.

---

## 6) Game model (the orchestrator)

Blueprintable UObject:

**`UChessGameModel`**
Owns:

* `UChessRuleSet* RuleSet`
* Board state

Core methods:

* `Initialize(UChessRuleSet* InRuleSet)`
* `GetLegalMovesForCoord(FBoardCoord Coord, OutMoves)`
* `GetLegalMovesForPiece(int32 PieceId, OutMoves)`
* `TryApplyMove(FChessMove Move, OutResult)` (returns bool + reason)
* `ApplyMoveInternal(Move, OutUndoRecord)`
* `UndoMove(UndoRecord)`

Events (BlueprintAssignable):

* `OnMoveApplied(FChessMove Move)`
* `OnPieceCaptured(int32 PieceId)`
* `OnPromotion(int32 PieceId, EPieceType NewType)`
* `OnTurnChanged(EPieceColor SideToMove)`
* `OnGameEnded(EGameEndReason Reason, EPieceColor WinnerOrNone)`

**Important:** Presentation is not referenced directly. The model just broadcasts.

---

## 7) Presentation layer (Actors)

### 7.1 Board actor

Actor class:

**`AChessBoardActor`**
Responsibilities:

* Create/own `UChessGameModel`
* Map board coords to world transforms
* Spawn, move, remove piece actors based on model events
* Handle input (select piece, choose destination)
* Highlight legal moves (via decals, widgets, instanced meshes, etc.)

Config:

* `UChessPieceStyleSet* StyleSet` (see below)
* Board layout parameters: square size, origin, orientation (white-bottom vs black-bottom)

Key functions:

* `WorldToCoord(FVector WorldLocation) -> FBoardCoord`
* `CoordToWorld(FBoardCoord) -> FTransform`
* `HandleSquareClicked(FBoardCoord)`
* `SyncFromModel()` (rebuild visuals from state, useful for reset)

### 7.2 Piece actor (visual only)

Actor class:

**`AChessPieceActor`**
Properties:

* `int32 PieceId`
* `EPieceType Type`
* `EPieceColor Color`

Events:

* `BlueprintImplementableEvent OnInitialized(Type, Color, PieceId)`
* `BlueprintImplementableEvent OnMoved(From, To, MoveType)`
* `BlueprintImplementableEvent OnCaptured()`
* `BlueprintImplementableEvent OnPromoted(NewType)`

**Overrideable appearance requirement:**

* Base class contains minimal components (e.g., SceneRoot)
* Child Blueprints add Mesh/Niagara/SFX/etc.

### 7.3 Style set (factory for piece classes)

Data asset:

**`UChessPieceStyleSet`**

* Maps (Color, Type) → `TSubclassOf<AChessPieceActor>`
* Optional: board material, highlight assets, SFX sets, etc.

Board actor uses it to spawn correct piece blueprint classes.

---

## 8) Interaction flow

1. Player clicks a square / piece.
2. Board actor identifies `FBoardCoord`.
3. If selecting a piece:

   * Ask model for legal moves from that coord.
   * Highlight targets.
4. If selecting a target:

   * Build `FChessMove` (or ask model to build from From/To + optional promotion).
   * `TryApplyMove`
5. Model applies and broadcasts events.
6. Board actor animates visuals accordingly.

Promotion:

* If move is promotion:

  * Either (A) pre-choose in UI before applying, or
  * (B) apply a “pending promotion” and block until choice
    Recommended for simplicity: UI chooses promotion type before calling `TryApplyMove`.

---

## 9) Determinism, replay, networking readiness

* Treat the **move list** as the authoritative history.
* For multiplayer later: replicate `FChessMove` from server to clients.
* Visual actors derive from model state and moves, not from replicated transforms.

---

## 10) Testing and debug hooks (strongly recommended)

Add developer utilities:

* “Print board” to log (8x8 ascii)
* Perft-style move count checks (later)
* FEN import/export (very useful even if not exposed to players)

---

## 11) Implementation plan (phased)

### Phase 1: Minimal playable chess

* Board state + piece registry
* Pseudo-legal moves for all pieces (exclude castling/en-passant at first if needed)
* Legal filtering by king safety
* Apply move, capture
* Board actor + piece actor spawns and moves pieces
* Basic selection + highlight + move

### Phase 2: Full rules

* Castling
* En-passant
* Promotion with UI
* Check/checkmate/stalemate detection

### Phase 3: Product polish

* Move history + undo
* FEN load/save
* SFX/VFX hooks
* Optional AI opponent scaffolding

---

## 12) Key invariants (engineer checklist)

* `Squares` and `Pieces` must never disagree.
* `TryApplyMove` must reject illegal moves even if UI tries them.
* Legal move generation must consider:

  * pinned pieces
  * moving into check
  * castling through check
  * en-passant exposing king (common pitfall)

