#include "GameTree.hh"
#include <memory>

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

};
