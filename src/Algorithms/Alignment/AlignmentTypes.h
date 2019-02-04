#pragma once
#include "Algorithms/Datatypes/Qubits.h"
#include "Algorithms/Datatypes/Chrono.h"

namespace cqp {
    namespace align {

        /// Identifier type bins
        using BinID = uint_fast16_t;

        /// The number different values a qubit can hold
        constexpr uint_fast8_t maxChannels = static_cast<uint8_t>(BB84::_Last);

        using ChannelOffsets = std::array<PicoSecondOffset, maxChannels>;

    } // namespace align
} // namespace cqp
