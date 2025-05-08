#include "../include/json.hpp"
#include <fstream>
using json = nlohmann::json;

int main() {
  std::ofstream file{"json_test.json"};
  json j;
  json c1;
  json c2;

  j["type"] = "ActionNode";
  j["pot"] = 40;
  j["children"] = {{{"type", "ChanceNode"}, {"pot", 10}},
                   {{"type", "ActionNode"}, {"pot", 20}}};

  file << j.dump(4);
  return 0;
}
