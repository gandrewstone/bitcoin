#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import json
import os
import sys

"""
TODO: Add this to build system. For now, manually run to update:
`./data/generate_abla_test_vectors.py ./data/abla_test_vectors/config*.json > ./data/abla_test_vectors.json.h`
"""

def main():
    vector_files = sys.argv[1:]
    top_level_array = []
    for vec_file in vector_files:
        assert os.path.isfile(vec_file)
        with open(vec_file, "rt", encoding="utf8") as f:
            contents = f.read()
            json_cont = json.loads(contents)
            # Some basic sanity checks
            assert isinstance(json_cont, dict)
            d = json_cont["ABLAConfig"]
            assert isinstance(d, dict)
            d = json_cont["ABLAStateInitial"]
            assert isinstance(d, dict)
            a = json_cont["testVector"]
            assert isinstance(a, list)
            if "testName" not in json_cont:
                json_cont["testName"] = vec_file.rsplit(".", 1)[0].split("/")[-1]
            top_level_array.append(json_cont)
    json_bytes = json.dumps(top_level_array).encode(encoding="utf8")
    print("#include <cstdint>\n")
    print("namespace json_tests {")
    print("static const uint8_t abla_test_vectors[] = {")

    ctr = 0

    def formatter(bb: bytes) -> str:
        nonlocal ctr
        ret = f"0x{bb:02x}"
        # Allow for up to 20 items per line
        if ctr >= 20:
            ctr = 0
            ret = "\n" + ret
        ctr += 1
        return ret

    print(", ".join(map(formatter, json_bytes)))
    print("};")
    print("} // namespace json_tests")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: generate_abla_test_vectors.py [json_test_vector_file, ...]")
        sys.exit(1)

    main()
