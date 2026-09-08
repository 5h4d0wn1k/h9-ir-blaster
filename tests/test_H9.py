import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "host"))
import h9_cli as m


class TestDecodeNec(unittest.TestCase):
    def test_valid_nec(self):
        # code = 0x00FF40BF -> little endian bytes BF 40 FF 00
        d = m.decode_nec(bytes.fromhex("BF 40 FF 00"))
        self.assertEqual(d["proto"], "NEC")
        self.assertEqual(d["addr"], 0x40)
        self.assertEqual(d["cmd"], 0xFF)
        self.assertTrue(d["checksum_ok"])

    def test_bad_checksum(self):
        d = m.decode_nec(bytes.fromhex("BF 40 00 00"))
        self.assertFalse(d["checksum_ok"])

    def test_short(self):
        self.assertIsNone(m.decode_nec(b"\x01\x02"))


if __name__ == "__main__":
    unittest.main()
