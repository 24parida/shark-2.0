
struct Action {
  enum ActionType { FOLD, CHECK, CALL, BET, RAISE };
  ActionType type;
  int amount;

  static bool is_valid_action(Action action, int stack, int wager,
                              int call_amount, int minimum_raise_size) {
    switch (action.type) {
    case FOLD:
      return call_amount > 0;
    case CHECK:
      return call_amount == 0;
    case CALL:
      return call_amount > 0 &&
             ((action.amount == call_amount && action.amount <= stack) ||
              action.amount == stack);
    case BET:
      return call_amount == 0 &&
             ((action.amount > minimum_raise_size && action.amount <= stack) ||
              (action.amount > 0 && action.amount == stack));
    case RAISE:
      int raiseSize = action.amount - call_amount - wager;
      return call_amount > 0 &&
             ((raiseSize >= minimum_raise_size &&
               action.amount <= (stack + wager)) ||
              (raiseSize > 0 && action.amount == (stack + wager)));
    }

    return false;
  }
};
