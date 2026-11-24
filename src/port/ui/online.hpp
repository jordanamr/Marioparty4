#pragma once

#include "window.hpp"

#include <string>
#if defined(__linux__) && !defined(__ANDROID__)
#include "port/settings.h"
#include <chrono>
#include <filesystem>
struct SDL_Process;
#endif

// The "Play Online" window on platforms that cannot start PartyBoardOnline.exe
// (Android, iOS, Linux, macOS). It carries the same lobby as the Windows
// companion's LobbyForm -- nickname, disc, player count, create, join with a
// PB4 invitation, players and status -- inside the game itself, since a phone
// cannot launch a second process next to it.
namespace partyboard::ui {

// Android: the lobby is the app's own screen in a separate process
// (platforms/android, online/LobbyActivity.java), because the game has to be
// restarted with its --netplay arguments exactly like partyboard.exe is by the
// Windows companion. Returns false if the screen could not be opened; on
// success the caller ends the game, as the Windows "Play Online" entry does.
// With an invitation (a CubeShelf friend's), the lobby joins it by itself.
bool open_android_lobby(const std::string &invitation = {});

class OnlineWindow : public Window {
public:
    // With an invitation -- one a CubeShelf friend sent -- the window opens on joining it.
    explicit OnlineWindow(std::string invitation = {});
#if defined(__linux__) && !defined(__ANDROID__)
    ~OnlineWindow() override;
    void update() override;
#endif

private:
    void build(Rml::Element *content);
    void set_status(std::string text);
    void create_lobby();
    void join_lobby();
    void paste_invitation();
    void leave_lobby();

#if defined(__linux__) && !defined(__ANDROID__)
    void open_linux_room(bool host);
    bool send_room_request(const char *action, const char *value = nullptr);
    bool consume_close_request() override;
    void reset_linux_room();
    GameLanguage mHostLanguage = getSettings().game.language.getValue();
    SDL_Process *mCompanion = nullptr;
    std::filesystem::path mRoomDirectory;
    std::chrono::steady_clock::time_point mNextPoll{};
    std::string mHostInvitation;
    bool mCanStart = false;
    bool mRefreshRoom = false;
    bool mCloseRequested = false;
    bool mPopWhenClosed = false;
#endif
    int mPlayers = 2;
    std::string mInvitation;
    std::string mStatus;
    Rml::Element *mStatusElem = nullptr;
};

} // namespace partyboard::ui
