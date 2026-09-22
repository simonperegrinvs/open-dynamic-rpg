#include "odr/session.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iterator>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace {

struct GameError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

using Hex = std::pair<int, int>;

Hex hex(const json& value) {
    if (!value.is_array() || value.size() != 2 || !value[0].is_number_integer() ||
        !value[1].is_number_integer()) {
        throw GameError("expected [q,r] hex coordinates");
    }
    return {value[0].get<int>(), value[1].get<int>()};
}

json tile(Hex value) {
    return json::array({value.first, value.second});
}

int distance(Hex a, Hex b) {
    const int dq = a.first - b.first;
    const int dr = a.second - b.second;
    return (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
}

std::vector<Hex> neighbors(Hex h) {
    return {{h.first + 1, h.second}, {h.first + 1, h.second - 1}, {h.first, h.second - 1},
            {h.first - 1, h.second}, {h.first - 1, h.second + 1}, {h.first, h.second + 1}};
}

std::string required_string(const json& value, const char* key) {
    if (!value.contains(key) || !value[key].is_string()) {
        throw GameError(std::string("missing string: ") + key);
    }
    return value[key].get<std::string>();
}

int required_int(const json& value, const char* key) {
    if (!value.contains(key) || !value[key].is_number_integer()) {
        throw GameError(std::string("missing integer: ") + key);
    }
    return value[key].get<int>();
}

json parse_object(const char* text) {
    if (text == nullptr) {
        throw GameError("missing JSON input");
    }
    json value = json::parse(text, nullptr, false);
    if (value.is_discarded() || !value.is_object()) {
        throw GameError("invalid JSON object");
    }
    return value;
}

bool member_of(const std::string& value, const std::vector<std::string>& choices) {
    return std::find(choices.begin(), choices.end(), value) != choices.end();
}

std::uint64_t next_random(json& state) {
    auto value = state["rng"].get<std::uint64_t>();
    value ^= value << 13;
    value ^= value >> 7;
    value ^= value << 17;
    state["rng"] = value;
    return value;
}

std::string next_id(json& state, const std::string& prefix) {
    const auto number = state["next_id"].get<std::uint64_t>();
    state["next_id"] = number + 1;
    return prefix + "-" + std::to_string(number);
}

json character(const std::string& id, const std::string& name, const std::string& ancestry,
               const std::string& background, const std::string& role) {
    const bool caster = role == "Mage" || role == "Cleric";
    const int max_hp = role == "Barbarian" ? 19 : role == "Warrior" ? 17 : 13;
    return {{"id", id},
            {"name", name},
            {"ancestry", ancestry},
            {"background", background},
            {"class", role},
            {"level", 1},
            {"xp", 0},
            {"hp", max_hp},
            {"max_hp", max_hp},
            {"wounds", 0},
            {"spell_uses", caster ? 2 : 0},
            {"max_spell_uses", caster ? 2 : 0},
            {"visual_id", "placeholder." + role}};
}

json combatant(const json& source, Hex position) {
    const std::string role = source["class"].get<std::string>();
    const int level = source["level"].get<int>();
    const bool ranged = role == "Ranger" || role == "Mage";
    return {{"id", source["id"]},
            {"name", source["name"]},
            {"team", "party"},
            {"class", role},
            {"pos", tile(position)},
            {"hp", source["hp"]},
            {"max_hp", source["max_hp"]},
            {"armor", role == "Warrior"     ? 3
                      : role == "Barbarian" ? 2
                                            : 1},
            {"evasion", role == "Rogue" ? 14 : 8},
            {"accuracy", 5 + level},
            {"damage", (role == "Barbarian" ? 7 : 5) + level},
            {"range", ranged ? 4 : 1},
            {"speed", role == "Rogue"    ? 11
                      : role == "Ranger" ? 10
                                         : 7},
            {"move_left", 4},
            {"acted", false},
            {"poison", 0},
            {"defending", false},
            {"spell_uses", source["spell_uses"]},
            {"visual_id", source["visual_id"]}};
}

json enemy(const std::string& id, const json& source) {
    return {{"id", id},
            {"name", required_string(source, "name")},
            {"team", "enemy"},
            {"class", required_string(source, "class")},
            {"pos", source.at("pos")},
            {"hp", required_int(source, "hp")},
            {"max_hp", required_int(source, "max_hp")},
            {"armor", required_int(source, "armor")},
            {"evasion", required_int(source, "evasion")},
            {"accuracy", required_int(source, "accuracy")},
            {"damage", required_int(source, "damage")},
            {"range", required_int(source, "range")},
            {"speed", required_int(source, "speed")},
            {"move_left", required_int(source, "move_left")},
            {"acted", false},
            {"poison", 0},
            {"defending", false},
            {"visual_id", required_string(source, "visual_id")}};
}

std::set<Hex> hex_set(const json& array) {
    std::set<Hex> result;
    for (const auto& item : array) {
        result.insert(hex(item));
    }
    return result;
}

bool walkable(const json& layout, Hex point) {
    const auto allowed = hex_set(layout["tiles"]);
    const auto walls = hex_set(layout["walls"]);
    return allowed.contains(point) && !walls.contains(point);
}

std::vector<Hex> deployment_hexes(const json& layout, Hex trigger, Hex entry,
                                  const std::set<Hex>& occupied = {}) {
    const auto allowed = hex_set(layout["tiles"]);
    const auto walls = hex_set(layout["walls"]);
    const int direction_q = entry.first - trigger.first;
    const int direction_r = entry.second - trigger.second;
    const int direction_s = -direction_q - direction_r;
    std::vector<Hex> result;
    for (int q = trigger.first - 4; q <= trigger.first + 4; ++q) {
        for (int r = trigger.second - 4; r <= trigger.second + 4; ++r) {
            const Hex point{q, r};
            const int offset_q = q - trigger.first;
            const int offset_r = r - trigger.second;
            const int offset_s = -offset_q - offset_r;
            const int alignment =
                offset_q * direction_q + offset_r * direction_r + offset_s * direction_s;
            if (distance(point, trigger) <= 4 && alignment >= 0 && allowed.contains(point) &&
                !walls.contains(point) && !occupied.contains(point)) {
                result.push_back(point);
            }
        }
    }
    std::sort(result.begin(), result.end(), [entry, trigger](Hex left, Hex right) {
        return std::tuple{distance(left, entry), distance(left, trigger), left.first, left.second} <
               std::tuple{distance(right, entry), distance(right, trigger), right.first,
                          right.second};
    });
    return result;
}

void validate_enemy_spawns(const json& layout) {
    if (!layout.contains("enemy_positions") || !layout["enemy_positions"].is_object()) {
        throw GameError("layout is missing enemy spawn bindings");
    }
    const auto& objects = layout["objects"];
    for (const char* encounter : {"main", "secret"}) {
        if (!layout["enemy_positions"].contains(encounter) ||
            !layout["enemy_positions"][encounter].is_array()) {
            throw GameError(std::string("layout is missing ") + encounter + " spawn bindings");
        }
        std::set<Hex> occupied;
        for (const auto& spawn : layout["enemy_positions"][encounter]) {
            if (!spawn.is_object() || !spawn.contains("name") || !spawn["name"].is_string() ||
                !spawn.contains("pos")) {
                throw GameError("enemy spawn binding is malformed");
            }
            const std::string name = spawn["name"].get<std::string>();
            const Hex position = hex(spawn["pos"]);
            if (!walkable(layout, position)) {
                throw GameError(name + " spawn is not walkable");
            }
            if (!occupied.insert(position).second) {
                throw GameError(name + " spawn overlaps another enemy");
            }
            for (const auto& [object_name, object] : objects.items()) {
                if (object.is_array() && object.size() == 2 && object[0].is_number_integer() &&
                    object[1].is_number_integer() && hex(object) == position) {
                    throw GameError(name +
                                    " spawn overlaps layout object: " + std::string(object_name));
                }
            }
        }
    }
}

std::map<Hex, int> distances(const json& layout, Hex start, const std::set<Hex>& occupied = {}) {
    const auto allowed = hex_set(layout["tiles"]);
    const auto walls = hex_set(layout["walls"]);
    const auto rough = hex_set(layout["rough"]);
    std::priority_queue<std::pair<int, Hex>, std::vector<std::pair<int, Hex>>,
                        std::greater<std::pair<int, Hex>>>
        pending;
    std::map<Hex, int> costs{{start, 0}};
    pending.push({0, start});
    while (!pending.empty()) {
        const auto [cost, point] = pending.top();
        pending.pop();
        if (cost != costs[point]) {
            continue;
        }
        for (Hex neighbor : neighbors(point)) {
            if (!allowed.contains(neighbor) || walls.contains(neighbor) ||
                occupied.contains(neighbor)) {
                continue;
            }
            const int next = cost + (rough.contains(neighbor) ? 2 : 1);
            if (!costs.contains(neighbor) || next < costs[neighbor]) {
                costs[neighbor] = next;
                pending.push({next, neighbor});
            }
        }
    }
    return costs;
}

bool line_of_sight(const json& layout, Hex start, Hex end) {
    const int steps = distance(start, end);
    if (steps == 0) {
        return true;
    }
    for (int step = 1; step < steps; ++step) {
        const double fraction = static_cast<double>(step) / static_cast<double>(steps);
        const double q = start.first + (end.first - start.first) * fraction;
        const double r = start.second + (end.second - start.second) * fraction;
        const double s = -q - r;
        int rq = static_cast<int>(std::round(q));
        int rr = static_cast<int>(std::round(r));
        const int rs = static_cast<int>(std::round(s));
        const double dq = std::abs(rq - q);
        const double dr = std::abs(rr - r);
        const double ds = std::abs(rs - s);
        if (dq > dr && dq > ds) {
            rq = -rr - rs;
        } else if (dr > ds) {
            rr = -rq - rs;
        }
        if (!walkable(layout, {rq, rr})) {
            return false;
        }
    }
    return true;
}

json make_layout(const json& definition, std::uint64_t seed, bool generated, bool boss) {
    const json& terrain = definition["mine"]["terrain"];
    json objects = definition["mine"];
    objects.erase("terrain");
    json layout = {{"schema_version", 1},
                   {"seed", seed},
                   {"provider_version", "hex-rooms-v1"},
                   {"tiles", json::array()},
                   {"walls", json::array()},
                   {"rough", json::array()},
                   {"rooms", json::array({
                                 {{"id", "entry"}, {"role", "entry"}},
                                 {{"id", "gallery"}, {"role", "exploration"}},
                                 {{"id", "main"}, {"role", boss ? "boss" : "main"}},
                                 {{"id", "secret"}, {"role", "optional_secret"}},
                             })},
                   {"connections", json::array({json::array({"entry", "gallery"}),
                                                json::array({"gallery", "main"}),
                                                json::array({"gallery", "secret"})})},
                   {"objects", std::move(objects)},
                   {"enemy_positions", {{"main", json::array()}, {"secret", json::array()}}}};
    for (const auto& spec : definition["encounters"]["main"]) {
        if (!spec.value("boss", false) || boss) {
            layout["enemy_positions"]["main"].push_back(
                {{"name", required_string(spec, "suffix")}, {"pos", spec.at("pos")}});
        }
    }
    for (const auto& spec : definition["encounters"]["secret"]) {
        layout["enemy_positions"]["secret"].push_back(
            {{"name", required_string(spec, "suffix")}, {"pos", spec.at("pos")}});
    }
    layout["floors"] =
        boss ? json::array({{{"id", "upper"}, {"depth", 1}}, {{"id", "lower"}, {"depth", 2}}})
             : json::array({{{"id", "mine"}, {"depth", 1}}});
    if (boss) {
        layout["objects"]["stairs_down"] = tile({6, 0});
        layout["objects"]["stairs_up"] = tile({6, 0});
        layout["connections"] =
            json::array({json::array({"entry", "gallery"}), json::array({"gallery", "stairs"}),
                         json::array({"stairs", "main"}), json::array({"gallery", "secret"})});
        layout["rooms"].push_back({{"id", "stairs"}, {"role", "floor_transition"}});
    }
    for (int q = required_int(terrain, "q_min"); q <= required_int(terrain, "q_max"); ++q) {
        for (int r = required_int(terrain, "r_min"); r <= required_int(terrain, "r_max"); ++r) {
            layout["tiles"].push_back(tile({q, r}));
        }
    }
    layout["walls"] = terrain.at("walls");
    layout["rough"] = terrain.at("rough");
    if (generated) {
        const auto& walls = terrain.at("generated_wall_options");
        const auto& rough = terrain.at("generated_rough_options");
        layout["walls"].push_back(walls[seed % walls.size()]);
        layout["rough"].push_back(rough[(seed / walls.size()) % rough.size()]);
    }
    return layout;
}

void validate_encounter_bindings(const json& layout, const json& definition, bool boss) {
    if (!layout.contains("enemy_positions") || !layout["enemy_positions"].is_object()) {
        throw GameError("layout is missing enemy spawn bindings");
    }
    for (const char* encounter : {"main", "secret"}) {
        if (!layout["enemy_positions"].contains(encounter) ||
            !layout["enemy_positions"][encounter].is_array()) {
            throw GameError(std::string("layout is missing ") + encounter + " spawn bindings");
        }
        std::set<std::string> expected;
        for (const auto& spec : definition["encounters"][encounter]) {
            if (std::string(encounter) == "main" && spec.value("boss", false) && !boss) {
                continue;
            }
            expected.insert(required_string(spec, "suffix"));
        }
        std::set<std::string> actual;
        for (const auto& spawn : layout["enemy_positions"][encounter]) {
            if (!spawn.is_object() || !spawn.contains("name") || !spawn["name"].is_string() ||
                !spawn.contains("pos")) {
                throw GameError(std::string(encounter) + " enemy spawn binding is malformed");
            }
            hex(spawn["pos"]);
            if (!actual.insert(spawn["name"].get<std::string>()).second ||
                !expected.contains(spawn["name"].get<std::string>())) {
                throw GameError(std::string(encounter) + " enemy spawn identity is invalid");
            }
        }
        if (actual != expected) {
            throw GameError(std::string(encounter) + " enemy spawn set is incomplete");
        }
    }
}

void validate_layout(const json& layout, int party_size) {
    if (!layout.is_object() || !layout.contains("tiles") || !layout.contains("walls") ||
        !layout.contains("rough") || !layout.contains("objects") || !layout.contains("rooms") ||
        !layout.contains("connections") || !layout.contains("floors") ||
        !layout.contains("enemy_positions")) {
        throw GameError("layout is missing required geometry or bindings");
    }
    if (party_size < 1 || party_size > 12) {
        throw GameError("deployment party size must be between one and twelve");
    }
    if (!layout["floors"].is_array() || layout["floors"].empty() || layout["floors"].size() > 2) {
        throw GameError("layout must contain one or two ordered floors");
    }
    for (std::size_t index = 0; index < layout["floors"].size(); ++index) {
        const json& floor = layout["floors"][index];
        if (!floor.is_object() || required_int(floor, "depth") != static_cast<int>(index) + 1 ||
            required_string(floor, "id").empty()) {
            throw GameError("floor depth or identity is inconsistent");
        }
    }
    const json& objects = layout["objects"];
    const Hex entry = hex(objects.at("entry"));
    if (!walkable(layout, entry)) {
        throw GameError("mine entry is blocked");
    }
    validate_enemy_spawns(layout);
    const auto reachable = distances(layout, entry);
    for (const char* required :
         {"clue", "hidden_loot", "main_trigger", "ore", "secret_trigger", "exit"}) {
        if (!reachable.contains(hex(objects.at(required)))) {
            throw GameError(std::string("unreachable objective: ") + required);
        }
    }
    for (const char* name : {"main_trigger", "secret_trigger"}) {
        const Hex trigger = hex(objects.at(name));
        bool legal_approach = false;
        std::set<Hex> enemy_positions;
        const char* encounter = std::string(name) == "main_trigger" ? "main" : "secret";
        for (const auto& spawn : layout["enemy_positions"][encounter]) {
            enemy_positions.insert(hex(spawn["pos"]));
        }
        for (const Hex& entry_hex : neighbors(trigger)) {
            if (!reachable.contains(entry_hex)) {
                continue;
            }
            legal_approach = true;
            const auto deployment = deployment_hexes(layout, trigger, entry_hex, enemy_positions);
            const auto connected = distances(layout, entry_hex, enemy_positions);
            const auto connected_deployment =
                std::count_if(deployment.begin(), deployment.end(),
                              [&connected](Hex point) { return connected.contains(point); });
            if (static_cast<std::size_t>(connected_deployment) <
                static_cast<std::size_t>(party_size)) {
                throw GameError(std::string(name) +
                                " approach lacks connected party deployment capacity");
            }
            if (deployment.size() < static_cast<std::size_t>(party_size)) {
                throw GameError(std::string(name) + " approach lacks party deployment capacity");
            }
        }
        if (!legal_approach) {
            throw GameError(std::string(name) + " has no reachable battle approach");
        }
    }
    const bool boss = layout["floors"].size() == 2;
    std::set<std::string> room_ids;
    for (const auto& room : layout["rooms"]) {
        if (!room.is_object() || !room_ids.insert(required_string(room, "id")).second) {
            throw GameError("room identities must be unique");
        }
    }
    for (const auto& link : layout["connections"]) {
        if (!link.is_array() || link.size() != 2 || !link[0].is_string() || !link[1].is_string() ||
            !room_ids.contains(link[0].get<std::string>()) ||
            !room_ids.contains(link[1].get<std::string>())) {
            throw GameError("room connection references an absent room");
        }
    }
    const std::vector<json> required_links =
        boss
            ? std::vector<json>{json::array({"entry", "gallery"}),
                                json::array({"gallery", "stairs"}), json::array({"stairs", "main"}),
                                json::array({"gallery", "secret"})}
            : std::vector<json>{json::array({"entry", "gallery"}), json::array({"gallery", "main"}),
                                json::array({"gallery", "secret"})};
    for (const auto& link : required_links) {
        if (std::find(layout["connections"].begin(), layout["connections"].end(), link) ==
            layout["connections"].end()) {
            throw GameError("required room connection is missing: " + link.dump());
        }
    }
    if (boss && (!reachable.contains(hex(objects.at("stairs_down"))) ||
                 !reachable.contains(hex(objects.at("stairs_up"))))) {
        throw GameError("boss floor stairs are unreachable");
    }
}

void validate_content_definition(const json& definition) {
    if (!definition.contains("mine") || !definition["mine"].is_object()) {
        throw GameError("content is missing mine bindings");
    }
    for (const char* key :
         {"entry", "clue", "hidden_loot", "main_trigger", "ore", "secret_trigger", "exit"}) {
        if (!definition["mine"].contains(key)) {
            throw GameError(std::string("content is missing objective binding: ") + key);
        }
        hex(definition["mine"][key]);
    }
    if (!definition.contains("recruits") || !definition["recruits"].is_array()) {
        throw GameError("content recruits must be an array");
    }
    std::set<std::string> recruit_ids;
    for (const auto& recruit : definition["recruits"]) {
        for (const char* key : {"id", "name", "class"}) {
            required_string(recruit, key);
        }
        if (!recruit_ids.insert(recruit["id"].get<std::string>()).second ||
            !member_of(recruit["class"].get<std::string>(),
                       {"Warrior", "Rogue", "Ranger", "Mage", "Cleric", "Barbarian"})) {
            throw GameError("content recruit identity or class is invalid");
        }
    }
    const auto& terrain = definition.at("mine").at("terrain");
    for (const char* key : {"q_min", "q_max", "r_min", "r_max"}) {
        (void)required_int(terrain, key);
    }
    if (terrain["q_min"].get<int>() > terrain["q_max"].get<int>() ||
        terrain["r_min"].get<int>() > terrain["r_max"].get<int>()) {
        throw GameError("content terrain bounds are invalid");
    }
    for (const char* key :
         {"walls", "rough", "generated_wall_options", "generated_rough_options"}) {
        if (!terrain.contains(key) || !terrain[key].is_array() ||
            (std::string(key).starts_with("generated_") && terrain[key].empty())) {
            throw GameError(std::string("content terrain binding is invalid: ") + key);
        }
        for (const auto& point : terrain[key]) {
            hex(point);
        }
    }
    for (const char* key : {"participant", "location", "reward"}) {
        required_string(definition.at("defaults"), key);
    }
    for (const char* key : {"cache_gold", "ore_quantity", "main_xp", "secret_xp", "secret_gold"}) {
        if (required_int(definition.at("defaults"), key) <= 0) {
            throw GameError(std::string("content default quantity is invalid: ") + key);
        }
    }
    std::set<std::string> suffixes;
    for (const char* encounter : {"main", "secret"}) {
        const auto& records = definition.at("encounters").at(encounter);
        if (!records.is_array() || records.empty()) {
            throw GameError(std::string("content encounter list is empty: ") + encounter);
        }
        int bosses = 0;
        for (const auto& record : records) {
            for (const char* key : {"suffix", "name", "class", "visual_id"}) {
                required_string(record, key);
            }
            const std::string suffix = record["suffix"].get<std::string>();
            if (!suffixes.insert(suffix).second || suffix.empty()) {
                throw GameError("content encounter IDs are not unique");
            }
            hex(record.at("pos"));
            for (const char* key : {"hp", "max_hp", "armor", "evasion", "accuracy", "damage",
                                    "range", "speed", "move_left"}) {
                if (required_int(record, key) < 0) {
                    throw GameError(std::string("content enemy stat is invalid: ") + key);
                }
            }
            if (record["hp"].get<int>() <= 0 ||
                record["max_hp"].get<int>() < record["hp"].get<int>() ||
                record["range"].get<int>() <= 0 || record["speed"].get<int>() <= 0) {
                throw GameError("content enemy combat stats are invalid");
            }
            if (record.value("boss", false)) {
                if (std::string(encounter) != "main" || ++bosses > 1) {
                    throw GameError("content boss encounter binding is invalid");
                }
            }
        }
    }
}

void require_save_object(const json& value, const char* key, const char* context) {
    if (!value.contains(key) || !value[key].is_object()) {
        throw GameError(std::string("save is missing object: ") + context + "." + key);
    }
}

void require_save_integer(const json& value, const char* key, const char* context) {
    if (!value.contains(key) || !value[key].is_number_integer()) {
        throw GameError(std::string("save is missing integer: ") + context + "." + key);
    }
}

void require_save_string(const json& value, const char* key, const char* context) {
    if (!value.contains(key) || !value[key].is_string()) {
        throw GameError(std::string("save is missing string: ") + context + "." + key);
    }
}

void require_save_bool(const json& value, const char* key, const char* context) {
    if (!value.contains(key) || !value[key].is_boolean()) {
        throw GameError(std::string("save is missing boolean: ") + context + "." + key);
    }
}

void validate_enemy_record(const json& enemy_record, const std::string& run_id,
                           const char* context) {
    if (!enemy_record.is_object()) {
        throw GameError(std::string("save ") + context + " is not an object");
    }
    for (const char* key : {"id", "name", "team", "class", "visual_id"}) {
        require_save_string(enemy_record, key, context);
    }
    if (enemy_record["team"] != "enemy") {
        throw GameError(std::string("save ") + context + " team is invalid");
    }
    const std::string id = enemy_record["id"].get<std::string>();
    if (id.rfind(run_id + "-", 0) != 0) {
        throw GameError(std::string("save ") + context + " identity is not bound to its run");
    }
    if (!enemy_record.contains("pos") || !enemy_record["pos"].is_array()) {
        throw GameError(std::string("save ") + context + " is missing position");
    }
    hex(enemy_record["pos"]);
    for (const char* key : {"hp", "max_hp", "armor", "evasion", "accuracy", "damage", "range",
                            "speed", "move_left", "poison"}) {
        require_save_integer(enemy_record, key, context);
    }
    for (const char* key : {"acted", "defending"}) {
        require_save_bool(enemy_record, key, context);
    }
    const int hp = enemy_record["hp"].get<int>();
    const int max_hp = enemy_record["max_hp"].get<int>();
    if (max_hp <= 0 || hp < 0 || hp > max_hp || enemy_record["armor"].get<int>() < 0 ||
        enemy_record["evasion"].get<int>() < 0 || enemy_record["accuracy"].get<int>() < 0 ||
        enemy_record["damage"].get<int>() < 0 || enemy_record["range"].get<int>() <= 0 ||
        enemy_record["speed"].get<int>() <= 0 || enemy_record["move_left"].get<int>() < 0 ||
        enemy_record["poison"].get<int>() < 0) {
        throw GameError(std::string("save ") + context + " has an invalid combat range");
    }
}

void validate_saved_battle(const json& battle, const json& run,
                           const std::set<std::string>& active_party,
                           const std::map<std::string, json>& party_by_id, int mine_floor) {
    if (!battle.is_object()) {
        throw GameError("save battle state must be an object");
    }
    require_save_string(battle, "kind", "battle");
    if (!member_of(battle["kind"].get<std::string>(), {"main", "secret"})) {
        throw GameError("save battle kind is invalid");
    }
    if (!battle.contains("approach") || !battle.contains("trigger") ||
        !battle["approach"].is_array() || !battle["trigger"].is_array()) {
        throw GameError("save battle is missing approach coordinates");
    }
    hex(battle["approach"]);
    hex(battle["trigger"]);
    require_save_integer(battle, "round", "battle");
    require_save_integer(battle, "turn_index", "battle");
    if (battle["round"].get<int>() < 1) {
        throw GameError("save battle round is invalid");
    }
    if (!battle.contains("actors") || !battle["actors"].is_array() || !battle.contains("order") ||
        !battle["order"].is_array() || battle["order"].empty()) {
        throw GameError("save battle is missing actors or order");
    }
    const bool secret = battle["kind"] == "secret";
    const char* trigger_name = secret ? "secret_trigger" : "main_trigger";
    require_save_string(run, "id", "run");
    require_save_string(run, "template_id", "run");
    require_save_object(run, "layout", "run");
    require_save_object(run["layout"], "objects", "run layout");
    for (const char* key : {"tiles", "walls"}) {
        if (!run["layout"].contains(key) || !run["layout"][key].is_array()) {
            throw GameError(std::string("save run layout is missing array: ") + key);
        }
    }
    require_save_object(run, "changes", "run");
    for (const char* key : {"secret_found", "secret_won", "main_won"}) {
        require_save_bool(run["changes"], key, "run changes");
    }
    if (!run["layout"]["objects"].contains(trigger_name) ||
        battle["trigger"] != run["layout"]["objects"][trigger_name]) {
        throw GameError("save battle trigger does not match its encounter");
    }
    if (distance(hex(battle["approach"]), hex(battle["trigger"])) != 1 ||
        !walkable(run["layout"], hex(battle["approach"]))) {
        throw GameError("save battle approach is not a walkable adjacent entry");
    }
    const auto& changes = run["changes"];
    if (secret ? (!changes["secret_found"].get<bool>() || changes["secret_won"].get<bool>())
               : changes["main_won"].get<bool>()) {
        throw GameError("save battle is inconsistent with completed run changes");
    }
    require_save_bool(run, "boss", "run");
    const int expected_floor = secret ? 1 : (run["boss"].get<bool>() ? 2 : 1);
    if (mine_floor != expected_floor) {
        throw GameError("save battle floor does not match its encounter");
    }
    const char* enemy_field = secret ? "secret_enemies" : "main_enemies";
    if (!run.contains(enemy_field) || !run[enemy_field].is_array()) {
        throw GameError(std::string("save run is missing enemy array: ") + enemy_field);
    }
    std::map<std::string, json> enemy_by_id;
    for (const auto& enemy_record : run[enemy_field]) {
        validate_enemy_record(enemy_record, run["id"].get<std::string>(), enemy_field);
        enemy_by_id.emplace(enemy_record["id"].get<std::string>(), enemy_record);
    }
    std::set<std::string> expected_enemies;
    for (const auto& [id, enemy_record] : enemy_by_id) {
        if (enemy_record["hp"].get<int>() > 0) {
            expected_enemies.insert(id);
        }
    }
    std::set<std::string> actor_ids;
    std::set<std::string> battle_enemies;
    std::set<std::string> expected_party;
    for (const auto& id_value : active_party) {
        const auto& member = party_by_id.at(id_value);
        if (member["hp"].get<int>() > 0) {
            expected_party.insert(id_value);
        }
    }
    std::set<std::string> battle_party;
    std::set<Hex> positions;
    bool living_party = false;
    bool living_enemy = false;
    for (const auto& actor : battle["actors"]) {
        if (!actor.is_object()) {
            throw GameError("save battle actor is not an object");
        }
        for (const char* key : {"id", "name", "team", "class", "visual_id"}) {
            require_save_string(actor, key, "battle actor");
        }
        if (!member_of(actor["team"].get<std::string>(), {"party", "enemy"})) {
            throw GameError("save battle actor team is invalid");
        }
        if (!actor.contains("pos") || !actor["pos"].is_array()) {
            throw GameError("save battle actor is missing position");
        }
        hex(actor["pos"]);
        if (!walkable(run["layout"], hex(actor["pos"]))) {
            throw GameError("save battle actor is outside the walkable layout");
        }
        if (actor["hp"].get<int>() > 0 && !positions.insert(hex(actor["pos"])).second) {
            throw GameError("save battle actors overlap");
        }
        for (const char* key : {"hp", "max_hp", "armor", "evasion", "accuracy", "damage", "range",
                                "speed", "move_left", "poison"}) {
            require_save_integer(actor, key, "battle actor");
        }
        if (actor["team"] == "party") {
            require_save_integer(actor, "spell_uses", "battle actor");
        }
        for (const char* key : {"acted", "defending"}) {
            require_save_bool(actor, key, "battle actor");
        }
        const std::string id = actor["id"].get<std::string>();
        const int hp = actor["hp"].get<int>();
        const int max_hp = actor["max_hp"].get<int>();
        if (max_hp <= 0 || hp < 0 || hp > max_hp || actor["armor"].get<int>() < 0 ||
            actor["evasion"].get<int>() < 0 || actor["accuracy"].get<int>() < 0 ||
            actor["damage"].get<int>() < 0 || actor["range"].get<int>() <= 0 ||
            actor["speed"].get<int>() <= 0 || actor["move_left"].get<int>() < 0 ||
            actor["move_left"].get<int>() > 4 || actor["poison"].get<int>() < 0) {
            throw GameError("save battle actor has an invalid combat range");
        }
        if (actor["team"] == "party") {
            if (!active_party.contains(id) || !party_by_id.contains(id) ||
                !battle_party.insert(id).second) {
                throw GameError("save battle party actor is not active");
            }
            const auto& member = party_by_id.at(id);
            for (const char* key : {"name", "class", "visual_id"}) {
                if (actor[key] != member[key]) {
                    throw GameError("save battle party identity does not match party state");
                }
            }
            if (actor["spell_uses"].get<int>() < 0) {
                throw GameError("save battle spell uses are invalid");
            }
            living_party = living_party || hp > 0;
        } else {
            if (!enemy_by_id.contains(id) || !battle_enemies.insert(id).second) {
                throw GameError("save battle enemy identity is invalid");
            }
            const auto& persisted = enemy_by_id.at(id);
            for (const char* key : {"name", "class", "visual_id"}) {
                if (actor[key] != persisted[key]) {
                    throw GameError("save battle enemy identity does not match run state");
                }
            }
            living_enemy = living_enemy || hp > 0;
        }
        if (!actor_ids.insert(id).second) {
            throw GameError("save battle actor IDs are not unique");
        }
    }
    if (battle_enemies != expected_enemies) {
        throw GameError("save battle enemy membership does not match run state");
    }
    if (battle_party != expected_party || !living_party || !living_enemy) {
        throw GameError("save battle party membership or living actors are inconsistent");
    }
    if (battle["turn_index"].get<int>() < 0 ||
        battle["turn_index"].get<std::size_t>() >= battle["order"].size()) {
        throw GameError("save battle turn index is invalid");
    }
    std::set<std::string> ordered_ids;
    for (const auto& id : battle["order"]) {
        if (!id.is_string() || !actor_ids.contains(id.get<std::string>()) ||
            !ordered_ids.insert(id.get<std::string>()).second) {
            throw GameError("save battle order references an invalid actor");
        }
    }
    if (ordered_ids.size() != actor_ids.size()) {
        throw GameError("save battle order does not include every actor");
    }
}

void validate_saved_state(const json& loaded) {
    for (const char* key :
         {"schema_version", "next_id", "next_event", "rng", "clock_minutes", "mine_floor"}) {
        require_save_integer(loaded, key, "state");
    }
    require_save_string(loaded, "template_id", "state");
    require_save_string(loaded, "phase", "state");
    require_save_string(loaded, "hero_id", "state");
    require_save_string(loaded, "active_run", "state");
    for (const char* key : {"party", "active_party", "events"}) {
        if (!loaded.contains(key) || !loaded[key].is_array()) {
            throw GameError(std::string("save is missing array: state.") + key);
        }
    }
    if (!loaded.contains("runs") || !loaded["runs"].is_object()) {
        throw GameError("save is missing object: state.runs");
    }
    std::set<std::string> party_ids;
    for (const auto& member : loaded["party"]) {
        if (!member.is_object()) {
            throw GameError("save party member is not an object");
        }
        for (const char* key : {"id", "name", "ancestry", "background", "class", "visual_id"}) {
            require_save_string(member, key, "party member");
        }
        for (const char* key :
             {"level", "xp", "hp", "max_hp", "wounds", "spell_uses", "max_spell_uses"}) {
            require_save_integer(member, key, "party member");
        }
        const std::string id = member["id"].get<std::string>();
        if (!party_ids.insert(id).second) {
            throw GameError("save party member IDs are not unique");
        }
    }
    std::set<std::string> active_ids;
    std::map<std::string, json> party_by_id;
    for (const auto& member : loaded["party"]) {
        party_by_id.emplace(member["id"].get<std::string>(), member);
    }
    for (const auto& id : loaded["active_party"]) {
        if (!id.is_string() || !active_ids.insert(id.get<std::string>()).second ||
            !party_ids.contains(id.get<std::string>())) {
            throw GameError("save active party references an invalid or duplicate member");
        }
    }
    const std::string hero_id = loaded["hero_id"].get<std::string>();
    if (!hero_id.empty() && !party_ids.contains(hero_id)) {
        throw GameError("save hero is absent from the party");
    }
    for (const char* key : {"mine_pos", "mine_previous_pos"}) {
        if (!loaded.contains(key) || !loaded[key].is_array()) {
            throw GameError(std::string("save is missing coordinates: state.") + key);
        }
        hex(loaded[key]);
    }
    require_save_object(loaded, "inventory", "state");
    require_save_object(loaded, "equipment", "state");
    for (const char* key : {"gold", "camp_supplies", "ore"}) {
        require_save_integer(loaded["inventory"], key, "inventory");
    }
    require_save_string(loaded["equipment"], "id", "equipment");
    require_save_integer(loaded["equipment"], "tier", "equipment");
    require_save_object(loaded, "world", "state");
    if (!loaded["world"].contains("pos") || !loaded["world"]["pos"].is_array()) {
        throw GameError("save world is missing position");
    }
    hex(loaded["world"]["pos"]);
    if (!loaded["world"].contains("discovered") || !loaded["world"]["discovered"].is_array()) {
        throw GameError("save world is missing discoveries");
    }
    require_save_bool(loaded["world"], "ruins_inspected", "world");
    if (!loaded.contains("battle")) {
        throw GameError("save is missing battle state");
    }
    if (loaded["mine_floor"].get<int>() < 1 || loaded["mine_floor"].get<int>() > 2) {
        throw GameError("save floor is invalid");
    }
    const std::string phase = loaded["phase"].get<std::string>();
    const std::string active_run = loaded["active_run"].get<std::string>();
    if ((phase == "dungeon" || phase == "battle") &&
        (active_run.empty() || !loaded["runs"].contains(active_run))) {
        throw GameError("save active run is required during dungeon or battle");
    }
    if (phase == "battle") {
        const std::string active_run = loaded["active_run"].get<std::string>();
        validate_saved_battle(loaded["battle"], loaded["runs"].at(active_run), active_ids,
                              party_by_id, loaded["mine_floor"].get<int>());
    } else if (!loaded["battle"].is_null()) {
        throw GameError("save battle state is inconsistent with phase");
    }
}

void validate_saved_run(const json& run, const std::string& id, const std::string& template_id) {
    if (!run.is_object()) {
        throw GameError("saved adventure run is not an object");
    }
    for (const char* key :
         {"id", "template_id", "mode", "layout_source", "participant", "location", "reward"}) {
        require_save_string(run, key, "run");
    }
    require_save_integer(run, "content_schema_version", "run");
    if (run["content_schema_version"].get<int>() != 2) {
        throw GameError("saved run content schema is unsupported");
    }
    require_save_object(run, "rewards", "run");
    for (const char* key : {"cache_gold", "ore_quantity", "main_xp", "secret_xp", "secret_gold"}) {
        require_save_integer(run["rewards"], key, "run rewards");
        if (run["rewards"][key].get<int>() <= 0) {
            throw GameError("save run reward quantity is invalid");
        }
    }
    require_save_object(run, "bindings", "run");
    require_save_object(run["bindings"], "objects", "run bindings");
    for (const char* key : {"main_enemy_ids", "secret_enemy_ids"}) {
        if (!run["bindings"].contains(key) || !run["bindings"][key].is_array()) {
            throw GameError(std::string("save run bindings is missing array: ") + key);
        }
    }
    if (run["id"] != id || run["template_id"] != template_id ||
        !member_of(run["mode"].get<std::string>(), {"authored", "generated"}) ||
        !member_of(run["layout_source"].get<std::string>(),
                   {"authored", "generated", "authored_fallback"})) {
        throw GameError("saved adventure identity or mode is inconsistent");
    }
    require_save_bool(run, "boss", "run");
    if (!run.contains("generation_diagnostics") || !run["generation_diagnostics"].is_array()) {
        throw GameError("save run generation diagnostics must be an array");
    }
    require_save_object(run, "layout", "run");
    require_save_object(run, "changes", "run");
    for (const char* key :
         {"hidden_loot_taken", "secret_found", "secret_won", "main_won", "ore_taken", "upgraded"}) {
        require_save_bool(run["changes"], key, "run changes");
    }
    const auto& changes = run["changes"];
    if (changes["secret_won"].get<bool>() && !changes["secret_found"].get<bool>()) {
        throw GameError("save run secret progress is inconsistent");
    }
    if (changes["ore_taken"].get<bool>() && !changes["main_won"].get<bool>()) {
        throw GameError("save run ore progress is inconsistent");
    }
    if (changes["upgraded"].get<bool>() && !changes["ore_taken"].get<bool>()) {
        throw GameError("save run upgrade progress is inconsistent");
    }
    for (const char* key : {"main_enemies", "secret_enemies"}) {
        if (!run.contains(key) || !run[key].is_array() || run[key].empty()) {
            throw GameError(std::string("save run is missing enemy array: ") + key);
        }
        std::set<std::string> enemy_ids;
        for (const auto& enemy_record : run[key]) {
            validate_enemy_record(enemy_record, id, key);
            if (!enemy_ids.insert(enemy_record["id"].get<std::string>()).second) {
                throw GameError(std::string("save run ") + key + " IDs are not unique");
            }
        }
    }
    const auto binding_ids = [](const json& records) {
        json ids = json::array();
        for (const auto& record : records) {
            ids.push_back(record["id"]);
        }
        return ids;
    };
    if (run["bindings"]["objects"] != run["layout"]["objects"] ||
        run["bindings"]["main_enemy_ids"] != binding_ids(run["main_enemies"]) ||
        run["bindings"]["secret_enemy_ids"] != binding_ids(run["secret_enemies"])) {
        throw GameError("save run bindings do not match resolved content");
    }
    std::set<std::string> all_enemy_ids;
    for (const char* key : {"main_enemies", "secret_enemies"}) {
        for (const auto& enemy_record : run[key]) {
            if (!all_enemy_ids.insert(enemy_record["id"].get<std::string>()).second) {
                throw GameError("save run enemy IDs are not unique across encounters");
            }
        }
        const bool won =
            key[0] == 'm' ? changes["main_won"].get<bool>() : changes["secret_won"].get<bool>();
        bool living = false;
        for (const auto& enemy_record : run[key]) {
            living = living || enemy_record["hp"].get<int>() > 0;
        }
        if (won && living) {
            throw GameError(std::string("save run ") + key + " remains unresolved");
        }
        if (!won && !living) {
            throw GameError(std::string("save run ") + key + " is complete without progress");
        }
    }
    validate_layout(run["layout"], 12);
    for (const char* key : {"main_enemies", "secret_enemies"}) {
        std::set<Hex> enemy_positions;
        for (const auto& enemy_record : run[key]) {
            const Hex position = hex(enemy_record["pos"]);
            if (!walkable(run["layout"], position) ||
                (enemy_record["hp"].get<int>() > 0 && !enemy_positions.insert(position).second)) {
                throw GameError("save run enemy placement is invalid");
            }
        }
    }
    if (run["boss"].get<bool>() != (run["layout"]["floors"].size() == 2)) {
        throw GameError("saved floor identity is inconsistent");
    }
}

} // namespace

