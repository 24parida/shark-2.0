#include "GameTree.hh"
#include "../Helper.hh"
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
  state.p1 = std::make_shared<PlayerState>(
      1, m_settings.in_position_player == 1, m_settings.starting_stack);
  state.p2 = std::make_shared<PlayerState>(
      2, m_settings.in_position_player == 2, m_settings.starting_stack);
  state.board = m_settings.initial_board;

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
      child = build_term_nodes(node.get(), nxt_state);
    } else {
      child = build_chance_nodes(node.get(), nxt_state);
    }
  } else {
    child = build_action_nodes(node.get(), nxt_state);
  }

  node->push_child(std::move(child));
  node->push_action(action);
  return node;
}

auto GameTree::build_action_nodes(const Node *parent, const GameState &state)
    -> std::unique_ptr<Node> {
  using ActionType = Action::ActionType;
  auto action_node = std::make_unique<ActionNode>(parent, state.current->_id);
  const std::vector<ActionType> types{ActionType::FOLD, ActionType::CHECK,
                                      ActionType::CALL, ActionType::BET,
                                      ActionType::RAISE};

  if (state.street == Street::FLOP)
    ++m_flop_action_node_count;
  else if (state.street == Street::TURN)
    ++m_turn_action_node_count;
  else if (state.street == Street::RIVER)
    ++m_river_action_node_count;

  for (const auto &i : types) {
    if (i == ActionType::FOLD || i == ActionType::CHECK ||
        i == ActionType::CALL) {
      const int amount{i == Action::ActionType::CALL ? state.get_call_amount()
                                                     : 0};
      Action action{.type = i, .amount = amount};
      action_node = build_action(std::move(action_node), state, action);
    } else if (i == ActionType::BET) {
      for (const auto &size : GameParams::BET_SIZES) {
        int bet_amount{static_cast<int>(size * state.pot)};
        bet_amount = bet_amount > state.current->stack ? state.current->stack
                                                       : bet_amount;

        if ((static_cast<float>(bet_amount + state.current->wager) /
             (state.current->stack + state.current->wager)) >=
            m_settings.all_in_threshold) {
          bet_amount = state.current->stack;
          const Action action{.type = i, .amount = bet_amount};
          action_node = build_action(std::move(action_node), state, action);
          break;
        }
        const Action action{.type = i, .amount = bet_amount};
        action_node = build_action(std::move(action_node), state, action);
      }
    } else {
      for (const auto &size : GameParams::RAISE_SIZES) {
        int raise_amount{
            static_cast<int>(state.current->wager + state.get_call_amount() +
                             size * (state.get_call_amount() + state.pot))};
        raise_amount =
            raise_amount > state.current->stack + state.current->wager
                ? state.current->stack + state.current->wager
                : raise_amount;

        if ((static_cast<float>(raise_amount) /
             (state.current->stack + state.current->wager)) >=
            m_settings.all_in_threshold) {
          raise_amount = state.current->stack;
          const Action action{.type = i, .amount = raise_amount};
          action_node = build_action(std::move(action_node), state, action);
          break;
        }
        const Action action{.type = i, .amount = raise_amount};
        action_node = build_action(std::move(action_node), state, action);
      }
    }
  }

  const int curr_num_hands{state.current->_id == 1 ? m_p1_num_hands
                                                   : m_p2_num_hands};
  action_node->init(curr_num_hands);
  return action_node;
}

auto GameTree::build_chance_nodes(const Node *parent, const GameState &state)
    -> std::unique_ptr<Node> {
  using phevaluator::Card;
  using ChanceType = ChanceNode::ChanceType;

  ++m_chance_node_count;

  assert((state.street == Street::FLOP || state.street == Street::TURN) &&
         "build_chance_node error incorrect street");

  const ChanceType type{state.street == Street::FLOP ? ChanceType::DEAL_TURN
                                                     : ChanceType ::DEAL_RIVER};
  auto chance_node = std::make_unique<ChanceNode>(parent, type);

  for (int i{0}; i < GameParams::NUM_CARDS; ++i) {
    Card card{i};
    if (CardUtility::overlap(card, state.board))
      continue;

    GameState nxt_state{state};
    if (chance_node->get_type() == ChanceType::DEAL_TURN) {
      nxt_state.set_turn(card);
    } else if (chance_node->get_type() == ChanceType::DEAL_RIVER) {
      nxt_state.set_river(card);
    }
    nxt_state.go_to_next_street();

    auto action_node{build_action_nodes(chance_node.get(), nxt_state)};
    chance_node->add_child(std::move(action_node), card);
  }

  return chance_node;
}

auto GameTree::build_term_nodes(const Node *parent, const GameState &state)
    -> std::unique_ptr<Node> {
  using TerminalType = TerminalNode::TerminalType;
  ++m_terminal_node_count;

  TerminalType type;
  if (state.both_all_in() && state.street != Street::RIVER) {
    type = TerminalType::ALLIN;
  } else if (state.is_uncontested()) {
    type = TerminalType::UNCONTESTED;
  } else {
    type = TerminalType::SHOWDOWN;
  }

  auto terminal_node = std::make_unique<TerminalNode>(parent, type);
  terminal_node->set_pot(state.pot);

  const Node *node{parent};
  while (node->get_node_type() != NodeType::ACTION_NODE) {
    node = node->get_parent();
  }

  assert(node && "terminal_node no viable action_node_parent");
  const ActionNode *action_parent = dynamic_cast<const ActionNode *>(node);

  terminal_node->set_last_to_act(action_parent->get_player());
  return terminal_node;
}

bool is_valid_action(const Action &action, const GameState &state) {
  const int call_amount = state.get_call_amount();
  const int stack = state.current->stack;
  const int minimum_raise_size = state.minimum_raise_size;
  const int wager = state.current->wager;

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

auto GameTree::getTreeStats() const -> TreeStatistics {
  TreeStatistics stats;
  stats.flop_action_nodes = m_flop_action_node_count;
  stats.turn_action_nodes = m_turn_action_node_count;
  stats.river_action_nodes = m_river_action_node_count;
  stats.total_action_nodes = m_flop_action_node_count + m_turn_action_node_count + m_river_action_node_count;
  stats.chance_nodes = m_chance_node_count;
  stats.terminal_nodes = m_terminal_node_count;
  stats.p1_num_hands = m_p1_num_hands;
  stats.p2_num_hands = m_p2_num_hands;
  return stats;
}

size_t TreeStatistics::estimateMemoryBytes() const {
  // DCFR storage per action node:
  // - 2 bytes per (hand, action) for cumulative regret (int16_t)
  // - 2 bytes per (hand, action) for cumulative strategy (int16_t)
  // Assuming average 3 actions per node
  const int avg_actions_per_node = 3;
  const int bytes_per_hand_action = 4;  // 2 for regret + 2 for strategy

  size_t dcfr_bytes = 0;

  // Estimate for each action node (player perspective matters)
  // Simplified: each action node stores data for both players
  int max_hands = (p1_num_hands > p2_num_hands) ? p1_num_hands : p2_num_hands;
  dcfr_bytes = static_cast<size_t>(total_action_nodes) * max_hands * avg_actions_per_node * bytes_per_hand_action;

  // Tree structure overhead (nodes, pointers, etc.)
  // Rough estimate: 200 bytes per node
  size_t tree_structure_bytes = static_cast<size_t>(total_action_nodes + chance_nodes + terminal_nodes) * 200;

  // Total with 20% overhead for misc data structures
  return static_cast<size_t>((dcfr_bytes + tree_structure_bytes) * 1.2);
}
