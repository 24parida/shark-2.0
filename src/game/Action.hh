#pragma once
struct Action {
  enum ActionType { FOLD, CHECK, CALL, BET, RAISE };
  ActionType type;
  int amount;
};