struct OdrSession {
    json definition;
    json state;
    std::string response;
    std::string error;

    explicit OdrSession(json source) : definition(std::move(source)) {
        if (definition.value("schema_version", 0) != 2 || !definition.contains("mine") ||
            !definition.contains("recruits") || !definition.contains("defaults") ||
            !definition.contains("encounters")) {
            throw GameError("unsupported or incomplete adventure definition");
        }
        validate_content_definition(definition);
        const std::string template_id = required_string(definition, "template_id");
        state = {{"schema_version", 2},
                 {"template_id", template_id},
                 {"phase", "creation"},
                 {"next_id", 1},
                 {"next_event", 1},
                 {"rng", static_cast<std::uint64_t>(0x123456789abcdefULL)},
                 {"clock_minutes", 480},
                 {"hero_id", ""},
                 {"party", json::array()},
                 {"active_party", json::array()},
                 {"inventory", {{"gold", 30}, {"camp_supplies", 1}, {"ore", 0}}},
                 {"equipment", {{"id", "signature_blade"}, {"tier", 0}}},
                 {"world",
                  {{"pos", json::array({0, 0})},
                   {"discovered", json::array({"city"})},
                   {"ruins_inspected", false}}},
                 {"runs", json::object()},
                 {"active_run", ""},
                 {"mine_pos", json::array({0, 0})},
                 {"mine_previous_pos", json::array({0, 0})},
                 {"mine_floor", 1},
                 {"battle", nullptr},
                 {"events", json::array()}};
    }

