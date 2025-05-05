
#include "TreeBuilderSettings.hh"
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
};
