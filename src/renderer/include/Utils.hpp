#pragma once

namespace utils
{
    // zaokrągla value do najbliższej wielokrotności alignment
    // value + alignment - 1 zaokrągla w górę
    // ~(alignment - 1) neguje bity alignment -1 przez co zerująy się bity mniejsze niż alignment -1
    // operacja & zostawia tylko bity większe lub równe alignment
    // np. value = 13 alignment = 8
    // 13 + 8 - 1 = 20 = 00010100
    // 8 - 1 = 7 = 00000111
    // ~7 = 11111000
    // 20 & 11111000 = 16 = 00010000
    // wynik to 16 czyli najbliższa wielokrotność 8 większa lub równa 13
    inline uint32_t aligned_size(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }
}