    void event(const std::string& kind, const std::string& description) {
        const auto id = state["next_event"].get<std::uint64_t>();
        state["next_event"] = id + 1;
        state["events"].push_back({{"id", id}, {"kind", kind}, {"text", description}});
    }

    json& run() {
        const std::string id = state["active_run"].get<std::string>();
        if (id.empty() || !state["runs"].contains(id)) {
            throw GameError("no selected adventure run");
        }
        return state["runs"][id];
    }

    json& actor(const std::string& id) {
        for (auto& member : state["party"]) {
            if (member["id"] == id) {
                return member;
            }
        }
        throw GameError("unknown character: " + id);
    }

    void require_phase(const std::string& phase) const {
        if (state["phase"] != phase) {
            throw GameError("action requires " + phase + " phase");
        }
    }

    void add_minutes(int minutes) {
        state["clock_minutes"] = state["clock_minutes"].get<int>() + minutes;
    }

    void create_run(const json& command) {
        if (state["phase"] != "city" && state["phase"] != "overworld") {
            throw GameError("adventures are accepted in town or on the overworld");
        }
        const std::string mode = command.value("mode", "authored");
        if (mode != "authored" && mode != "generated") {
            throw GameError("mode must be authored or generated");
        }
        const auto seed = command.value("seed", static_cast<std::uint64_t>(17));
        const bool boss = command.value("boss", false);
        json layout;
        json diagnostics = json::array();
        std::string source = mode;
        if (mode == "generated") {
            const int attempts = command.value("max_attempts", 8);
            if (attempts < 1 || attempts > 16) {
                throw GameError("generated layout attempts must be between one and sixteen");
            }
            const json candidates = command.value("candidate_layouts", json::array());
            if (!candidates.is_array()) {
                throw GameError("candidate layouts must be an array");
            }
            for (int attempt = 0; attempt < attempts; ++attempt) {
                json candidate =
                    candidates.empty()
                        ? make_layout(definition, seed + static_cast<std::uint64_t>(attempt), true,
                                      boss)
                        : candidates[static_cast<std::size_t>(attempt) % candidates.size()];
                try {
                    validate_encounter_bindings(candidate, definition, boss);
                    validate_layout(candidate, 12);
                    if ((candidate["floors"].size() == 2) != boss) {
                        throw GameError("candidate floor count does not match boss variant");
                    }
                    layout = std::move(candidate);
                    break;
                } catch (const std::exception& error) {
                    diagnostics.push_back({{"attempt", attempt + 1}, {"reason", error.what()}});
                }
            }
            if (layout.is_null()) {
                layout = make_layout(definition, seed, false, boss);
                validate_encounter_bindings(layout, definition, boss);
                validate_layout(layout, 12);
                source = "authored_fallback";
            }
        } else {
            layout = make_layout(definition, seed, false, boss);
            validate_encounter_bindings(layout, definition, boss);
            validate_layout(layout, 12);
        }
        const std::string id = next_id(state, "run");
        const json& defaults = definition["defaults"];
        const auto run_string = [&command](const char* key, const std::string& fallback) {
            if (!command.contains(key)) {
                return fallback;
            }
            const std::string value = required_string(command, key);
            if (value.empty()) {
                throw GameError(std::string("run ") + key + " must not be empty");
            }
            return value;
        };
        const std::string participant =
            run_string("participant", required_string(defaults, "participant"));
        const std::string location = run_string("location", required_string(defaults, "location"));
        const std::string reward = run_string("reward", required_string(defaults, "reward"));
        const json& encounter_defs = definition["encounters"];
        auto build_enemies = [&id, &layout](const json& specs, bool include_boss,
                                            const char* encounter) {
            json result = json::array();
            for (const auto& spec : specs) {
                if (include_boss || !spec.value("boss", false)) {
                    json resolved = spec;
                    const std::string suffix = required_string(spec, "suffix");
                    for (const auto& spawn : layout["enemy_positions"][encounter]) {
                        if (spawn["name"] == suffix) {
                            resolved["pos"] = spawn["pos"];
                            break;
                        }
                    }
                    result.push_back(enemy(id + "-" + suffix, resolved));
                }
            }
            return result;
        };
        json main_enemies = build_enemies(encounter_defs.at("main"), boss, "main");
        json secret_enemies = build_enemies(encounter_defs.at("secret"), true, "secret");
        json main_enemy_ids = json::array();
        for (const auto& record : main_enemies) {
            main_enemy_ids.push_back(record["id"]);
        }
        json secret_enemy_ids = json::array();
        for (const auto& record : secret_enemies) {
            secret_enemy_ids.push_back(record["id"]);
        }
        json adventure = {{"id", id},
                          {"template_id", state["template_id"]},
                          {"content_schema_version", definition["schema_version"]},
                          {"mode", mode},
                          {"boss", boss},
                          {"layout_source", source},
                          {"generation_diagnostics", diagnostics},
                          {"participant", participant},
                          {"location", location},
                          {"reward", reward},
                          {"rewards",
                           {{"cache_gold", defaults.at("cache_gold")},
                            {"ore_quantity", defaults.at("ore_quantity")},
                            {"main_xp", defaults.at("main_xp")},
                            {"secret_xp", defaults.at("secret_xp")},
                            {"secret_gold", defaults.at("secret_gold")}}},
                          {"bindings",
                           {{"objects", layout["objects"]},
                            {"main_enemy_ids", main_enemy_ids},
                            {"secret_enemy_ids", secret_enemy_ids}}},
                          {"layout", std::move(layout)},
                          {"changes",
                           {{"hidden_loot_taken", false},
                            {"secret_found", false},
                            {"secret_won", false},
                            {"main_won", false},
                            {"ore_taken", false},
                            {"upgraded", false}}},
                          {"main_enemies", std::move(main_enemies)},
                          {"secret_enemies", std::move(secret_enemies)}};
        state["runs"][id] = std::move(adventure);
        state["active_run"] = id;
        if (source == "authored_fallback") {
            event("generation_fallback", "generated candidates rejected; authored mine accepted");
        }
        event("adventure_accepted", id);
    }

