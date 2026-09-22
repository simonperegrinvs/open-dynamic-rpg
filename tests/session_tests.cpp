#include "odr/session.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using json = nlohmann::json;
using Session = std::unique_ptr<OdrSession, decltype(&odr_destroy)>;

namespace {

std::vector<json>* recording = nullptr;

void check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string definition() {
    std::ifstream input("game/content/mine.json");
    check(static_cast<bool>(input), "cannot open adventure definition");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::string quarry_definition() {
    std::ifstream input("game/content/quarry.json");
    check(static_cast<bool>(input), "cannot open quarry definition");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

Session create_from(const std::string& source) {
    Session session(odr_create(source.c_str()), &odr_destroy);
    check(static_cast<bool>(session), "cannot create session");
    return session;
}

Session create() {
    return create_from(definition());
}

json snapshot(const Session& session) {
    return json::parse(odr_snapshot(session.get()));
}

bool send(const Session& session, const json& command) {
    const std::string input = command.dump();
    const bool successful = odr_apply(session.get(), input.c_str()) != 0;
    if (successful && recording != nullptr) {
        recording->push_back(command);
    }
    return successful;
}

void apply(const Session& session, const json& command) {
    if (!send(session, command)) {
        throw std::runtime_error("command " + command.dump() + ": " + odr_error(session.get()));
    }
}

int distance(const json& a, const json& b) {
    const int dq = a[0].get<int>() - b[0].get<int>();
    const int dr = a[1].get<int>() - b[1].get<int>();
    return (std::abs(dq) + std::abs(dr) + std::abs(dq + dr)) / 2;
}

void make_hero(const Session& session, const std::string& role = "Warrior") {
    apply(session, {{"action", "create_hero"},
                    {"name", "Ari"},
                    {"ancestry", "Dwarf"},
                    {"background", "Scholar"},
                    {"class", role}});
}

void walk_world(const Session& session, const std::vector<std::pair<int, int>>& steps) {
    for (auto [q, r] : steps) {
        apply(session, {{"action", "travel"}, {"q", q}, {"r", r}});
    }
}

void walk_mine(const Session& session, const std::vector<std::pair<int, int>>& steps) {
    for (auto [q, r] : steps) {
        apply(session, {{"action", "move_dungeon"}, {"q", q}, {"r", r}});
    }
}

void reach_mine(const Session& session) {
    apply(session, {{"action", "leave_city"}});
    walk_world(session, {{1, 0}, {2, 0}, {3, 0}, {3, 1}});
    apply(session, {{"action", "enter_mine"}});
    walk_mine(session, {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}});
}

void walk_to(const Session& session, std::pair<int, int> destination) {
    const json state = snapshot(session);
    const auto& layout = state["runs"][state["active_run"].get<std::string>()]["layout"];
    const auto origin = std::pair{state["mine_pos"][0].get<int>(), state["mine_pos"][1].get<int>()};
    std::set<std::pair<int, int>> allowed;
    for (const auto& point : layout["tiles"]) {
        allowed.emplace(point[0].get<int>(), point[1].get<int>());
    }
    for (const auto& point : layout["walls"]) {
        allowed.erase({point[0].get<int>(), point[1].get<int>()});
    }
    std::queue<std::pair<int, int>> pending;
    std::map<std::pair<int, int>, std::pair<int, int>> previous;
    pending.push(origin);
    previous[origin] = origin;
    constexpr std::pair<int, int> offsets[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
    while (!pending.empty() && !previous.contains(destination)) {
        const auto point = pending.front();
        pending.pop();
        for (const auto& [dq, dr] : offsets) {
            const auto next = std::pair{point.first + dq, point.second + dr};
            if (allowed.contains(next) && !previous.contains(next)) {
                previous[next] = point;
                pending.push(next);
            }
        }
    }
    check(previous.contains(destination), "mine objective has no walking route");
    std::vector<std::pair<int, int>> route;
    for (auto point = destination; point != origin; point = previous.at(point)) {
        route.push_back(point);
    }
    std::reverse(route.begin(), route.end());
    walk_mine(session, route);
}

struct BattleStats {
    int party_turns = 0;
    int movement_only_turns = 0;
    int max_round = 0;
};

BattleStats win_battle(const Session& session) {
    BattleStats stats;
    for (int turn = 0; turn < 500; ++turn) {
        json state = snapshot(session);
        if (state["phase"] != "battle") {
            check(state["phase"] == "dungeon", "party lost the encounter");
            return stats;
        }
        const json& battle = state["battle"];
        ++stats.party_turns;
        stats.max_round = std::max(stats.max_round, battle["round"].get<int>());
        const auto index = static_cast<std::size_t>(battle["turn_index"].get<int>());
        const std::string id = battle["order"][index].get<std::string>();
        json actor;
        std::vector<json> enemies;
        for (const auto& participant : battle["actors"]) {
            if (participant["id"] == id) {
                actor = participant;
            } else if (participant["team"] == "enemy" && participant["hp"].get<int>() > 0) {
                enemies.push_back(participant);
            }
        }
        check(!actor.is_null() && !enemies.empty(), "invalid battle turn state");
        const auto nearest = [&actor](const json& left, const json& right) {
            return distance(actor["pos"], left["pos"]) < distance(actor["pos"], right["pos"]);
        };
        std::sort(enemies.begin(), enemies.end(), nearest);
        auto use_action = [&session, &actor](const std::vector<json>& targets) {
            for (const auto& target : targets) {
                if (actor["class"] == "Mage" && send(session, {{"action", "cast_area"},
                                                               {"q", target["pos"][0]},
                                                               {"r", target["pos"][1]}})) {
                    return true;
                }
                if (send(session, {{"action", "attack"}, {"target", target["id"]}})) {
                    return true;
                }
                if (send(session, {{"action", "cast"}, {"target", target["id"]}})) {
                    return true;
                }
            }
            return false;
        };
        if (use_action(enemies)) {
            continue;
        }
        const int current_distance = distance(actor["pos"], enemies.front()["pos"]);
        std::vector<json> candidates =
            state["runs"][state["active_run"].get<std::string>()]["layout"]["tiles"]
                .get<std::vector<json>>();
        std::sort(candidates.begin(), candidates.end(),
                  [&enemies](const json& left, const json& right) {
                      return distance(left, enemies.front()["pos"]) <
                             distance(right, enemies.front()["pos"]);
                  });
        for (const auto& point : candidates) {
            if (distance(point, enemies.front()["pos"]) >= current_distance) {
                break;
            }
            if (send(session, {{"action", "move_battle"}, {"q", point[0]}, {"r", point[1]}})) {
                break;
            }
        }
        if (!use_action(enemies)) {
            apply(session, {{"action", "defend"}});
            ++stats.movement_only_turns;
        }
    }
    throw std::runtime_error("battle exceeded 500 party turns");
}

void test_identity_validation_and_save() {
    auto session = create();
    make_hero(session);
    apply(session,
          {{"action", "create_run"}, {"mode", "authored"}, {"participant", "claimants A"}});
    const std::string first_id = snapshot(session)["active_run"].get<std::string>();
    apply(session, {{"action", "create_run"},
                    {"mode", "generated"},
                    {"seed", 42},
                    {"participant", "claimants B"},
                    {"location", "new mine"},
                    {"reward", "other iron"}});
    json before = snapshot(session);
    const std::string second_id = before["active_run"].get<std::string>();
    check(!send(session, {{"action", "create_run"}, {"participant", 7}}),
          "non-string run participant override was accepted");
    check(snapshot(session) == before, "invalid run metadata changed authoritative state");
    check(first_id != second_id, "concurrent runs must have distinct IDs");
    check(before["runs"][first_id]["participant"] != before["runs"][second_id]["participant"],
          "run bindings leaked");
    json candidate = before["runs"][second_id]["layout"];
    candidate["walls"].push_back(candidate["objects"]["ore"]);
    check(!send(session, {{"action", "validate_layout"}, {"layout", candidate}}),
          "blocked objective should be rejected");
    check(snapshot(session) == before, "rejected command changed authoritative state");
    const std::string save = odr_save(session.get());
    auto restored = create();
    check(odr_load(restored.get(), save.c_str()) != 0, "roundtrip load failed");
    check(snapshot(restored) == before, "roundtrip changed the resolved layout or world state");
    json incompatible = before;
    incompatible["schema_version"] = 99;
    const std::string bad = incompatible.dump();
    check(!odr_load(restored.get(), bad.c_str()), "incompatible save was accepted");
    check(snapshot(restored) == before, "failed load overwrote the previous session");
    json legacy = before;
    legacy["schema_version"] = 1;
    check(!odr_load(restored.get(), legacy.dump().c_str()),
          "version-one save was accepted after the schema bump");
    check(snapshot(restored) == before, "legacy save rejection overwrote the previous session");

    const auto expect_bad_save = [&restored, &before](const json& corrupted, const char* message) {
        check(odr_load(restored.get(), corrupted.dump().c_str()) == 0, message);
        check(snapshot(restored) == before, "corrupted save overwrote the previous session");
    };
    json missing_inventory = before;
    missing_inventory.erase("inventory");
    expect_bad_save(missing_inventory, "save without inventory was accepted");
    json missing_equipment = before;
    missing_equipment.erase("equipment");
    expect_bad_save(missing_equipment, "save without equipment was accepted");
    json missing_mine_position = before;
    missing_mine_position.erase("mine_pos");
    expect_bad_save(missing_mine_position, "save without mine position was accepted");
    json missing_party_field = before;
    missing_party_field["party"][0].erase("name");
    expect_bad_save(missing_party_field, "party member without name was accepted");
    json duplicate_party_id = before;
    duplicate_party_id["party"][1]["id"] = duplicate_party_id["party"][0]["id"];
    expect_bad_save(duplicate_party_id, "duplicate party member ID was accepted");
    json invalid_active_member = before;
    invalid_active_member["active_party"][0] = "missing-member";
    expect_bad_save(invalid_active_member, "active party member was not validated");
    json invalid_hero = before;
    invalid_hero["hero_id"] = "missing-hero";
    expect_bad_save(invalid_hero, "hero membership was not validated");

    reach_mine(session);
    apply(session, {{"action", "begin_main"}});
    const json battle_save = json::parse(odr_save(session.get()));
    json missing_order = battle_save;
    missing_order["battle"].erase("order");
    expect_bad_save(missing_order, "battle save without order was accepted");
    json incomplete_order = battle_save;
    incomplete_order["battle"]["order"].erase(0);
    expect_bad_save(incomplete_order, "incomplete battle order was accepted");
    json missing_actor_position = battle_save;
    missing_actor_position["battle"]["actors"][0].erase("pos");
    expect_bad_save(missing_actor_position, "battle save without actor position was accepted");
    json missing_active_run = battle_save;
    missing_active_run.erase("active_run");
    expect_bad_save(missing_active_run, "dungeon save without active run was accepted");
    json missing_run_changes = battle_save;
    missing_run_changes["runs"][missing_run_changes["active_run"].get<std::string>()].erase(
        "changes");
    expect_bad_save(missing_run_changes, "save without active run changes was accepted");
    json missing_enemy_actor = battle_save;
    std::string removed_enemy;
    for (auto it = missing_enemy_actor["battle"]["actors"].begin();
         it != missing_enemy_actor["battle"]["actors"].end(); ++it) {
        if ((*it)["team"] == "enemy") {
            removed_enemy = (*it)["id"].get<std::string>();
            missing_enemy_actor["battle"]["actors"].erase(it);
            break;
        }
    }
    check(!removed_enemy.empty(), "battle fixture has no enemy actor");
    auto& enemy_order = missing_enemy_actor["battle"]["order"];
    enemy_order.erase(std::remove(enemy_order.begin(), enemy_order.end(), removed_enemy),
                      enemy_order.end());
    expect_bad_save(missing_enemy_actor,
                    "save missing a live enemy actor and order entry was accepted");
    json actor_on_dead_cell = battle_save;
    actor_on_dead_cell["battle"]["actors"][1]["hp"] = 0;
    actor_on_dead_cell["battle"]["actors"][0]["pos"] =
        actor_on_dead_cell["battle"]["actors"][1]["pos"];
    check(odr_load(restored.get(), actor_on_dead_cell.dump().c_str()) != 0,
          std::string("valid dead-cell battle save was rejected: ") + odr_error(restored.get()));
    check(snapshot(restored) == actor_on_dead_cell, "dead-cell battle save changed while loading");
    json actor_in_wall = battle_save;
    actor_in_wall["battle"]["actors"][0]["pos"] =
        actor_in_wall["runs"][actor_in_wall["active_run"].get<std::string>()]["layout"]["walls"][0];
    check(odr_load(restored.get(), actor_in_wall.dump().c_str()) == 0,
          "battle actor outside walkable layout was accepted");
    check(snapshot(restored) == actor_on_dead_cell,
          "failed walkability load overwrote the prior valid state");
    json divergent_enemy_hp = battle_save;
    for (auto& actor : divergent_enemy_hp["battle"]["actors"]) {
        if (actor["team"] == "enemy") {
            actor["hp"] = actor["hp"].get<int>() - 1;
            break;
        }
    }
    check(odr_load(restored.get(), divergent_enemy_hp.dump().c_str()) != 0,
          "valid battle HP divergence was rejected");
    check(snapshot(restored) == divergent_enemy_hp, "battle HP divergence changed while loading");
    json inconsistent_progress = before;
    inconsistent_progress["runs"][inconsistent_progress["active_run"].get<std::string>()]["changes"]
                         ["ore_taken"] = true;
    check(odr_load(restored.get(), inconsistent_progress.dump().c_str()) == 0,
          "inconsistent run reward progress was accepted");
    check(snapshot(restored) == divergent_enemy_hp,
          "failed progress load overwrote the prior valid state");
}

void test_world_and_retreat() {
    auto session = create();
    make_hero(session, "Ranger");
    apply(session, {{"action", "create_run"}});
    const std::string town_save = odr_save(session.get());
    auto town_restored = create();
    check(odr_load(town_restored.get(), town_save.c_str()) != 0 &&
              snapshot(town_restored) == snapshot(session),
          "town visit did not roundtrip");
    apply(session, {{"action", "leave_city"}});
    walk_world(session, {{-1, 0}, {-2, 0}, {-2, 1}});
    apply(session, {{"action", "inspect_ruins"}});
    check(snapshot(session)["world"]["ruins_inspected"] == true, "detour did not persist");
    walk_world(session, {{-1, 1}, {0, 1}, {1, 1}, {2, 1}, {3, 1}});
    apply(session, {{"action", "enter_mine"}});
    walk_mine(session, {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}});
    apply(session, {{"action", "begin_main"}});
    check(snapshot(session)["phase"] == "battle", "main encounter did not start");
    const std::string mid_battle = odr_save(session.get());
    auto restored = create();
    check(odr_load(restored.get(), mid_battle.c_str()) != 0,
          std::string("mid-battle load failed: ") + odr_error(restored.get()));
    check(snapshot(restored) == snapshot(session), "mid-battle turn state changed on load");
    apply(session, {{"action", "move_battle"}, {"q", 7}, {"r", 1}});
    const std::string between_move_and_action = odr_save(session.get());
    check(odr_load(restored.get(), between_move_and_action.c_str()) != 0 &&
              snapshot(restored) == snapshot(session),
          "save between movement and action changed the turn");
    apply(session, {{"action", "retreat"}});
    check(snapshot(session)["phase"] == "dungeon", "retreat did not return to mine");
    check(snapshot(session)["runs"][snapshot(session)["active_run"].get<std::string>()]["changes"]
                           ["main_won"] == false,
          "retreat incorrectly completed the main battle");
    const std::string retreat_save = odr_save(session.get());
    check(odr_load(restored.get(), retreat_save.c_str()) != 0 &&
              snapshot(restored) == snapshot(session),
          "retreat changes did not persist through save/load");
    apply(session, {{"action", "begin_main"}});
    check(snapshot(session)["phase"] == "battle", "unfinished encounter did not resume");
}

void test_run_progress_isolation() {
    auto session = create();
    make_hero(session);
    apply(session, {{"action", "create_run"}, {"participant", "first claimants"}});
    const std::string first_id = snapshot(session)["active_run"].get<std::string>();
    apply(session, {{"action", "create_run"},
                    {"mode", "generated"},
                    {"seed", 42},
                    {"participant", "second claimants"}});
    const std::string second_id = snapshot(session)["active_run"].get<std::string>();
    apply(session, {{"action", "select_run"}, {"id", first_id}});
    reach_mine(session);
    walk_to(session, {5, 3});
    apply(session, {{"action", "take_hidden_loot"}});
    walk_to(session, {0, 0});
    apply(session, {{"action", "leave_mine"}});
    apply(session, {{"action", "select_run"}, {"id", second_id}});
    apply(session, {{"action", "enter_mine"}});
    const auto state = snapshot(session);
    check(state["runs"][first_id]["changes"]["hidden_loot_taken"] == true &&
              state["runs"][second_id]["changes"]["hidden_loot_taken"] == false,
          "one expedition's loot state leaked into another");
    walk_to(session, {5, 3});
    apply(session, {{"action", "take_hidden_loot"}});
    check(snapshot(session)["inventory"]["gold"] == 60,
          "two separately bound cache rewards were not independent");
}

void test_recruitment_and_progression() {
    auto session = create();
    make_hero(session);
    for (const char* id : {"rowan", "sable", "tala", "iona", "bran", "kora", "eden", "fenn", "mira",
                           "orin", "pax"}) {
        apply(session, {{"action", "recruit"}, {"id", id}});
    }
    json state = snapshot(session);
    check(state["party"].size() == 12, "twelve-character comparison roster is incomplete");
    check(state["active_party"].size() == 8, "initial active roster should contain eight");
    check(!send(session, {{"action", "recruit"}, {"id", "rowan"}}),
          "duplicate companion recruited");
    std::vector<std::string> ids;
    for (const auto& member : state["party"]) {
        ids.push_back(member["id"].get<std::string>());
    }
    apply(session, {{"action", "set_active"}, {"ids", ids}});
    check(snapshot(session)["active_party"].size() == 12, "twelve-member deployment unavailable");
    apply(session, {{"action", "create_run"}, {"mode", "generated"}, {"seed", 9}});
    reach_mine(session);
    apply(session, {{"action", "begin_main"}});
    check(snapshot(session)["battle"]["actors"].size() == 15,
          "twelve allies and three enemies were not deployed");
}

void test_battle_deployment_follows_entry() {
    auto prepare = [] {
        auto session = create();
        make_hero(session);
        for (const char* id : {"rowan", "sable", "tala", "iona", "bran", "kora", "eden"}) {
            apply(session, {{"action", "recruit"}, {"id", id}});
        }
        apply(session, {{"action", "create_run"}});
        reach_mine(session);
        return session;
    };
    auto west = prepare();
    apply(west, {{"action", "begin_main"}});
    const json west_battle = snapshot(west)["battle"];
    check(west_battle["approach"] == json::array({6, 0}),
          "west approach was not retained as the battle entry zone");

    auto south = prepare();
    apply(south, {{"action", "move_dungeon"}, {"q", 7}, {"r", 1}});
    apply(south, {{"action", "begin_main"}});
    const json south_battle = snapshot(south)["battle"];
    check(south_battle["approach"] == json::array({7, 1}),
          "south approach was not retained as the battle entry zone");
    check(west_battle["actors"] != south_battle["actors"],
          "different exploration approaches produced identical deployment");
    for (const json* battle : {&west_battle, &south_battle}) {
        std::set<std::pair<int, int>> occupied;
        for (const auto& actor : (*battle)["actors"]) {
            const auto position = std::pair{actor["pos"][0].get<int>(), actor["pos"][1].get<int>()};
            check(occupied.insert(position).second, "battle deployment has overlapping actors");
        }
    }
}

void test_complete_adventure(int active_count, bool generated = false) {
    std::vector<json> commands;
    recording = generated ? nullptr : &commands;
    auto session = create();
    make_hero(session, "Ranger");
    const std::vector<std::string> recruits = {"rowan", "sable", "tala", "iona", "bran", "kora",
                                               "eden",  "fenn",  "mira", "orin", "pax"};
    for (int index = 0; index < active_count - 1; ++index) {
        apply(session, {{"action", "recruit"}, {"id", recruits[static_cast<std::size_t>(index)]}});
    }
    if (active_count == 12) {
        json selected = json::array({snapshot(session)["hero_id"]});
        for (const auto& id : recruits) {
            selected.push_back(id);
        }
        apply(session, {{"action", "set_active"}, {"ids", selected}});
    }
    if (generated) {
        apply(session, {{"action", "create_run"}, {"mode", "generated"}, {"seed", 42}});
    } else {
        apply(session, {{"action", "create_run"}});
    }
    const json town_save = snapshot(session);
    reach_mine(session);
    walk_to(session, {4, 3});
    apply(session, {{"action", "search"}});
    walk_to(session, {5, 3});
    apply(session, {{"action", "take_hidden_loot"}});
    check(!send(session, {{"action", "take_hidden_loot"}}), "hidden cache duplicated");
    walk_to(session, {7, 0});
    apply(session, {{"action", "begin_main"}});
    const std::string before_action = odr_save(session.get());
    auto restored = create();
    check(odr_load(restored.get(), before_action.c_str()) != 0,
          "save before a battle action failed to reload");
    check(snapshot(restored) == snapshot(session), "battle state changed on load");
    const auto battle_started = std::chrono::steady_clock::now();
    const BattleStats battle_stats = win_battle(session);
    const auto battle_ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - battle_started)
            .count();
    if (std::getenv("ODR_MEASURE") != nullptr) {
        std::cout << "ODR_BATTLE party=" << active_count << " generated=" << generated
                  << " party_turns=" << battle_stats.party_turns
                  << " movement_only_turns=" << battle_stats.movement_only_turns
                  << " rounds=" << battle_stats.max_round << " elapsed_ms=" << battle_ms << '\n';
    }
    const json after_battle = snapshot(session);
    check(after_battle["runs"][after_battle["active_run"].get<std::string>()]["changes"]
                      ["main_won"] == true,
          "main fight did not clear the ore route");
    walk_to(session, {14, 0});
    apply(session, {{"action", "take_ore"}});
    check(!send(session, {{"action", "take_ore"}}), "ore reward duplicated");
    walk_to(session, {0, 0});
    apply(session, {{"action", "leave_mine"}});
    walk_world(session, {{3, 0}, {2, 0}, {1, 0}, {0, 0}});
    apply(session, {{"action", "enter_city"}});
    apply(session, {{"action", "upgrade"}});
    check(!send(session, {{"action", "upgrade"}}), "equipment upgrade duplicated");
    const std::string final_save = odr_save(session.get());
    check(odr_load(restored.get(), final_save.c_str()) != 0, "finished save did not reload");
    check(snapshot(restored) == snapshot(session), "finished adventure changed on load");
    check(snapshot(session)["equipment"]["tier"] == 1, "equipment upgrade was not lasting");
    check(town_save["equipment"]["tier"] == 0, "initial equipment tier changed");
    recording = nullptr;
    if (const char* path = generated ? nullptr
                                     : std::getenv(active_count == 12 ? "ODR_RECORD_SCENARIO_12"
                                                                      : "ODR_RECORD_SCENARIO")) {
        std::ofstream output(path, std::ios::trunc);
        check(static_cast<bool>(output), "cannot write recorded scenario");
        for (const auto& command : commands) {
            output << command.dump() << '\n';
        }
        check(static_cast<bool>(output), "recorded scenario write failed");
    }
}

void test_generated_fallback() {
    auto session = create();
    make_hero(session);
    apply(session, {{"action", "create_run"}});
    const auto before = snapshot(session);
    const json authored = before["runs"][before["active_run"].get<std::string>()]["layout"];
    json unreachable = authored;
    unreachable["walls"].push_back(unreachable["objects"]["ore"]);
    check(!send(session, {{"action", "validate_layout"}, {"layout", unreachable}}),
          "unreachable generated candidate was accepted");
    check(std::string(odr_error(session.get())).find("unreachable objective") != std::string::npos,
          "unreachable diagnostic did not identify objective access");
    json blocked_spawn = authored;
    blocked_spawn["walls"].push_back({13, -2});
    check(!send(session, {{"action", "validate_layout"}, {"layout", blocked_spawn}}),
          "blocked enemy spawn was accepted");
    check(std::string(odr_error(session.get())).find("spawn") != std::string::npos,
          "enemy spawn diagnostic was not reported");
    json crowded = authored;
    crowded["tiles"] = json::array();
    crowded["walls"] = json::array();
    for (int q = 0; q <= 17; ++q) {
        crowded["tiles"].push_back({q, 0});
    }
    for (int r = 1; r <= 3; ++r) {
        crowded["tiles"].push_back({4, r});
    }
    crowded["tiles"].push_back({5, 3});
    crowded["tiles"].push_back({5, 4});
    crowded["tiles"].push_back({6, 4});
    crowded["tiles"].push_back({6, 5});
    crowded["tiles"].push_back({13, -2});
    crowded["tiles"].push_back({14, 2});
    crowded["tiles"].push_back({15, 0});
    crowded["tiles"].push_back({12, 4});
    check(!send(session, {{"action", "validate_layout"}, {"layout", crowded}}),
          "overcrowded generated candidate was accepted");
    check(std::string(odr_error(session.get())).find("deployment") != std::string::npos,
          "overcrowding diagnostic did not identify deployment");
    apply(session, {{"action", "create_run"},
                    {"mode", "generated"},
                    {"seed", 91},
                    {"max_attempts", 2},
                    {"candidate_layouts", json::array({unreachable, crowded})}});
    const auto state = snapshot(session);
    const auto run = state["runs"][state["active_run"].get<std::string>()];
    check(run["layout_source"] == "authored_fallback", "bounded fallback was not used");
    check(run["generation_diagnostics"].size() == 2, "rejected candidates were not diagnosed");
    check(run["layout"]["seed"] == 91, "accepted fallback seed was not recorded");

    auto moved_session = create();
    make_hero(moved_session);
    apply(moved_session, {{"action", "create_run"}});
    json moved_layout = snapshot(
        moved_session)["runs"][snapshot(moved_session)["active_run"].get<std::string>()]["layout"];
    moved_layout["enemy_positions"]["main"][0]["pos"] = {13, -1};
    apply(moved_session, {{"action", "create_run"},
                          {"mode", "generated"},
                          {"seed", 7},
                          {"candidate_layouts", json::array({moved_layout})}});
    const json moved_run =
        snapshot(moved_session)["runs"][snapshot(moved_session)["active_run"].get<std::string>()];
    check(moved_run["layout"]["enemy_positions"]["main"][0]["pos"] == json::array({13, -1}) &&
              moved_run["main_enemies"][0]["pos"] == json::array({13, -1}),
          "accepted candidate enemy position was not authoritative");
    auto incomplete_session = create();
    make_hero(incomplete_session);
    apply(incomplete_session, {{"action", "create_run"}});
    json incomplete_layout = snapshot(
        incomplete_session)["runs"][snapshot(incomplete_session)["active_run"].get<std::string>()]
                           ["layout"];
    incomplete_layout["enemy_positions"]["main"] = json::array();
    apply(incomplete_session, {{"action", "create_run"},
                               {"mode", "generated"},
                               {"candidate_layouts", json::array({incomplete_layout})},
                               {"max_attempts", 1}});
    const json incomplete_run = snapshot(
        incomplete_session)["runs"][snapshot(incomplete_session)["active_run"].get<std::string>()];
    check(incomplete_run["layout_source"] == "authored_fallback" &&
              incomplete_run["generation_diagnostics"].size() == 1,
          "incomplete candidate spawn bindings did not fall back with a diagnostic");
}

void test_secret_and_lowest_floor_boss() {
    auto session = create();
    make_hero(session, "Ranger");
    for (const char* id : {"rowan", "sable", "tala", "iona", "bran", "kora", "eden"}) {
        apply(session, {{"action", "recruit"}, {"id", id}});
    }
    apply(session, {{"action", "create_run"}, {"boss", true}});
    reach_mine(session);
    check(!send(session, {{"action", "begin_main"}}),
          "lowest-floor boss began from the upper floor");
    walk_to(session, {6, 5});
    apply(session, {{"action", "search"}});
    apply(session, {{"action", "begin_secret"}});
    win_battle(session);
    const auto after_secret = snapshot(session);
    const auto adventure = after_secret["runs"][after_secret["active_run"].get<std::string>()];
    check(adventure["changes"]["secret_won"] == true && adventure["changes"]["main_won"] == false,
          "secret battle leaked into required main progress");
    walk_to(session, {6, 0});
    apply(session, {{"action", "descend"}});
    check(snapshot(session)["mine_floor"] == 2, "boss floor did not become active");
    apply(session, {{"action", "begin_main"}});
    win_battle(session);
    walk_to(session, {14, 0});
    apply(session, {{"action", "take_ore"}});
    walk_to(session, {6, 0});
    apply(session, {{"action", "ascend"}});
    check(snapshot(session)["mine_floor"] == 1, "boss party did not return upstairs");
    walk_to(session, {0, 0});
    apply(session, {{"action", "leave_mine"}});
}

void test_levels_one_to_three() {
    auto session = create();
    make_hero(session, "Ranger");
    for (const char* id : {"rowan", "sable", "tala", "iona", "bran", "kora", "eden"}) {
        apply(session, {{"action", "recruit"}, {"id", id}});
    }
    std::vector<std::string> runs;
    for (int index = 0; index < 3; ++index) {
        apply(session, {{"action", "create_run"}, {"mode", "generated"}, {"seed", 30 + index}});
        runs.push_back(snapshot(session)["active_run"].get<std::string>());
    }
    apply(session, {{"action", "select_run"}, {"id", runs[0]}});
    reach_mine(session);
    for (int index = 0; index < 3; ++index) {
        if (index > 0) {
            apply(session,
                  {{"action", "select_run"}, {"id", runs[static_cast<std::size_t>(index)]}});
            apply(session, {{"action", "enter_mine"}});
        }
        if (index < 2) {
            walk_to(session, {7, 0});
            apply(session, {{"action", "begin_main"}});
        } else {
            walk_to(session, {6, 5});
            apply(session, {{"action", "search"}});
            apply(session, {{"action", "begin_secret"}});
        }
        win_battle(session);
        walk_to(session, {0, 0});
        apply(session, {{"action", "leave_mine"}});
    }
    walk_world(session, {{3, 0}, {2, 0}, {1, 0}, {0, 0}});
    apply(session, {{"action", "enter_city"}});
    const std::string hero = snapshot(session)["hero_id"].get<std::string>();
    apply(session, {{"action", "level_up"}, {"id", hero}});
    apply(session, {{"action", "level_up"}, {"id", hero}});
    check(snapshot(session)["party"][0]["level"] == 3, "prototype level three was not reached");
    check(!send(session, {{"action", "level_up"}, {"id", hero}}),
          "prototype allowed a fourth level");
}

void test_quarry_content_bindings() {
    auto session = create_from(quarry_definition());
    make_hero(session);
    for (const char* id : {"rowan", "sable", "tala", "iona", "bran", "kora", "eden"}) {
        apply(session, {{"action", "recruit"}, {"id", id}});
    }
    apply(session, {{"action", "create_run"}, {"mode", "authored"}});
    json state = snapshot(session);
    const std::string run_id = state["active_run"].get<std::string>();
    const auto& run = state["runs"][run_id];
    check(run["content_schema_version"] == 2, "quarry content schema was not resolved");
    check(run["participant"] == "quarry prospectors" && run["location"] == "blue stone quarry" &&
              run["reward"] == "blue stone",
          "quarry defaults were not resolved from content");
    check(run["main_enemies"].size() == 2 && run["secret_enemies"].size() == 2,
          "quarry encounter bindings have the wrong enemy counts");
    check(run["rewards"]["cache_gold"] == 25 && run["rewards"]["ore_quantity"] == 2,
          "quarry reward quantities were not resolved");
    auto shared_retreat = create_from(quarry_definition());
    make_hero(shared_retreat);
    for (const char* id : {"rowan", "sable", "tala", "iona", "bran", "kora", "eden"}) {
        apply(shared_retreat, {{"action", "recruit"}, {"id", id}});
    }
    apply(shared_retreat, {{"action", "create_run"}});
    reach_mine(shared_retreat);
    apply(shared_retreat, {{"action", "begin_main"}});
    apply(shared_retreat, {{"action", "retreat"}});
    const std::string shared_retreat_save = odr_save(shared_retreat.get());
    auto shared_retreat_loaded = create_from(quarry_definition());
    check(odr_load(shared_retreat_loaded.get(), shared_retreat_save.c_str()) != 0,
          "quarry shared encounter spawn retreat save was rejected");
    auto mine = create();
    make_hero(mine);
    apply(mine, {{"action", "create_run"}});
    check(
        run["layout"]["objects"] !=
            snapshot(
                mine)["runs"][snapshot(mine)["active_run"].get<std::string>()]["layout"]["objects"],
        "quarry did not resolve distinct authored bindings");
    const std::string save = odr_save(session.get());
    auto restored = create_from(quarry_definition());
    check(odr_load(restored.get(), save.c_str()) != 0 && snapshot(restored) == state,
          "quarry authored save did not roundtrip");
    apply(session, {{"action", "create_run"}, {"mode", "generated"}, {"seed", 11}});
    const json generated = snapshot(session);
    check(generated["runs"][generated["active_run"].get<std::string>()]["layout_source"] ==
              "generated",
          "quarry generated layout was not accepted");
    apply(session, {{"action", "select_run"}, {"id", run_id}});
    reach_mine(session);
    walk_to(session, {5, 5});
    apply(session, {{"action", "search"}});
    apply(session, {{"action", "take_hidden_loot"}});
    check(snapshot(session)["inventory"]["gold"] == 55, "quarry cache reward was not resolved");
    walk_to(session, {7, 0});
    apply(session, {{"action", "begin_main"}});
    win_battle(session);
    walk_to(session, {14, 1});
    apply(session, {{"action", "take_ore"}});
    check(snapshot(session)["inventory"]["ore"] == 2,
          "quarry ore reward quantity was not resolved");
    const std::string progressed_save = odr_save(session.get());
    json updated_definition = json::parse(quarry_definition());
    updated_definition["defaults"]["reward"] = "changed future reward";
    updated_definition["encounters"]["main"][0]["name"] = "Changed Future Sentinel";
    auto updated = create_from(updated_definition.dump());
    check(odr_load(updated.get(), progressed_save.c_str()) != 0 &&
              snapshot(updated) == snapshot(session),
          "quarry save did not load under updated content definition");
    json bad = state;
    bad["runs"][run_id]["main_enemies"][0]["pos"] = {99, 99};
    check(odr_load(restored.get(), bad.dump().c_str()) == 0,
          "quarry save with invalid encounter placement was accepted");
    json malformed = json::parse(quarry_definition());
    malformed["encounters"]["main"][0]["hp"] = 0;
    check(odr_create(malformed.dump().c_str()) == nullptr,
          "malformed quarry enemy stats were accepted");
    malformed = json::parse(quarry_definition());
    malformed["encounters"]["secret"][0]["suffix"] = malformed["encounters"]["main"][0]["suffix"];
    check(odr_create(malformed.dump().c_str()) == nullptr,
          "duplicate quarry encounter IDs were accepted");
    malformed = json::parse(quarry_definition());
    malformed["mine"].erase("ore");
    check(odr_create(malformed.dump().c_str()) == nullptr,
          "quarry definition without ore objective was accepted");
    malformed = json::parse(quarry_definition());
    malformed["recruits"][0].erase("class");
    check(odr_create(malformed.dump().c_str()) == nullptr,
          "quarry definition with malformed recruit was accepted");
}

} // namespace

int main() {
    try {
        test_identity_validation_and_save();
        test_world_and_retreat();
        test_run_progress_isolation();
        test_recruitment_and_progression();
        test_battle_deployment_follows_entry();
        test_complete_adventure(8);
        test_complete_adventure(12);
        test_complete_adventure(8, true);
        test_generated_fallback();
        test_secret_and_lowest_floor_boss();
        test_levels_one_to_three();
        test_quarry_content_bindings();
        std::cout << "session scenarios passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "session scenarios failed: " << error.what() << '\n';
        return 1;
    }
}
