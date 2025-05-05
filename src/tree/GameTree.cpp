#include "GameTree.hh"

#include <memory>

bool is_valid_action(Action action, int stack, int wager, int call_amount,
                     int minimum_raise_size) {
  switch (action.type) {
  case Action::FOLD:
    return call_amount > 0;
  case Action::CHECK:
    return call_amount == 0;
  case Action::CALL:
    return call_amount > 0 &&
           ((action.amount == call_amount && action.amount <= stack) ||
            action.amount == stack);
  case Action::BET:
    return call_amount == 0 &&
           ((action.amount > minimum_raise_size && action.amount <= stack) ||
            (action.amount > 0 && action.amount == stack));
  case Action::RAISE:
    int raiseSize = action.amount - call_amount - wager;
    return call_amount > 0 &&
           ((raiseSize >= minimum_raise_size &&
             action.amount <= (stack + wager)) ||
            (raiseSize > 0 && action.amount == (stack + wager)));
  }

  return false;
}

auto GameTree::get_init_state() -> std::unique_ptr<GameState> {
  std::unique_ptr<GameState> state = std::make_unique<GameState>();

  state->street = m_settings.initial_street;
  state->pot = m_settings.starting_pot;
  state->minimum_bet_size = m_settings.minimum_bet;
  state->minimum_raise_size = m_settings.minimum_bet;

  state->p1 = PlayerState(1, m_settings.in_position_player == 1,
                          m_settings.starting_stack);
  state->p2 = PlayerState(2, m_settings.in_position_player == 2,
                          m_settings.starting_stack);

  for (std::size_t i{0}; i < m_settings.initial_board.size(); i++) {
    state->board[i] = m_settings.initial_board[i];
  }

  state->init_current();
  state->init_last_to_act();
  return state;
}

auto GameTree::build() -> std::unique_ptr<Node> {
  std::unique_ptr<Node> root = build_action_nodes(nullptr, get_init_state());
  root->set_parent(root.get());
  return root;
}

void GameTree::build_action(const ActionNode node, const GameState state,
                            const Action action,
                            const std::vector<Node> children,
                            const std::vector<Action> actions) {

  if (is_valid_action(action, state.current.stack, state.current.wager,
                      state.get_call_amount(), state.minimum_raise_size)) {
    Node child{nullptr};
  }
};