    void apply(const json& command);
    void begin_battle(bool secret);
    void battle_command(const json& command);
    void advance_turn();
    void attack(json& attacker, json& defender, bool magic = false);
    void finish_battle(bool won);
    void sync_party();
};

void OdrSession::apply(const json& command) {
    const std::string action = required_string(command, "action");
    state["events"] = json::array();
    if (action == "create_hero") {
        require_phase("creation");
        const std::string name = required_string(command, "name");
        const std::string ancestry = required_string(command, "ancestry");
        const std::string background = required_string(command, "background");
        const std::string role = required_string(command, "class");
        if (name.empty() || name.size() > 32 ||
            !member_of(ancestry, {"Human", "Elf", "Dwarf", "Orc"}) ||
            !member_of(background, {"Guard", "Scholar", "Outlaw"}) ||
            !member_of(role, {"Warrior", "Rogue", "Ranger", "Mage", "Cleric", "Barbarian"})) {
            throw GameError("invalid hero name, ancestry, background or class");
        }
        const std::string id = next_id(state, "hero");
        state["hero_id"] = id;
        state["party"].push_back(character(id, name, ancestry, background, role));
        state["active_party"].push_back(id);
        state["phase"] = "city";
        event("hero_created", name + " begins in the trading town");
    } else if (action == "recruit") {
        require_phase("city");
        const std::string recruit_id = required_string(command, "id");
        const json* recruit = nullptr;
        for (const auto& candidate : definition["recruits"]) {
            if (candidate.value("id", "") == recruit_id) {
                recruit = &candidate;
            }
        }
        if (recruit == nullptr) {
            throw GameError("unknown recruit");
        }
        for (const auto& member : state["party"]) {
            if (member["id"] == recruit_id) {
                throw GameError("character already recruited");
            }
        }
        json member = character(recruit_id, required_string(*recruit, "name"), "Human", "Guard",
                                required_string(*recruit, "class"));
        state["party"].push_back(member);
        if (state["active_party"].size() < 8) {
            state["active_party"].push_back(recruit_id);
        }
        event("recruited", member["name"].get<std::string>() + " joins the roster");
    } else if (action == "set_active") {
        require_phase("city");
        if (!command.contains("ids") || !command["ids"].is_array() || command["ids"].empty() ||
            command["ids"].size() > 12) {
            throw GameError("active party must contain 1 to 12 characters");
        }
        std::set<std::string> selected;
        for (const auto& id : command["ids"]) {
            if (!id.is_string() || !selected.insert(id.get<std::string>()).second) {
                throw GameError("duplicate or invalid party ID");
            }
            (void)actor(id.get<std::string>());
        }
        if (!selected.contains(state["hero_id"].get<std::string>())) {
            throw GameError("active party must include the hero");
        }
        state["active_party"] = command["ids"];
        event("party_changed", "active roster changed");
    } else if (action == "create_run") {
        create_run(command);
    } else if (action == "select_run") {
        if (state["phase"] != "city" && state["phase"] != "overworld") {
            throw GameError("select a run outside the mine");
        }
        const std::string id = required_string(command, "id");
        if (!state["runs"].contains(id)) {
            throw GameError("unknown adventure run");
        }
        state["active_run"] = id;
    } else if (action == "leave_city") {
        require_phase("city");
        state["phase"] = "overworld";
        event("travel_started", "the party leaves town");
    } else if (action == "enter_city") {
        require_phase("overworld");
        if (hex(state["world"]["pos"]) != Hex{0, 0}) {
            throw GameError("travel back to the city marker first");
        }
        state["phase"] = "city";
        event("city_entered", "the party returns to town");
    } else if (action == "travel") {
        require_phase("overworld");
        Hex destination{required_int(command, "q"), required_int(command, "r")};
        if (distance(hex(state["world"]["pos"]), destination) != 1 ||
            distance({0, 0}, destination) > 5) {
            throw GameError("travel requires an adjacent traversable hex");
        }
        state["world"]["pos"] = tile(destination);
        add_minutes(30);
        for (const auto& [name, site] :
             std::vector<std::pair<std::string, Hex>>{{"mine", {3, 1}}, {"ruins", {-2, 1}}}) {
            if (distance(destination, site) <= 1) {
                auto& found = state["world"]["discovered"];
                if (std::find(found.begin(), found.end(), name) == found.end()) {
                    found.push_back(name);
                    event("discovery", "discovered " + name);
                }
            }
        }
    } else if (action == "inspect_ruins") {
        require_phase("overworld");
        if (hex(state["world"]["pos"]) != Hex{-2, 1}) {
            throw GameError("travel to the ruins first");
        }
        if (!state["world"]["ruins_inspected"].get<bool>()) {
            state["world"]["ruins_inspected"] = true;
            add_minutes(20);
            event("discovery", "an old route marker hints at the mine's history");
        }
    } else if (action == "enter_mine") {
        require_phase("overworld");
        if (hex(state["world"]["pos"]) != Hex{3, 1}) {
            throw GameError("travel to the mine first");
        }
        json& adventure = run();
        validate_layout(adventure["layout"], static_cast<int>(state["active_party"].size()));
        state["mine_pos"] = adventure["layout"]["objects"]["entry"];
        state["mine_previous_pos"] = state["mine_pos"];
        state["mine_floor"] = 1;
        state["phase"] = "dungeon";
        event("mine_entered", "the party enters " + adventure["location"].get<std::string>());
    } else if (action == "leave_mine") {
        require_phase("dungeon");
        if (state["mine_floor"].get<int>() != 1) {
            throw GameError("return to the upper floor before leaving");
        }
        if (distance(hex(state["mine_pos"]), hex(run()["layout"]["objects"]["exit"])) > 1) {
            throw GameError("reach the mine exit before leaving");
        }
        state["phase"] = "overworld";
        event("mine_left", "the party returns to the regional map");
    } else if (action == "move_dungeon") {
        require_phase("dungeon");
        Hex destination{required_int(command, "q"), required_int(command, "r")};
        if (distance(hex(state["mine_pos"]), destination) != 1 ||
            !walkable(run()["layout"], destination)) {
            throw GameError("the leader must move to an adjacent walkable hex");
        }
        state["mine_previous_pos"] = state["mine_pos"];
        state["mine_pos"] = tile(destination);
        add_minutes(1);
    } else if (action == "descend" || action == "ascend") {
        require_phase("dungeon");
        json& adventure = run();
        if (!adventure.value("boss", false)) {
            throw GameError("this mine has only one floor");
        }
        const bool descending = action == "descend";
        if (state["mine_floor"].get<int>() != (descending ? 1 : 2)) {
            throw GameError("stairs are not on this floor");
        }
        const char* from = descending ? "stairs_down" : "stairs_up";
        const char* to = descending ? "stairs_up" : "stairs_down";
        if (distance(hex(state["mine_pos"]), hex(adventure["layout"]["objects"][from])) > 1) {
            throw GameError("reach the stairs before changing floors");
        }
        state["mine_floor"] = descending ? 2 : 1;
        state["mine_pos"] = adventure["layout"]["objects"][to];
        state["mine_previous_pos"] = state["mine_pos"];
        add_minutes(2);
        event("floor_changed", descending ? "the party descends to the lowest floor"
                                          : "the party climbs to the upper floor");
    } else if (action == "search") {
        require_phase("dungeon");
        if (state["mine_floor"].get<int>() != 1) {
            throw GameError("the clue lies on the upper floor");
        }
        json& adventure = run();
        if (distance(hex(state["mine_pos"]), hex(adventure["layout"]["objects"]["clue"])) <= 1 ||
            distance(hex(state["mine_pos"]),
                     hex(adventure["layout"]["objects"]["secret_trigger"])) <= 1) {
            adventure["changes"]["secret_found"] = true;
            event("discovery", "scratches and a draught reveal a hidden branch");
        }
        add_minutes(5);
    } else if (action == "take_hidden_loot") {
        require_phase("dungeon");
        if (state["mine_floor"].get<int>() != 1) {
            throw GameError("the cache lies on the upper floor");
        }
        json& adventure = run();
        if (distance(hex(state["mine_pos"]), hex(adventure["layout"]["objects"]["hidden_loot"])) >
                1 ||
            adventure["changes"]["hidden_loot_taken"].get<bool>()) {
            throw GameError("hidden cache is unavailable");
        }
        adventure["changes"]["hidden_loot_taken"] = true;
        const int gold = adventure["rewards"]["cache_gold"].get<int>();
        state["inventory"]["gold"] = state["inventory"]["gold"].get<int>() + gold;
        event("loot", "found an unguarded cache of " + std::to_string(gold) + " gold");
    } else if (action == "begin_main" || action == "begin_secret") {
        require_phase("dungeon");
        begin_battle(action == "begin_secret");
    } else if (action == "take_ore") {
        require_phase("dungeon");
        json& adventure = run();
        if (state["mine_floor"].get<int>() != (adventure.value("boss", false) ? 2 : 1)) {
            throw GameError("the guarded ore lies on the main encounter floor");
        }
        if (!adventure["changes"]["main_won"].get<bool>() ||
            adventure["changes"]["ore_taken"].get<bool>() ||
            distance(hex(state["mine_pos"]), hex(adventure["layout"]["objects"]["ore"])) > 1) {
            throw GameError("the guarded ore is not available");
        }
        adventure["changes"]["ore_taken"] = true;
        const int quantity = adventure["rewards"]["ore_quantity"].get<int>();
        state["inventory"]["ore"] = state["inventory"]["ore"].get<int>() + quantity;
        event("ore_recovered",
              adventure["reward"].get<std::string>() + " recovered for the blacksmith");
    } else if (action == "upgrade") {
        require_phase("city");
        json& adventure = run();
        if (!adventure["changes"]["ore_taken"].get<bool>() ||
            adventure["changes"]["upgraded"].get<bool>() ||
            state["inventory"]["ore"].get<int>() < 1) {
            throw GameError("a new star iron sample is required");
        }
        state["inventory"]["ore"] = state["inventory"]["ore"].get<int>() - 1;
        state["equipment"]["tier"] = state["equipment"]["tier"].get<int>() + 1;
        adventure["changes"]["upgraded"] = true;
        event("equipment_upgraded", "the blacksmith reforges the signature blade");
    } else if (action == "level_up") {
        require_phase("city");
        json& member = actor(required_string(command, "id"));
        const int level = member["level"].get<int>();
        if (level >= 3 || member["xp"].get<int>() < level * 100) {
            throw GameError("this character cannot gain a prototype level yet");
        }
        member["xp"] = member["xp"].get<int>() - level * 100;
        member["level"] = level + 1;
        member["max_hp"] = member["max_hp"].get<int>() + 3;
        member["hp"] = member["hp"].get<int>() + 3;
        event("level_up",
              member["name"].get<std::string>() + " reaches level " + std::to_string(level + 1));
    } else if (action == "rest" || action == "camp") {
        if (action == "rest") {
            require_phase("city");
        } else {
            require_phase("dungeon");
            if (state["inventory"]["camp_supplies"].get<int>() < 1) {
                throw GameError("no camping supplies remain");
            }
            state["inventory"]["camp_supplies"] =
                state["inventory"]["camp_supplies"].get<int>() - 1;
        }
        for (auto& member : state["party"]) {
            member["hp"] = member["max_hp"];
            member["wounds"] = 0;
            member["spell_uses"] = member["max_spell_uses"];
        }
        add_minutes(480);
        event("recovered", "the party rests at a safe place");
    } else if (action == "validate_layout") {
        const json& layout = command.at("layout");
        validate_layout(layout, command.value("party_size", 12));
    } else if (state["phase"] == "battle") {
        battle_command(command);
    } else {
        throw GameError("unknown or unavailable action: " + action);
    }
}

