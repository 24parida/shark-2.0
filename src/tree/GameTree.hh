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

  GameTree(const TreeBuilderSettings settings)
      : m_settings(settings), m_p1_num_hands(settings.range1.num_hands),
        m_p2_num_hands(settings.range2.num_hands) {}
  auto get_init_state() -> std::unique_ptr<GameState>;
  auto build() -> std::unique_ptr<Node>;
  void build_action(const ActionNode node, const GameState state,
                    const Action action, const std::vector<Node> children,
                    const std::vector<Action> actions);

  auto build_action_nodes(Node parent, std::unique_ptr<GameState> state)
      -> std::unique_ptr<Node>;
  auto build_chance_nodes(Node parent, std::unique_ptr<GameState> state)
      -> std::unique_ptr<Node>;
  auto build_terminal_nodes(Node parent, std::unique_ptr<GameState> state)
      -> std::unique_ptr<Node>;
};
