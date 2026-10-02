#pragma once

class GameMessage;

// GeneralsX @refactor Codex 02/10/2026 The only synchronized dispatcher entry point for formation orders.
// Execution must not depend on any player's local control-scheme preference.
void executeFormationOrder(const GameMessage *message);
