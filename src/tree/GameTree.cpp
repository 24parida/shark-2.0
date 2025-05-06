#include "GameTree.hh"
#include "game/Game.hh"

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
                            std::vector<std::unique_ptr<Node>> &children,
                            std::vector<Action> &actions) {

  if (is_valid_action(action, state.current.stack, state.current.wager,
                      state.get_call_amount(), state.minimum_raise_size)) {
    std::unique_ptr<Node> child;
    std::unique_ptr<GameState> nxt_state = std::make_unique<GameState>(state);
    bool bets_setteld = nxt_state->apply_action(action);

    if (bets_setteld) {
      if (nxt_state->is_uncontested() || nxt_state->both_all_in() ||
          state.street == Street::RIVER) {
        child = build_term_nodes(node, std::move(nxt_state));
      } else {
        child = build_chance_nodes(node, std::move(nxt_state));
      }
    } else {
      child = build_action_nodes(node, std::move(nxt_state));
    }

    children.push_back(std::move(child));
    actions.push_back(action);
  }
}

auto GameTree::build_action_nodes(Node parent, std::unique_ptr<GameState> state)
    -> std::unique_ptr<Node> {

  return std::make_unique<Node>();
}
auto GameTree::build_chance_nodes(Node parent, std::unique_ptr<GameState> state)
    -> std::unique_ptr<Node> {

  return std::make_unique<Node>();
}
auto GameTree::build_term_nodes(Node parent, std::unique_ptr<GameState> state)
    -> std::unique_ptr<Node> {

  return std::make_unique<Node>();
}
