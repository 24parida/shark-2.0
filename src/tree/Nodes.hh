#include "../trainer/DCFR.hh"
#include <memory>
#include <vector>

class Node {

  Node *m_parent;
  std::vector<Node> m_children;

public:
  Node() = default;
  Node(Node *parent) : m_parent(parent) {}
  void set_parent(Node *parent) { m_parent = parent; }
};

class ActionNode : public Node {
public:
private:
  DCFR m_dcfr;
  std::vector<int> m_actions;
  int m_num_hands;
  int m_num_actions;
  int m_player;
};
class ChanceNode : public Node {};
class TerminalNode : public Node {};
