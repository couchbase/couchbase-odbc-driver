#include "driver/utils/ieee_754_converter.h"

uint64_t ieee_double_to_ull(double d) {
    if (isnan(d)) {
        return 0x7ff8000000000000ULL;
    }

    if (std::numeric_limits<double>::has_infinity) {
        if (d == std::numeric_limits<double>().infinity()) {
            return 0x7ff0000000000000ULL;
        }

        if (d == -1 * std::numeric_limits<double>().infinity()) {
            return 0xfff0000000000000ULL;
        }
    }

    uint64_t bits;
    std::memcpy(&bits, &d, sizeof(d));
    return bits;
}

double ieee_ull_str_to_double(std::string ullStr) {
    uint64_t ull = stoull(ullStr, nullptr, 10);
    if (ull == 9221120237041090560ULL) {
        return std::numeric_limits<double>().quiet_NaN(); // or signaling NaN ?
    }
    if (ull == 9218868437227405312ULL) {
        return std::numeric_limits<double>().infinity();
    }
    if (ull == 18442240474082181120ULL) {
        return -1 * std::numeric_limits<double>().infinity();
    }

    double d;
    std::memcpy(&d, &ull, sizeof(ull));
    return d;
}