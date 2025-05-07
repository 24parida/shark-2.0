#include "GameTree.hh"
#include "card.h"
#include "game/Game.hh"
#include "optional"
#include "tree/Nodes.hh"

#include <algorithm>
#include <memory>

bool is_valid_action(const Action action, const int stack, const int wager,
                     const int call_amount, const int minimum_raise_size);

auto GameTree::get_init_state() -> GameState {
  GameState state = {};

  state.street = m_settings.initial_street;
  state.pot = m_settings.starting_pot;
  state.minimum_bet_size = m_settings.minimum_bet;
  state.minimum_raise_size = m_settings.minimum_bet;
  state.p1 = PlayerState(1, m_settings.in_position_player == 1,
                         m_settings.starting_stack);
  state.p2 = PlayerState(2, m_settings.in_position_player == 2,
                         m_settings.starting_stack);

  for (std::size_t i{0}; i < m_settings.initial_board.size(); i++) {
    state.board[i] = m_settings.initial_board[i];
  }

  state.init_current();
  state.init_last_to_act();
  return state;
}

auto GameTree::build() -> std::unique_ptr<Node> {
  std::unique_ptr<Node> root = build_action_nodes(nullptr, get_init_state());
  root->set_parent(root.get());
  return root;
}

// TOOD: I hate the fact that this is a void funciton
void GameTree::build_action(std::unique_ptr<Node> node, const GameState &state,
                            const Action action,
                            std::vector<std::unique_ptr<Node>> &children,
                            std::vector<Action> &actions) {

  if (is_valid_action(action, state.current.stack, state.current.wager,
                      state.get_call_amount(), state.minimum_raise_size)) {

    GameState nxt_state = {state};
    std::unique_ptr<Node> child{nullptr};
    bool bets_setteld = nxt_state.apply_action(action);

    if (bets_setteld) {
      if (nxt_state.is_uncontested() || nxt_state.both_all_in() ||
          state.street == Street::RIVER) {
        child = std::move(build_term_nodes(node.get(), nxt_state));
      } else {
        child = std::move(build_chance_nodes(node.get(), nxt_state));
      }
    } else {
      child = std::move(build_action_nodes(node.get(), nxt_state));
    }

    children.push_back(std::move(node));
    actions.push_back(action);
  }
}

auto GameTree::build_action_nodes(const Node *parent, const GameState &state)
    -> std::unique_ptr<Node> {

  std::unique_ptr<ActionNode> action_node =
      std::make_unique<ActionNode>(parent, state.current._id);

  // TODO: move these types into type alias's
  std::vector<Action> actions;
  std::vector<std::unique_ptr<Node>> children;

  // TODO: set bet_sizes and raise_sizes
  std::vector<double> bet_sizes{};
  std::vector<double> raise_sizes{};

  const std::vector<Action::ActionType> types{
      Action::ActionType::FOLD, Action::ActionType::CHECK,
      Action::ActionType::CALL, Action::ActionType::BET,
      Action::ActionType::RAISE};

  for (const auto &i : types) {
    if (i == Action::ActionType::FOLD || i == Action::ActionType::CHECK ||
        i == Action::ActionType::CALL) {
      const int amount{i == Action::ActionType::CALL ? state.get_call_amount()
                                                     : 0};
      Action action{.type = i, .amount = amount};
      build_action(std::move(action_node), state, action, children, actions);
    } else {
      for (const auto &size : bet_sizes) {
        int bet_amount{static_cast<int>(size * state.pot)};
        bet_amount =
            bet_amount > state.current.stack ? state.current.stack : bet_amount;

        // TODO: all in thresholdstack
        Action action{.type = i, .amount = bet_amount};
        build_action(std::move(action_node), state, action, children, actions);
      }
    }
  }

  const int curr_num_hands{state.current._id == 1 ? m_p1_num_hands
                                                  : m_p2_num_hands};
  action_node->init(children, actions, curr_num_hands);
}

auto GameTree::build_chance_nodes(const Node *parent, const GameState &state)

    -> std::unique_ptr<Node> {
  assert(state.street == Street::FLOP ||
         state.street == Street::TURN && "build_chance_node error: ");
  using phevaluator::Card;
  using ChanceType = ChanceNode::ChanceType;
  ChanceNode::ChanceType type{state.street == Street::FLOP
                                  ? ChanceType::DEAL_TURN
                                  : ChanceType::DEAL_RIVER};
  ChanceNode chance_node{parent, type};

  for (int i{0}; i < 52; ++i) {
    Card card{i};

    // if card in board skip
    GameState nxt_state{state};

    nxt_state.street =
        static_cast<Street>(static_cast<int>(nxt_state.street) + 1);

    std::unique_ptr<Node> action_node{
        build_action_nodes(&chance_node, nxt_state)};
    chance_node.add_child(std::move(action_node), card);
  }
}

auto GameTree::build_term_nodes(const Node *parent, const GameState &state)
    -> std::unique_ptr<Node> {
  using TerminalType = TerminalNode::TerminalType;

  TerminalType type;
  if (state.both_all_in() && state.street != Street::RIVER) {
    type = TerminalType::ALLIN;
  } else if (state.is_uncontested()) {
    type = TerminalType::UNCONTESTED;
  } else {
    type = TerminalType::SHOWDOWN;
  }

  std::unique_ptr<TerminalNode> terminal_node =
      std::make_unique<TerminalNode>(parent, type);

  const Node *node{parent};
  while (!dynamic_cast<const ActionNode *>(node)) {
    node = node->get_parent();
  }

  assert(node && "terminal_node no viable action_node_parent");
  const ActionNode *action_parent = dynamic_cast<const ActionNode *>(node);

  terminal_node->set_last_to_act(action_parent->get_player());
  return std::move(terminal_node);
}

bool is_valid_action(const Action action, const int stack, const int wager,
                     const int call_amount, const int minimum_raise_size) {
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
