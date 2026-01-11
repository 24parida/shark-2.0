#pragma once
#include "../game/Action.hh"
#include "../trainer/DCFR.hh"
#include "../solver/Isomorphism.hh"
#include <cassert>
#include <memory>
#include <vector>

enum class NodeType { ACTION_NODE, CHANCE_NODE, TERMINAL_NODE };

class Node {
protected:
  Node const *m_parent;
  NodeType m_node_type;

public:
  Node(const Node *parent, const NodeType &node_type)
      : m_parent(parent), m_node_type(node_type) {}
  void set_parent(const Node *parent) { m_parent = parent; }
  const Node *get_parent() const { return m_parent; };
  auto get_node_type() const -> NodeType { return m_node_type; }
  virtual ~Node() = default;
};

// Action Node
class ActionNode : public Node {
  std::vector<Action> m_actions;
  std::vector<std::unique_ptr<Node>> m_children;
  int m_num_hands;
  int m_num_actions;
  int m_player;
  std::unique_ptr<DCFR> m_dcfr;

public:
  ActionNode(const Node *parent, const int player)
      : Node(parent, NodeType::ACTION_NODE), m_player(player) {}
  void init(const int num_hands) {
    m_num_hands = num_hands;
    m_num_actions = m_actions.size();
  }

  auto get_num_actions() const -> int { return m_num_actions; }
  auto get_children() -> std::vector<std::unique_ptr<Node>> & {
    return m_children;
  }
  auto get_actions() const -> const std::vector<Action> & { return m_actions; }
  auto get_num_hands() const -> int { return m_num_hands; }
  auto get_player() const -> int { return m_player; }

  void push_child(std::unique_ptr<Node> child) {
    m_children.push_back(std::move(child));
  }

  void push_action(const Action action) { m_actions.push_back(action); }

  void load_trainer(const ActionNode *node) {
    m_dcfr = std::make_unique<DCFR>(node);
  }
  auto get_trainer() -> DCFR * { return m_dcfr.get(); }
  auto get_trainer() const -> const DCFR * { return m_dcfr.get(); }

  auto get_child(const int index) const -> Node * {
    assert(index >= 0 && index < m_children.size() &&
           "Node.hh attempting to access child out of range");
    return m_children[static_cast<std::size_t>(index)].get();
  }
  auto get_action(const int index) const -> Action {
    assert(index >= 0 && index < m_actions.size() &&
           "Node.hh attempting to access action out of range");
    return m_actions[static_cast<std::size_t>(index)];
  }
  auto get_average_strat() const -> std::vector<float> {
    return m_dcfr->get_average_strat();
  }
  auto get_current_strat() const -> std::vector<float> {
    return m_dcfr->get_current_strat();
  }
};

// Chance Node
class ChanceNode : public Node {
public:
  enum ChanceType { DEAL_TURN, DEAL_RIVER };

private:
  std::vector<std::unique_ptr<Node>> m_children;
  int m_child_count;
  ChanceType m_type;
  IsomorphismData m_iso_data;

public:
  ChanceNode(const Node *parent, const ChanceType type)
      : Node(parent, NodeType::CHANCE_NODE), m_children(52), m_child_count(0),
        m_type(type) {}

  void add_child(std::unique_ptr<Node> node, const int card) {
    assert(card >= 0 && card < 52 && "ChanceNode: add_child card out of range");
    m_children[card] = std::move(node);
    ++m_child_count;
  }
  void set_isomorphism_data(IsomorphismData&& data) { m_iso_data = std::move(data); }
  auto get_isomorphism_data() const -> const IsomorphismData& { return m_iso_data; }
  auto get_num_children() const -> int { return m_child_count; }
  auto get_type() const -> ChanceType { return m_type; }
  auto get_child(const int index) const -> Node *const {
    return m_children[static_cast<std::size_t>(index)].get();
  }
  auto get_node_type() const -> NodeType { return m_node_type; }
};

// Terminal Node
class TerminalNode : public Node {
public:
  enum TerminalType { ALLIN, UNCONTESTED, SHOWDOWN };

private:
  TerminalType m_type;
  int m_last_to_act;
  int m_pot;

public:
  TerminalNode(const Node *parent, TerminalType type)
      : Node(parent, NodeType::TERMINAL_NODE), m_type(type), m_pot(0) {}
  void set_last_to_act(int last_to_act) { m_last_to_act = last_to_act; }
  void set_pot(const int pot) { m_pot = pot; }
  auto get_type() const -> TerminalType { return m_type; }
  auto get_node_type() const -> NodeType { return m_node_type; }
  auto get_pot() const -> int { return m_pot; }
  auto get_last_to_act() const -> int { return m_last_to_act; }
};
