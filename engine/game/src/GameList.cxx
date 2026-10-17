#include "vibe/game/GameList.hxx"

#include <memory>
#include <utility>

#include <json/json.h>

namespace vibe::game {

namespace {
std::string text(const Json::Value& v, const char* key) {
    return v.isMember(key) && v[key].isString() ? v[key].asString() : std::string{};
}
} // namespace

bool loadGameList(std::string_view json, std::vector<GameEntry>& out, std::string& error) {
    out.clear();
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader{builder.newCharReader()};
    Json::Value root;
    std::string parseErrors;
    if(!reader->parse(json.data(), json.data() + json.size(), &root, &parseErrors)) {
        error = "games.json: " + parseErrors;
        return false;
    }
    if(!root.isArray()) {
        error = "games.json: expected an array of games";
        return false;
    }

    for(Json::ArrayIndex i = 0; i < root.size(); ++i) {
        const Json::Value& v = root[i];
        const auto fail = [&](const std::string& message) {
            error = "entry " + std::to_string(i) + ": " + message;
            out.clear();
            return false;
        };
        if(!v.isObject()) return fail("expected an object");

        GameEntry entry;
        entry.name = text(v, "name");
        entry.binary = text(v, "binary");
        if(entry.name.empty()) return fail("missing name");
        if(entry.binary.empty()) return fail("missing binary");
        entry.title = text(v, "title");
        if(entry.title.empty()) entry.title = entry.name;
        entry.players = text(v, "players");
        entry.description = text(v, "description");
        if(v.isMember("args")) {
            if(!v["args"].isArray()) return fail("args must be an array of strings");
            for(const Json::Value& arg : v["args"]) {
                if(!arg.isString()) return fail("args must be an array of strings");
                entry.args.push_back(arg.asString());
            }
        }
        out.push_back(std::move(entry));
    }
    return true;
}

} // namespace vibe::game
