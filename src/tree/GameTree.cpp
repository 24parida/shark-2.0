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

json GameTree::jsonify_tree(const Node *node) const {
  json ret;
  switch (node->get_node_type()) {
  case NodeType::ACTION_NODE: {
    auto action_node = dynamic_cast<const ActionNode *>(node);
    ret["type"] = "action_node";
    ret["player"] = action_node->get_player();
    ret["children"] = json::array();

    for (int i{0}; i < action_node->get_num_actions(); ++i) {
      const Node *child{action_node->get_child(i)};
      json child_json{jsonify_tree(child)};
      const Action action{action_node->get_action(i)};

      std::string action_type;
      switch (action.type) {
      case Action::ActionType::FOLD:
        action_type = "FOLD";
        break;
      case Action::ActionType::CHECK:
        action_type = "CHECK";
        break;
      case Action::ActionType::CALL:
        action_type = "CALL";
        break;
      case Action::ActionType::BET:
        action_type = "BET";
        break;
      case Action::ActionType::RAISE:
        action_type = "RAISE";
        break;
      }

      child_json["action"] = action_type;
      child_json["amount"] = action.amount;

      ret["children"].push_back(child_json);
    }
  } break;
  case NodeType::CHANCE_NODE: {
    auto chance_node = dynamic_cast<const ChanceNode *>(node);
    ret["type"] = "chance_node";
    ret["children"] = json::array();

    std::string chance_type{chance_node->get_type() ==
                                    ChanceNode::ChanceType::DEAL_TURN
                                ? "DEAL_TURN"
                                : "DEAL_RIVER"};
    ret["chance_type"] = chance_type;

    for (int i{0}; i < chance_node->get_num_children(); ++i) {
      const auto child{chance_node->get_child(i)};
      if (!child)
        continue;
      json child_json{jsonify_tree(child)};
      ret["children"].push_back(child_json);
    }
  } break;
  case NodeType::TERMINAL_NODE: {
    auto terminal_node = dynamic_cast<const TerminalNode *>(node);

    std::string term_type;
    switch (terminal_node->get_type()) {
    case TerminalNode::TerminalType::ALLIN:
      term_type = "ALLIN";
      break;
    case TerminalNode::TerminalType::UNCONTESTED:
      term_type = "UNCONTESTED";
      break;
    case TerminalNode::TerminalType::SHOWDOWN:
      term_type = "SHOWDOWN";
      break;
    }

    ret["type"] = "terminal_node";
    ret["terminal_type"] = term_type;
    ret["last_to_act"] = terminal_node->get_last_to_act();
    ret["pot"] = terminal_node->get_pot();
  } break;
  }
  return ret;
}
