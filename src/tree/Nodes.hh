#pragma once
#include "../game/Action.hh"
#include "../trainer/DCFR.hh"
#include <cassert>
#include <memory>
#include <vector>

class Node {
protected:
  Node *m_parent;

public:
  Node(Node *parent) : m_parent(parent) {}
  void set_parent(Node *parent) { m_parent = parent; }
  Node *get_parent() const { return m_parent; };
  virtual ~Node() = default;
};

// Action Node
class ActionNode : public Node {
  std::vector<Action> m_actions;
  std::vector<std::unique_ptr<Node>> m_children;
  int m_num_hands;
  int m_num_actions;
  int m_player;
  DCFR m_dcfr;

public:
  // check default initialization of std::vector
  ActionNode(Node *parent, const int player) : Node(parent), m_player(player) {}
  void init(std::vector<std::unique_ptr<Node>> &nodes,
            std::vector<Action> actions, const int num_hands) {
    m_num_hands = num_hands;

    for (const auto &i : nodes) {
      m_children.push_back(std::move(i));
    }
    for (const auto &i : actions) {
      m_actions.push_back(i);
    }

    m_num_actions = m_actions.size();
  }
  void set_trainer(const DCFR dcfr) { m_dcfr = dcfr; }
  auto get_trainer() const -> DCFR { return m_dcfr; }
  int get_player() const { return m_player; }

  auto get_child(const int index) const -> const Node * {
    assert(index >= 0 && index < m_children.size() &&
           "Node.hh attempting to access child out of range");
    return m_children[index].get();
  }
  auto get_action(const int index) const -> Action {
    assert(index >= 0 && index < m_actions.size() &&
           "Node.hh attempting to access action out of range");
    return m_actions[index];
  }
  auto get_average_strat() -> std::vector<double> {
    return m_dcfr.get_average_strat();
  }
  auto get_curent_strat() -> std::vector<double> {
    return m_dcfr.get_current_strat();
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

public:
  ChanceNode(Node *parent, const ChanceType type)
      : Node(parent), m_type(type), m_children(52) {}

  void add_child(std::unique_ptr<Node> node, const int card) {
    assert(card >= 0 && card < 52 && "ChanceNode: add_child card out of range");
    m_children[card] = std::move(node);
    ++m_child_count;
  }

  auto get_child(const int index) -> Node *const {
    return m_children[index].get();
  }
  auto get_child_count() -> int const { return m_child_count; }
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
  TerminalNode(Node *parent, TerminalType type)
      : Node(parent), m_type(type), m_pot(0) {}
  void set_last_to_act(int last_to_act) { m_last_to_act = last_to_act; }
};
