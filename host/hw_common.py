"""Shared offline-analysis helpers for all H* host tools.

Authorized own-lab use only. See each repo README section "IMPORTANT".
"""
import os
import sys

DEMO_TAG = "[demo][ok] offline simulation only - no RF emitted"


def banner(project, kind):
    return "=" * 60


def require_lab(env_name, what, port=None):
    """Gate a live trigger behind an explicit LAB_* allowlist env var.

    Exits non-zero unless <env_name>=1 AND (an interactive yes is given if
    allow_confirm=True). Live triggers should additionally require an explicit
    --yes flag so a bare invocation is always safe by default.
    """
    if os.environ.get(env_name) != "1":
        sys.stderr.write(
            "\n[BLOCKED] '%s' is a live RF action and is blocked.\n"
            "This repo is for study and simulation only.\n"
            "To run a live bench test you must:\n"
            "  1. Be on an authorized own-lab bench (own devices only, no third parties in range).\n"
            "  2. Set %s=1 AND pass the explicit --yes confirmation flag.\n"
            "\nAborting.\n" % (what, env_name)
        )
        sys.exit(2)
    sys.stderr.write(
        "\n[WARN] %s=1 accepted. You confirmed own-network/own-devices only.\n"
        % env_name
    )


def read_target(path, default_text=""):
    f = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", path)
    if os.path.exists(f):
        with open(f, "r") as fh:
            return fh.read()
    return default_text


def hexdump(data, cols=16):
    out = []
    for i in range(0, len(data), cols):
        chunk = data[i:i + cols]
        out.append(
            "%04X  %s  %s"
            % (
                i,
                " ".join("%02X" % b for b in chunk),
                "".join(chr(b) if 32 <= b < 127 else "." for b in chunk),
            )
        )
    return "\n".join(out)