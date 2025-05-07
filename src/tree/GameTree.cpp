#include "GameTree.hh"
#include "card.h"
#include "game/Game.hh"
#include "optional"
#include "tree/Nodes.hh"

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

void GameTree::build_action(std::unique_ptr<Node> node, const GameState state,
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
        child = build_term_nodes(&node, std::move(nxt_state));
      } else {
        child = build_chance_nodes(&node, std::move(nxt_state));
      }
    } else {
      child = build_action_nodes(&node, std::move(nxt_state));
    }

    children.push_back(std::move(child));
    actions.push_back(action);
  }
}

auto GameTree::build_action_nodes(Node *parent,
                                  std::unique_ptr<GameState> state)
    -> std::unique_ptr<Node> {
  std::unique_ptr<ActionNode> action_node =
      std::make_unique<ActionNode>(parent, state->current._id);

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
      const int amount{i == Action::ActionType::CALL ? state->get_call_amount()
                                                     : 0};
      Action action{.type = i, .amount = amount};
      buildAction(action_node, state, action, children, actions);
    } else {
      for (const auto &size : bet_sizes) {
        int bet_amount{static_cast<int>(size * state->pot)};
        bet_amount = bet_amount > state->current.stack ? state->current.stack
                                                       : bet_amount;
        // TODO: all in thresholdstack
        Action action{.type = i, .amount = bet_amount};
        build_action(action_node, state, action, children, actions);
      }
    }
  }

  const int curr_num_hands{state->current._id == 1 ? m_p1_num_hands
                                                   : m_p2_num_hands};
  action_node->init(children, actions, curr_num_hands);
}

auto GameTree::build_chance_nodes(Node *parent,
                                  std::unique_ptr<GameState> state)

    -> std::unique_ptr<Node> {
  assert(state->street == Street::FLOP ||
         state->street == Street::TURN && "build_chance_node error: ");
  using phevaluator::Card;
  using ChanceType = ChanceNode::ChanceType;
  ChanceNode::ChanceType type{state->street == Street::FLOP
                                  ? ChanceType::DEAL_TURN
                                  : ChanceType::DEAL_RIVER};
  ChanceNode chance_node{parent, type};

  for (int i{0}; i < 52; ++i) {
    Card card{i};

    // if card in board skip
    GameState nxt_state{*state};

    nxt_state.street =
        static_cast<Street>(static_cast<int>(nxt_state.street) + 1);

    std::unique_ptr<Node> action_node{
        build_action_nodes(&chance_node, nxt_state)};
    chance_node.add_child(std::move(action_node), card);
  }
}
auto GameTree::build_term_nodes(Node *parent, std::unique_ptr<GameState> state)
    -> std::unique_ptr<Node> {
  using TerminalType = TerminalNode::TerminalType;
  std::optional<TerminalNode> terminal_node;

  if (state->both_all_in() && state->street != Street::RIVER) {
    terminal_node.emplace(parent, TerminalType::ALLIN);
  } else if (state->is_uncontested()) {
    terminal_node.emplace(parent, TerminalType::UNCONTESTED);
  } else {
    terminal_node.emplace(parent, TerminalType::SHOWDOWN);
  }

  assert(terminal_node.has_value() &&
         "build_term_nodes uninitialized terminal_node");

  Node *node{parent};
  while (!dynamic_cast<ActionNode *>(node)) {
    node = node->get_parent();
  }

  assert(node && "terminal_node no viable action_node_parent");
  ActionNode *action_parent = dynamic_cast<ActionNode *>(node);

  terminal_node.value().set_last_to_act(action_parent->get_player());
  return terminal_node.value();
}
