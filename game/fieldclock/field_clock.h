// The guest VBlank field word Tekken's linked libetc VSync returns.
#pragma once

#include <cstdint>

class Core;

namespace tekken3::field {

// Linked libetc VSync entry.
inline constexpr std::uint32_t kEntry = 0x800859A8u;

// Field count a negative VSync mode returns; FUN_80086358 zeroes it, FUN_800863B0 increments it.
inline constexpr std::uint32_t kCounter = 0x8009AC68u;

std::uint32_t readCounter(Core &core);

} // namespace tekken3::field
