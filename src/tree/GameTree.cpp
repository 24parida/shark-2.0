#include "GameTree.hh"
#include "card.h"
#include "game/Game.hh"
#include "tree/Nodes.hh"

#include <memory>

bool is_valid_action(const Action &action, const GameState &state);

auto GameTree::get_init_state() const -> GameState {
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

auto GameTree::build_action(std::unique_ptr<ActionNode> node,
                            const GameState &state, const Action &action)
    -> std::unique_ptr<ActionNode> {

  if (!is_valid_action(action, state)) {
    return node;
  }

  GameState nxt_state = {state};
  bool bets_setteld = nxt_state.apply_action(action);
  std::unique_ptr<Node> child{nullptr};

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

  node->push_child(std::move(child));
  node->push_action(action);
  return node;
}

auto GameTree::build_action_nodes(const Node *parent, const GameState &state)
    -> std::unique_ptr<Node> {

  auto action_node = std::make_unique<ActionNode>(parent, state.current._id);
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
      build_action(std::move(action_node), state, action);
    } else {
      // TODO: skipped raise
      for (const auto &size : GameParams::BET_SIZES) {
        int bet_amount{static_cast<int>(size * state.pot)};
        bet_amount =
            bet_amount > state.current.stack ? state.current.stack : bet_amount;

        // TODO: all in thresholdstack
        Action action{.type = i, .amount = bet_amount};
        build_action(std::move(action_node), state, action);
      }
    }
  }

  const int curr_num_hands{state.current._id == 1 ? m_p1_num_hands
                                                  : m_p2_num_hands};
  action_node->init(curr_num_hands);
  return action_node;
}

auto GameTree::build_chance_nodes(const Node *parent, const GameState &state)
    -> std::unique_ptr<Node> {
  using phevaluator::Card;
  using ChanceType = ChanceNode::ChanceType;

  assert(state.street == Street::FLOP ||
         state.street == Street::TURN &&
             "build_chance_node error incorrect street");
  ChanceNode::ChanceType type{state.street == Street::FLOP
                                  ? ChanceType::DEAL_TURN
                                  : ChanceType::DEAL_RIVER};

  auto chance_node = std::make_unique<ChanceNode>(parent, type);

  for (int i{0}; i < 52; ++i) {
    Card card{i};

    // TODO: if card in board skip
    GameState nxt_state{state};

    nxt_state.street =
        static_cast<Street>(static_cast<int>(nxt_state.street) + 1);

    std::unique_ptr<Node> action_node{
        build_action_nodes(chance_node.get(), nxt_state)};
    chance_node->add_child(std::move(action_node), card);
  }

  return chance_node;
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

  auto terminal_node = std::make_unique<TerminalNode>(parent, type);

  const Node *node{parent};
  while (!dynamic_cast<const ActionNode *>(node)) {
    node = node->get_parent();
  }

  assert(node && "terminal_node no viable action_node_parent");
  const ActionNode *action_parent = dynamic_cast<const ActionNode *>(node);

  terminal_node->set_last_to_act(action_parent->get_player());
  return std::move(terminal_node);
}

bool is_valid_action(const Action &action, const GameState &state) {
  const int call_amount = state.get_call_amount();
  const int stack = state.current.stack;
  const int minimum_raise_size = state.minimum_raise_size;
  const int wager = state.current.wager;

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
