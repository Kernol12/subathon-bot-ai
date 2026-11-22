// SPDX-License-Identifier: GPL-3.0-or-later
#include <subathon/server/rule_simulator.hpp>

namespace subathon::server::rule_simulator {
SimulationResult inspect_simulation(const Scenario &, const Limits &) {
    throw RuleSimulatorNotImplemented{};
}
} // namespace subathon::server::rule_simulator
