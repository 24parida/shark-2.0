#include "../trainer/DCFR.hh"
#include <memory>
#include <vector>

class Node {

public:
  Node(std::weak_ptr<Node> parent) : m_parent(parent) {}

private:
  std::weak_ptr<Node> m_parent;
  std::vector<Node> m_children;
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
