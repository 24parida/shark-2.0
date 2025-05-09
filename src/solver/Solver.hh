#include "../hands/PreflopRangeManager.hh"
#include "../hands/RiverRangeManager.hh"

class ParallelDCFR {
  PreflopRangeManager m_prm;
  RiverRangeManager m_rrm;

  std::array<Card, 4> m_init_board;
  int m_init_pot;
  int m_in_position_player;

public:
  ParallelDCFR(const PreflopRangeManager prm,
               const std::array<Card, 4> init_board, const int init_pot,
               const int in_position_player)
      : m_prm(prm), m_init_board(init_board), m_init_pot(init_pot),
        m_in_position_player(in_position_player) {}
};

class CFRHeler {

public:
};
