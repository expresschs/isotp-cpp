#include <cstdio>
#include <chrono>
#include "isotp/isotp_wrapper.h"
#include "isotp/isotp_device.h"
#include "devices/device_creator.h"

using namespace isotp;

// Dump the first few bytes of a payload as hex (truncated for readability).
static void dump_hex(const char *tag, const std::vector<unsigned char> &data, size_t max_bytes = 16)
{
    size_t n = data.size() < max_bytes ? data.size() : max_bytes;
    printf("%s (%zu bytes): ", tag, data.size());
    for (size_t i = 0; i < n; i++) {
        printf("%02X ", data[i]);
    }
    if (data.size() > max_bytes) {
        printf("...");
    }
    printf("\n");
}

// Build a deterministic byte pattern of the requested length.
static std::vector<unsigned char> pattern(size_t n, unsigned char seed = 0)
{
    std::vector<unsigned char> v(n);
    for (size_t i = 0; i < n; i++) {
        v[i] = (unsigned char)(seed + (i & 0xFF));
    }
    return v;
}

// Compare the received payload against the transmitted one, reporting the
// first mismatch for easy debugging.
static bool verify_payload(const std::vector<unsigned char> &tx,
                           const std::vector<unsigned char> &rx)
{
    if (tx.size() != rx.size()) {
        printf("    size mismatch: tx=%zu rx=%zu\n", tx.size(), rx.size());
        return false;
    }
    for (size_t i = 0; i < tx.size(); i++) {
        if (tx[i] != rx[i]) {
            printf("    data mismatch at byte %zu: tx=%02X rx=%02X\n", i, tx[i], rx[i]);
            return false;
        }
    }
    return true;
}

// Send a payload on tx_tp, receive it on rx_tp and verify it round-trips intact.
static bool round_trip(isotp15765 &tx_tp, isotp15765 &rx_tp,
                       const std::vector<unsigned char> &payload,
                       int send_timeout_ms, int recv_timeout_ms)
{
    isotp_message tx;
    tx.data = payload;

    int sret = tx_tp.send(tx, send_timeout_ms);
    if (sret != ISOTP_RET_OK) {
        printf("    send failed: %d\n", sret);
        return false;
    }

    isotp_message rx;
    int rret = rx_tp.receive(rx, recv_timeout_ms);
    if (rret != ISOTP_RET_OK) {
        printf("    receive failed: %d\n", rret);
        return false;
    }

    return verify_payload(payload, rx.data);
}

// Send an oversized payload and expect ISOTP_RET_OVERFLOW.
static bool expect_overflow(isotp15765 &tp, const std::vector<unsigned char> &payload)
{
    isotp_message tx;
    tx.data = payload;

    int ret = tp.send(tx, 1000);
    if (ret == ISOTP_RET_OVERFLOW) {
        printf("    overflow detected as expected (ret=%d)\n", ret);
        return true;
    }
    printf("    expected overflow but got ret=%d\n", ret);
    return false;
}

int main()
{
    // One USBCAN2 box, wired back-to-back:
    //   CAN1_H <-> CAN2_H, CAN1_L <-> CAN2_L, with 120 ohm termination on both ends.
    isotpDevice dev1(DEVICE_CAN_USBCAN2, "0:0", 500 * 1000);  // device 0, channel 0
    isotpDevice dev2(DEVICE_CAN_USBCAN2, "0:1", 500 * 1000);  // device 0, channel 1

    if (!dev1.isValid()) {
        printf("FAILED to open channel 0\n");
        return -1;
    }
    if (!dev2.isValid()) {
        printf("FAILED to open channel 1\n");
        return -1;
    }
    printf("== both channels opened ==\n");

    // tp1 transmits on channel 0, tp2 receives on channel 1.
    // ISO-TP addresses are symmetric: tp1 sends with its srcAddr, and tp2 matches
    // the frame against its own destAddr (and vice versa).
    isotp15765 tp1(&dev1, 0x18DA00FA, 0x18DAFA00);
    isotp15765 tp2(&dev2, 0x18DAFA00, 0x18DA00FA);

    // Expected outcome of a test case: either a successful round-trip or an overflow.
    enum class Expect { MATCH, TOO_BIG };

    struct TestCase {
        const char *name;
        isotp15765 *tx;
        isotp15765 *rx;
        std::vector<unsigned char> data;
        Expect expect;
    };

    // Test cases covering ISO-TP frame boundaries, flow control, reverse
    // direction and buffer overflow.
    TestCase cases[] = {
        {"single frame (1B)",        &tp1, &tp2, pattern(1, 0xAA),        Expect::MATCH},
        {"single frame (4B)",        &tp1, &tp2, {0xDE, 0xAD, 0xBE, 0xEF}, Expect::MATCH},
        {"single frame max (7B)",    &tp1, &tp2, pattern(7, 0x01),        Expect::MATCH},
        {"multi-frame min (8B)",     &tp1, &tp2, pattern(8, 0x10),        Expect::MATCH},
        {"multi-frame (9B)",         &tp1, &tp2, pattern(9, 0x20),        Expect::MATCH},
        {"multi-frame (63B)",        &tp1, &tp2, pattern(63, 0x30),       Expect::MATCH},
        {"multi-frame (64B)",        &tp1, &tp2, pattern(64, 0x40),       Expect::MATCH},
        {"multi-frame (200B)",       &tp1, &tp2, pattern(200, 0x50),      Expect::MATCH},
        {"multi-frame (1000B)",      &tp1, &tp2, pattern(1000, 0x60),     Expect::MATCH},
        {"protocol max (4095B)",     &tp1, &tp2, pattern(4095, 0x70),     Expect::MATCH},
        {"reverse direction (50B)",  &tp2, &tp1, pattern(50, 0x80),       Expect::MATCH},
        {"oversized (5000B)",        &tp1, &tp2, pattern(5000, 0x90),     Expect::TOO_BIG},
    };

    int passed = 0;
    int total = 0;

    for (auto &c : cases) {
        total++;
        printf("\n[%s]\n", c.name);
        dump_hex("  TX", c.data);

        auto t0 = std::chrono::steady_clock::now();
        bool ok = (c.expect == Expect::MATCH)
                      ? round_trip(*c.tx, *c.rx, c.data, 5000, 10000)
                      : expect_overflow(*c.tx, c.data);
        auto t1 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        printf("  %s (%lld ms)\n", ok ? "PASS" : "FAIL", (long long)ms);
        if (ok) passed++;
    }

    printf("\n== result: %d/%d passed ==\n", passed, total);
    return passed == total ? 0 : -1;
}
