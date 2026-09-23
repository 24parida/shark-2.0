import csv
import math
import sys


def read(path):
    with open(path, newline="") as source:
        rows = list(csv.reader(source))
    result = {}
    for row in rows:
        if len(row) != 4 or row[0] in result:
            raise ValueError(f"Invalid result row: {row}")
        values = tuple(map(float, row[1:]))
        if not all(map(math.isfinite, values)):
            raise ValueError(f"Nonfinite value: {row[0]}")
        result[row[0]] = values
    return result


shark = read(sys.argv[1])
reference = read(sys.argv[2])
if not shark or shark.keys() != reference.keys():
    raise ValueError("Comparison cases do not match")
for name, actual in shark.items():
    expected = reference[name]
    error = max(abs(actual[i] - expected[i]) for i in range(2))
    lower = max(-actual[1], -expected[1])
    upper = min(actual[0], expected[0])
    if error > 0.05 or lower > upper + 0.0001:
        raise ValueError(f"{name}: inconsistent best responses: {actual}, {expected}")
    if any(values[2] < -0.0001 or values[2] > 0.05 for values in (actual, expected)):
        raise ValueError(f"{name}: exploitability exceeds tolerance")
    print(f"PASS {name}: maximum BR difference {error:.6f} chips")
