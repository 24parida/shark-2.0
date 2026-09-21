use postflop_solver::*;

fn main() {
    let path = std::env::args().nth(1).unwrap();
    for line in std::fs::read_to_string(path).unwrap().lines() {
        let fields: Vec<_> = line.split('|').collect();
        let board: Vec<_> = fields[3]
            .split_whitespace()
            .map(|c| card_from_str(c).unwrap())
            .collect();
        let cards = CardConfig {
            range: [fields[1].parse().unwrap(), fields[2].parse().unwrap()],
            flop: board[..3].try_into().unwrap(),
            turn: board.get(3).copied().unwrap_or(NOT_DEALT),
            river: board.get(4).copied().unwrap_or(NOT_DEALT),
        };
        let bets = BetSizeOptions::try_from(("100%", "")).unwrap();
        let config = TreeConfig {
            initial_state: match board.len() {
                3 => BoardState::Flop,
                4 => BoardState::Turn,
                _ => BoardState::River,
            },
            starting_pot: 100,
            effective_stack: 100,
            river_bet_sizes: [bets.clone(), bets],
            ..Default::default()
        };
        let tree = ActionTree::new(config).unwrap();
        let mut game = PostFlopGame::with_config(cards, tree).unwrap();
        game.allocate_memory(true);
        let iterations: u32 = fields[4].parse().unwrap();
        for i in 0..iterations {
            solve_step(&game, i);
        }
        let values = compute_mes_ev(&game);
        println!(
            "{},{:.9},{:.9},{:.9}",
            fields[0],
            values[0],
            values[1],
            (values[0] + values[1]) / 2.0
        );
    }
}
