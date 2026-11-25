// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace subathon::server::detail::v23_pragma {

// Private SQL for the executor's fenced connection and its spill witness.
inline constexpr char journal_mode[] = "PRAGMA main.journal_mode";
inline constexpr char journal_delete[] = "PRAGMA main.journal_mode=DELETE";
inline constexpr char journal_wal[] = "PRAGMA main.journal_mode=WAL";
inline constexpr char cache_size[] = "PRAGMA main.cache_size";
inline constexpr char cache_spill[] = "PRAGMA main.cache_spill";
inline constexpr char spill_off[] = "PRAGMA cache_spill=OFF";
inline constexpr char spill_on[] = "PRAGMA cache_spill=ON";
inline constexpr char temp_store[] = "PRAGMA temp_store";
inline constexpr char temp_memory[] = "PRAGMA temp_store=MEMORY";
inline constexpr char temp_default[] = "PRAGMA temp_store=0";
inline constexpr char temp_file[] = "PRAGMA temp_store=1";
inline constexpr char temp_schema_probe[] = "SELECT 1 FROM temp.sqlite_schema LIMIT 1";
inline constexpr char foreign_keys[] = "PRAGMA foreign_keys";
inline constexpr char foreign_keys_off[] = "PRAGMA foreign_keys=OFF";
inline constexpr char foreign_keys_on[] = "PRAGMA foreign_keys=ON";

} // namespace subathon::server::detail::v23_pragma
