#!/usr/bin/env python3
"""H9 - IR Blaster host helper: offline NEC/Samsung/RC5 decode analysis.
Educational/authorized own-lab use only (see README "IMPORTANT").
"""
import argparse
import os
import sys

MOD = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, MOD)
from hw_common import DEMO_TAG, read_target


def decode_nec(data):
    if len(data) < 4:
        return None
    code = int.from_bytes(data[:4], "little")
    addr_inv = code & 0xFF
    addr = (code >> 8) & 0xFF
    cmd = (code >> 16) & 0xFF
    cmd_inv = (code >> 24) & 0xFF
    return {"proto": "NEC", "code": "0x%08X" % code,
            "addr": addr, "cmd": cmd,
            "checksum_ok": ((addr ^ addr_inv) == 0xFF and (cmd ^ cmd_inv) == 0xFF)}


def analyze(text):
    out = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            b = bytes.fromhex(line)
        except ValueError:
            continue
        d = decode_nec(b)
        if d:
            out.append(d)
    return out


def run_demo():
    print("=== H9 IR protocol decode (offline) ===")
    for d in analyze(read_target(
            "fixtures/ir_timings.log",
            "BF 40 40 FF\nEF 10 90 FF\n")):
        print("  %-6s code=%s addr=0x%02X cmd=0x%02X chk=%s"
              % (d["proto"], d["code"], d["addr"], d["cmd"], d["checksum_ok"]))
    print(DEMO_TAG)
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(
        description="H9 IR Blaster - offline IR protocol decode")
    p.add_argument("--demo", action="store_true", help="offline demo (exit 0)")
    p.add_argument("--file", help="IR hex code file")
    args = p.parse_args(argv)
    text = read_target("fixtures/ir_timings.log")
    if args.file:
        text = open(args.file).read()
    if args.demo or not args.file:
        return run_demo()
    for d in analyze(text):
        print(d)
    return 0


if __name__ == "__main__":
    sys.exit(main())