void OdrSession::begin_battle(bool secret) {
    json& adventure = run();
    const int required_floor = secret ? 1 : (adventure.value("boss", false) ? 2 : 1);
    if (state["mine_floor"].get<int>() != required_floor) {
        throw GameError("encounter is on a different mine floor");
    }
    json& changes = adventure["changes"];
    if ((secret && (!changes["secret_found"].get<bool>() || changes["secret_won"].get<bool>())) ||
        (!secret && changes["main_won"].get<bool>())) {
        throw GameError("encounter is not available");
    }
    const char* trigger_name = secret ? "secret_trigger" : "main_trigger";
    const Hex trigger = hex(adventure["layout"]["objects"][trigger_name]);
    const Hex leader_position = hex(state["mine_pos"]);
    if (distance(leader_position, trigger) > 1) {
        throw GameError("reach the encounter from a discovered approach");
    }
    Hex entry = leader_position;
    if (entry == trigger) {
        if (state.contains("mine_previous_pos")) {
            const Hex previous = hex(state["mine_previous_pos"]);
            if (distance(previous, trigger) == 1 && walkable(adventure["layout"], previous)) {
                entry = previous;
            }
        }
        if (entry == trigger) {
            for (const Hex& neighbor : neighbors(trigger)) {
                if (walkable(adventure["layout"], neighbor)) {
                    entry = neighbor;
                    break;
                }
            }
        }
    }
    if (entry == trigger) {
        throw GameError("encounter has no legal entry hex");
    }
    const json& enemies = adventure[secret ? "secret_enemies" : "main_enemies"];
    bool living_enemy = false;
    std::set<Hex> occupied;
    for (const auto& opponent : enemies) {
        if (opponent["hp"].get<int>() > 0) {
            living_enemy = true;
            occupied.insert(hex(opponent["pos"]));
        }
    }
    if (!living_enemy) {
        throw GameError("encounter has no remaining opponents");
    }
    const auto candidates = deployment_hexes(adventure["layout"], trigger, entry, occupied);
    const auto connected = distances(adventure["layout"], entry, occupied);
    std::vector<Hex> deployment;
    std::copy_if(candidates.begin(), candidates.end(), std::back_inserter(deployment),
                 [&connected](Hex point) { return connected.contains(point); });
    if (deployment.size() < state["active_party"].size()) {
        throw GameError("approach has insufficient legal deployment hexes");
    }
    json battle = {{"kind", secret ? "secret" : "main"},
                   {"approach", tile(entry)},
                   {"trigger", tile(trigger)},
                   {"round", 1},
                   {"turn_index", -1},
                   {"actors", json::array()},
                   {"order", json::array()}};
    std::size_t slot = 0;
    for (const auto& id : state["active_party"]) {
        const json& member = actor(id.get<std::string>());
        if (member["hp"].get<int>() > 0) {
            battle["actors"].push_back(combatant(member, deployment[slot++]));
        }
    }
    for (const auto& opponent : enemies) {
        if (opponent["hp"].get<int>() > 0) {
            battle["actors"].push_back(opponent);
        }
    }
    std::vector<std::pair<int, std::string>> initiative;
    for (const auto& participant : battle["actors"]) {
        initiative.emplace_back(-participant["speed"].get<int>(),
                                participant["id"].get<std::string>());
    }
    std::sort(initiative.begin(), initiative.end());
    for (const auto& [speed, id] : initiative) {
        (void)speed;
        battle["order"].push_back(id);
    }
    state["battle"] = std::move(battle);
    state["phase"] = "battle";
    event("battle_started",
          secret ? "an optional cave encounter begins" : "the mine claimants attack");
    advance_turn();
}

