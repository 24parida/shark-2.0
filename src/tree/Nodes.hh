#pragma once
#include "../game/Action.hh"
#include "../trainer/DCFR.hh"
#include <cassert>
#include <memory>
#include <vector>

class Node {
protected:
  Node *m_parent;
  std::vector<std::unique_ptr<Node>> m_children;

  Node(Node *parent) : m_parent(parent) {}
  void set_parent(Node *parent) { m_parent = parent; }
  virtual ~Node() = default;
};

class ActionNode : public Node {
  std::vector<Action> m_actions;
  int m_num_hands;
  int m_num_actions;
  int m_player;
  DCFR m_dcfr;

public:
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
  auto get_action(const int index) const -> Action {
    assert(index >= 0 && index < m_children.size() &&
           "Node.hh attempting to access child out of range");
    return m_actions[index];
  }
  auto get_average_strat() -> std::vector<double> {
    return m_dcfr.get_average_strat();
  }
  auto get_curent_strat() -> std::vector<double> {
    return m_dcfr.get_current_strat();
  }
};

class ChanceNode : public Node {};
class TerminalNode : public Node {};
