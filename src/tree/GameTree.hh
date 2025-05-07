#include "../game/State.hh"
#include "Nodes.hh"
#include "TreeBuilderSettings.hh"
#include <memory>

class GameTree {
  TreeBuilderSettings m_settings;
  int m_p1_num_hands;
  int m_p2_num_hands;

  int m_flop_action_node_count;
  int m_turn_action_node_count;
  int m_river_action_node_count;

  int m_chance_node_count;
  int m_terminal_node_count;

public:
  GameTree(const TreeBuilderSettings settings)
      : m_settings(settings), m_p1_num_hands(settings.range1.num_hands),
        m_p2_num_hands(settings.range2.num_hands) {}

  auto get_init_state() const -> GameState;
  auto build() -> std::unique_ptr<Node>;
  auto build_action(std::unique_ptr<ActionNode> node, const GameState &state,
                    const Action &action) -> std::unique_ptr<ActionNode>;

  auto build_action_nodes(const Node *parent, const GameState &state)
      -> std::unique_ptr<Node>;
  auto build_chance_nodes(const Node *parent, const GameState &state)
      -> std::unique_ptr<Node>;
  auto build_term_nodes(const Node *, const GameState &state)
      -> std::unique_ptr<Node>;
};