void OdrSession::sync_party() {
    if (!state["battle"].is_object()) {
        return;
    }
    for (const auto& participant : state["battle"]["actors"]) {
        if (participant["team"] != "party") {
            continue;
        }
        json& member = actor(participant["id"].get<std::string>());
        member["hp"] = participant["hp"];
        member["spell_uses"] = participant["spell_uses"];
    }
}

void OdrSession::attack(json& attacker, json& defender, bool magic) {
    const int chance =
        std::clamp(75 + attacker["accuracy"].get<int>() - defender["evasion"].get<int>(), 5, 95);
    const int roll = static_cast<int>(next_random(state) % 100ULL) + 1;
    if (roll > chance) {
        event("miss", attacker["name"].get<std::string>() + " misses " +
                          defender["name"].get<std::string>());
        return;
    }
    const int armor =
        magic ? 0 : defender["armor"].get<int>() + (defender["defending"].get<bool>() ? 2 : 0);
    const int damage = std::max(1, attacker["damage"].get<int>() - armor);
    defender["hp"] = std::max(0, defender["hp"].get<int>() - damage);
    if (attacker["class"] == "Rogue" && !magic && defender["hp"].get<int>() > 0) {
        defender["poison"] = std::max(2, defender["poison"].get<int>());
    }
    event("hit", attacker["name"].get<std::string>() + " hits " +
                     defender["name"].get<std::string>() + " for " + std::to_string(damage));
}

