#if defined(__linux__) && !defined(__ANDROID__)
#include "online.hpp"
#include "localization.hpp"
#include "prelaunch.hpp"
#include "port/config.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_process.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <unistd.h>

namespace partyboard::ui {

OnlineWindow::~OnlineWindow()
{
    if (mCompanion) {
        // Closing the application can skip the normal Leave/acknowledgement path.
        send_room_request("close", "launcher_shutdown");
        SDL_DestroyProcess(mCompanion);
        // The companion watches the parent and still needs its request file.
        return;
    }
    reset_linux_room();
}

void OnlineWindow::reset_linux_room()
{
    if (mCompanion) {
        SDL_DestroyProcess(mCompanion);
        mCompanion = nullptr;
    }
    if (!mRoomDirectory.empty()) {
        std::error_code ec;
        std::filesystem::remove_all(mRoomDirectory, ec);
        mRoomDirectory.clear();
    }
    mHostInvitation.clear();
    mCanStart = false;
    mCloseRequested = false;
    mRefreshRoom = true;
}

void OnlineWindow::open_linux_room(bool host)
{
    if (mCompanion) {
        return;
    }
    std::error_code ec;
    auto temp = std::filesystem::temp_directory_path(ec);
    auto folder = (temp / "partyboard-lobby-XXXXXX").string();
    if (ec || !mkdtemp(folder.data())) {
        set_status("Could not create the lobby directory.");
        return;
    }
    mRoomDirectory = folder;
    std::filesystem::create_directory(mRoomDirectory / "requests", ec);
    const char *base = SDL_GetBasePath();
    const std::string companion = std::string(base ? base : "") + "PartyBoardOnline";
    const char *args[] = { companion.c_str(), host ? "--host" : "--join", "--game-language",
        game_language_code(host ? mHostLanguage : getSettings().game.language.getValue()), nullptr };
    auto *env = SDL_CreateEnvironment(true);
    const auto props = SDL_CreateProperties();
    const auto &name = getSettings().online.nickname.getValue();
    if (!ec && base && env && props
        && (SDL_GetEnvironmentVariable(env, "PARTYBOARD_ONLINE_UI_LANGUAGE")
            || SDL_SetEnvironmentVariable(env, "PARTYBOARD_ONLINE_UI_LANGUAGE", game_language_code(getSettings().game.language.getValue()), true))
        && SDL_SetEnvironmentVariable(env, "PARTYBOARD_ONLINE_DISC", prelaunch_state().configuredDiscPath.c_str(), true)
        && SDL_SetEnvironmentVariable(env, "PARTYBOARD_ONLINE_NICKNAME", name.empty() ? ui_translate("Player").c_str() : name.c_str(), true)
        && SDL_SetEnvironmentVariable(env, "PARTYBOARD_ONLINE_INVITE", host ? "" : mInvitation.c_str(), true)
        && SDL_SetEnvironmentVariable(env, "PARTYBOARD_ONLINE_PLAYERS", std::to_string(mPlayers).c_str(), true)
        && SDL_SetEnvironmentVariable(env, "PARTYBOARD_ONLINE_STATE_DIR", folder.c_str(), true)
        && SDL_SetEnvironmentVariable(env, "PARTYBOARD_ONLINE_PARENT_PID", std::to_string(getpid()).c_str(), true)
        && SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, args)
        && SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER, env)) {
        mCompanion = SDL_CreateProcessWithProperties(props);
    }
    SDL_DestroyProperties(props);
    SDL_DestroyEnvironment(env);
    if (!mCompanion) {
        reset_linux_room();
        set_status("Could not start PartyBoardOnline. Check the game installation.");
        return;
    }
    mRefreshRoom = true;
    set_status("Verifying disc and opening lobby…");
}

bool OnlineWindow::send_room_request(const char *action, const char *value)
{
    // Only one pending start/close of each kind is needed. Publish it atomically.
    const auto path = mRoomDirectory / "requests" / action;
    auto temporary = path;
    temporary += ".tmp";
    std::ofstream file(temporary);
    nlohmann::json request{{"id", action}, {"action", action}};
    if (value) request["value"] = value;
    file << request;
    file.close();
    std::error_code ec;
    if (file) {
        auto target = path;
        target += ".json";
        std::filesystem::rename(temporary, target, ec);
        if (!ec) {
            return true;
        }
    }
    set_status("Could not send the lobby command.");
    return false;
}

bool OnlineWindow::consume_close_request()
{
    if (!mCompanion) {
        return false;
    }
    mPopWhenClosed = true;
    leave_lobby();
    return true;
}

void OnlineWindow::update()
{
    if (top_document() == this && !mCompanion) {
        try_push_verification_modal(*this);
    }
    Window::update();
    const auto now = std::chrono::steady_clock::now();
    if (mCompanion && now >= mNextPoll) {
        mNextPoll = now + std::chrono::milliseconds(250);
        int exitCode = 0;
        const bool exited = SDL_WaitProcess(mCompanion, false, &exitCode);
        std::ifstream file(mRoomDirectory / "room.json");
        const auto room = file ? nlohmann::json::parse(file, nullptr, false) : nlohmann::json();
        bool gotFinalStatus = false;
        if (room.is_object()) {
            const auto invite = room.value("invitation", nlohmann::json());
            const auto invitation = invite.is_string() ? invite.get<std::string>() : std::string();
            const bool canStart = room.value("canStart", false);
            mRefreshRoom |= mCanStart != canStart || mHostInvitation.empty() != invitation.empty();
            mCanStart = canStart;
            mHostInvitation = invitation;
            const auto phase = room.value("state", std::string());
            gotFinalStatus = phase == "error" || phase == "closed";
            auto message = room.value("message", std::string());
            const auto players = room.find("players");
            if (players != room.end() && players->is_array() && !players->empty() && !gotFinalStatus) {
                message += "  (" + ui_translate("Players") + " — ";
                bool first = true;
                for (const auto &player : *players) {
                    if (player.is_string()) {
                        if (!first) message += ", ";
                        message += player.get<std::string>();
                        first = false;
                    }
                }
                message += ")";
            }
            if (phase == "closed") {
                message = ui_translate("Lobby closed.");
            }
            if (!message.empty() && message != mStatus && (!mCloseRequested || gotFinalStatus)) {
                set_status(message);
            }
        }
        if (exited) {
            if (!gotFinalStatus) {
                set_status(exitCode ? "The online lobby stopped unexpectedly." : "Lobby closed.");
            }
            reset_linux_room();
            if (mPopWhenClosed) {
                pop();
                return;
            }
        }
    }
    if (mRefreshRoom) {
        mRefreshRoom = false;
        refresh_active_tab();
    }
}
} // namespace partyboard::ui
#endif