void OdrSession::finish_battle(bool won) {
    json& battle = state["battle"];
    json& adventure = run();
    const bool secret = battle["kind"] == "secret";
    const std::string enemy_field = secret ? "secret_enemies" : "main_enemies";
    for (auto& enemy_state : adventure[enemy_field]) {
        for (const auto& participant : battle["actors"]) {
            if (participant["id"] == enemy_state["id"]) {
                enemy_state["hp"] = participant["hp"];
                enemy_state["pos"] = participant["pos"];
                enemy_state["poison"] = participant["poison"];
            }
        }
    }
    sync_party();
    if (won) {
        adventure["changes"][secret ? "secret_won" : "main_won"] = true;
        for (auto& member : state["party"]) {
            member["xp"] = member["xp"].get<int>() +
                           adventure["rewards"][secret ? "secret_xp" : "main_xp"].get<int>();
            if (member["hp"].get<int>() == 0 &&
                std::find(state["active_party"].begin(), state["active_party"].end(),
                          member["id"]) != state["active_party"].end()) {
                member["hp"] = 1;
                member["wounds"] = 1;
            }
        }
        if (secret) {
            state["inventory"]["gold"] = state["inventory"]["gold"].get<int>() +
                                         adventure["rewards"]["secret_gold"].get<int>();
        }
        state["phase"] = "dungeon";
        event("battle_won", secret ? "secret reward claimed" : "the ore route is safe");
    } else {
        state["phase"] = "defeat";
        event("party_defeated", "reload a previous save to continue");
    }
    state["battle"] = nullptr;
}

void OdrSession::advance_turn() {
    for (int guard = 0; guard < 200; ++guard) {
        json& battle = state["battle"];
        bool party_alive = false;
        bool enemies_alive = false;
        for (const auto& participant : battle["actors"]) {
            if (participant["hp"].get<int>() > 0) {
                if (participant["team"] == "party") {
                    party_alive = true;
                } else {
                    enemies_alive = true;
                }
            }
        }
        if (!enemies_alive) {
            finish_battle(true);
            return;
        }
        if (!party_alive) {
            finish_battle(false);
            return;
        }
        int index = battle["turn_index"].get<int>() + 1;
        if (index >= static_cast<int>(battle["order"].size())) {
            index = 0;
            battle["round"] = battle["round"].get<int>() + 1;
            add_minutes(1);
            for (auto& participant : battle["actors"]) {
                participant["move_left"] = participant["class"] == "Mine Warden" ? 3 : 4;
                participant["acted"] = false;
            }
        }
        battle["turn_index"] = index;
        const std::string id = battle["order"][static_cast<std::size_t>(index)].get<std::string>();
        json* current = nullptr;
        for (auto& participant : battle["actors"]) {
            if (participant["id"] == id) {
                current = &participant;
            }
        }
        if (current == nullptr || (*current)["hp"].get<int>() <= 0) {
            continue;
        }
        (*current)["defending"] = false;
        if ((*current)["poison"].get<int>() > 0) {
            (*current)["poison"] = (*current)["poison"].get<int>() - 1;
            (*current)["hp"] = std::max(0, (*current)["hp"].get<int>() - 2);
            event("poison", (*current)["name"].get<std::string>() + " suffers poison");
            if ((*current)["hp"].get<int>() == 0) {
                continue;
            }
        }
        if ((*current)["team"] == "party") {
            sync_party();
            return;
        }
        json* target = nullptr;
        int nearest = std::numeric_limits<int>::max();
        for (auto& participant : battle["actors"]) {
            if (participant["team"] != "party" || participant["hp"].get<int>() <= 0) {
                continue;
            }
            const int range = distance(hex((*current)["pos"]), hex(participant["pos"]));
            if (range < nearest ||
                (range == nearest && target != nullptr &&
                 participant["id"].get<std::string>() < (*target)["id"].get<std::string>())) {
                target = &participant;
                nearest = range;
            }
        }
        if (target == nullptr) {
            continue;
        }
        const json& layout = run()["layout"];
        if (nearest > (*current)["range"].get<int>() ||
            !line_of_sight(layout, hex((*current)["pos"]), hex((*target)["pos"]))) {
            std::set<Hex> occupied;
            for (const auto& participant : battle["actors"]) {
                if (participant["hp"].get<int>() > 0 && participant["id"] != (*current)["id"]) {
                    occupied.insert(hex(participant["pos"]));
                }
            }
            auto reachable = distances(layout, hex((*current)["pos"]), occupied);
            Hex best = hex((*current)["pos"]);
            int best_range = nearest;
            for (const auto& [point, cost] : reachable) {
                const int range = distance(point, hex((*target)["pos"]));
                if (cost <= (*current)["move_left"].get<int>() && range < best_range) {
                    best = point;
                    best_range = range;
                }
            }
            (*current)["pos"] = tile(best);
            nearest = best_range;
        }
        if (nearest <= (*current)["range"].get<int>() &&
            line_of_sight(layout, hex((*current)["pos"]), hex((*target)["pos"]))) {
            attack(*current, *target);
        } else {
            event("enemy_waited", (*current)["name"].get<std::string>() + " waits");
        }
        sync_party();
    }
    throw GameError("battle turn guard exceeded");
}

void OdrSession::battle_command(const json& command) {
    require_phase("battle");
    const std::string action = required_string(command, "action");
    json& battle = state["battle"];
    const int index = battle["turn_index"].get<int>();
    if (index < 0) {
        throw GameError("battle turn has not begun");
    }
    const std::string acting_id =
        battle["order"][static_cast<std::size_t>(index)].get<std::string>();
    json* acting = nullptr;
    for (auto& participant : battle["actors"]) {
        if (participant["id"] == acting_id) {
            acting = &participant;
        }
    }
    if (acting == nullptr || (*acting)["team"] != "party" || (*acting)["hp"].get<int>() <= 0) {
        throw GameError("not a living party member's turn");
    }
    if (action == "move_battle") {
        Hex destination{required_int(command, "q"), required_int(command, "r")};
        std::set<Hex> occupied;
        for (const auto& participant : battle["actors"]) {
            if (participant["hp"].get<int>() > 0 && participant["id"] != acting_id) {
                occupied.insert(hex(participant["pos"]));
            }
        }
        const auto reachable = distances(run()["layout"], hex((*acting)["pos"]), occupied);
        if ((*acting)["acted"].get<bool>() || destination == hex((*acting)["pos"]) ||
            !reachable.contains(destination) ||
            reachable.at(destination) > (*acting)["move_left"].get<int>()) {
            throw GameError("battle move is blocked or exceeds the movement budget");
        }
        (*acting)["move_left"] = (*acting)["move_left"].get<int>() - reachable.at(destination);
        (*acting)["pos"] = tile(destination);
        event("moved", acting_id + " moves to a legal hex");
        return;
    }
    if (action == "retreat") {
        const Hex approach = hex(battle["approach"]);
        for (const auto& participant : battle["actors"]) {
            if (participant["team"] == "party" && participant["hp"].get<int>() > 0 &&
                distance(hex(participant["pos"]), approach) > 2) {
                throw GameError("all conscious companions must reach the entry zone");
            }
        }
        json& adventure = run();
        const char* field = battle["kind"] == "secret" ? "secret_enemies" : "main_enemies";
        for (auto& enemy_state : adventure[field]) {
            for (const auto& participant : battle["actors"]) {
                if (participant["id"] == enemy_state["id"]) {
                    enemy_state["hp"] = participant["hp"];
                    enemy_state["pos"] = participant["pos"];
                    enemy_state["poison"] = participant["poison"];
                }
            }
        }
        sync_party();
        for (auto& member : state["party"]) {
            if (member["hp"].get<int>() == 0 &&
                std::find(state["active_party"].begin(), state["active_party"].end(),
                          member["id"]) != state["active_party"].end()) {
                member["hp"] = 1;
                member["wounds"] = 1;
            }
        }
        state["mine_pos"] = battle["approach"];
        state["mine_previous_pos"] = battle["trigger"];
        state["battle"] = nullptr;
        state["phase"] = "dungeon";
        event("retreated", "survivors withdraw with their wounded companions");
        return;
    }
    if ((*acting)["acted"].get<bool>()) {
        throw GameError("acting character has spent the action");
    }
    if (action == "attack" || action == "cast") {
        const std::string target_id = required_string(command, "target");
        json* target = nullptr;
        for (auto& participant : battle["actors"]) {
            if (participant["id"] == target_id) {
                target = &participant;
            }
        }
        if (target == nullptr || (*target)["hp"].get<int>() <= 0) {
            throw GameError("target is absent or down");
        }
        const bool casting = action == "cast";
        const std::string role = (*acting)["class"].get<std::string>();
        const bool healing = casting && role == "Cleric";
        if (casting && role != "Cleric" && role != "Mage") {
            throw GameError("only a prepared caster can cast");
        }
        if ((healing && (*target)["team"] != "party") ||
            (!healing && (*target)["team"] != "enemy")) {
            throw GameError("target belongs to the wrong side");
        }
        const int range = casting ? 3 : (*acting)["range"].get<int>();
        if (distance(hex((*acting)["pos"]), hex((*target)["pos"])) > range ||
            !line_of_sight(run()["layout"], hex((*acting)["pos"]), hex((*target)["pos"]))) {
            throw GameError("target is out of range or line of sight");
        }
        if (casting) {
            if ((*acting)["spell_uses"].get<int>() < 1) {
                throw GameError("no prepared spell uses remain");
            }
            (*acting)["spell_uses"] = (*acting)["spell_uses"].get<int>() - 1;
        }
        if (healing) {
            (*target)["hp"] =
                std::min((*target)["max_hp"].get<int>(), (*target)["hp"].get<int>() + 8);
            event("healed", (*target)["name"].get<std::string>() + " recovers eight health");
        } else {
            attack(*acting, *target, casting);
        }
    } else if (action == "cast_area") {
        if ((*acting)["class"] != "Mage" || (*acting)["spell_uses"].get<int>() < 1) {
            throw GameError("only a prepared mage can cast an area spell");
        }
        const Hex center{required_int(command, "q"), required_int(command, "r")};
        if (distance(hex((*acting)["pos"]), center) > 3 ||
            !line_of_sight(run()["layout"], hex((*acting)["pos"]), center)) {
            throw GameError("area center is out of range or line of sight");
        }
        int targets = 0;
        for (auto& participant : battle["actors"]) {
            if (participant["team"] == "enemy" && participant["hp"].get<int>() > 0 &&
                distance(hex(participant["pos"]), center) <= 1) {
                attack(*acting, participant, true);
                ++targets;
            }
        }
        if (targets == 0) {
            throw GameError("area spell has no living enemy in its footprint");
        }
        (*acting)["spell_uses"] = (*acting)["spell_uses"].get<int>() - 1;
        event("area_spell", "the mage strikes a radius-one hex area");
    } else if (action == "defend") {
        (*acting)["defending"] = true;
        event("defended", (*acting)["name"].get<std::string>() + " braces for an attack");
    } else if (action == "wait") {
        event("waited", (*acting)["name"].get<std::string>() + " waits");
    } else {
        throw GameError("unknown battle action: " + action);
    }
    (*acting)["acted"] = true;
    sync_party();
    advance_turn();
}

extern "C" OdrSession* odr_create(const char* definition_json) {
    try {
        return new OdrSession(parse_object(definition_json));
    } catch (...) {
        return nullptr;
    }
}

extern "C" void odr_destroy(OdrSession* session) {
    delete session;
}

extern "C" int odr_apply(OdrSession* session, const char* command_json) {
    if (session == nullptr) {
        return 0;
    }
    json previous = session->state;
    try {
        session->apply(parse_object(command_json));
        session->error.clear();
        return 1;
    } catch (const std::exception& error) {
        session->state = std::move(previous);
        session->error = error.what();
        return 0;
    }
}

extern "C" int odr_load(OdrSession* session, const char* save_json) {
    if (session == nullptr) {
        return 0;
    }
    try {
        json loaded = parse_object(save_json);
        if (loaded.value("schema_version", 0) != 2 ||
            loaded.value("template_id", "") !=
                session->definition["template_id"].get<std::string>() ||
            !loaded.contains("runs") || !loaded.contains("party") ||
            !loaded.contains("active_party") || !loaded.contains("battle") ||
            !loaded.contains("world") || !loaded.contains("rng") || !loaded.contains("next_id") ||
            !loaded.contains("next_event") || !loaded.contains("mine_floor") ||
            !loaded.contains("phase") || !loaded["runs"].is_object() ||
            !loaded["party"].is_array() || !loaded["active_party"].is_array() ||
            !member_of(loaded["phase"].get<std::string>(),
                       {"creation", "city", "overworld", "dungeon", "battle", "defeat"})) {
            throw GameError("save version or adventure template is incompatible");
        }
        validate_saved_state(loaded);
        const std::string active_run = loaded.value("active_run", "");
        if (!active_run.empty() && !loaded["runs"].contains(active_run)) {
            throw GameError("saved active run is missing");
        }
        for (const auto& [id, run] : loaded["runs"].items()) {
            validate_saved_run(run, id, loaded["template_id"].get<std::string>());
        }
        session->state = std::move(loaded);
        session->error.clear();
        return 1;
    } catch (const std::exception& error) {
        session->error = error.what();
        return 0;
    }
}

extern "C" const char* odr_snapshot(OdrSession* session) {
    if (session == nullptr) {
        return "{}";
    }
    session->response = session->state.dump();
    return session->response.c_str();
}

extern "C" const char* odr_save(OdrSession* session) {
    return odr_snapshot(session);
}

extern "C" const char* odr_error(OdrSession* session) {
    return session == nullptr ? "session is not initialized" : session->error.c_str();
}
