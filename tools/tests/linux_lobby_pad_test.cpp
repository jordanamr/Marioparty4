// Run under X11/Xvfb with tools/test_linux_lobby_pad.py.
#include "src/port/ui/button.hpp"
#include "src/port/ui/input.hpp"

#include <SDL3/SDL.h>
#include <RmlUi/Core.h>
#include <aurora/rmlui.hpp>
#include <X11/Xlib.h>
#include <cassert>
#include <cstdio>

namespace {
Rml::Context *context;
SDL_Window *launcher;
SDL_Joystick *joystick;
int padEvents;
int closeRequests;
int directions;

struct Renderer : Rml::RenderInterface {
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override { return 1; }
    void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
    void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
    Rml::TextureHandle LoadTexture(Rml::Vector2i &, const Rml::String &) override { return 0; }
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return 0; }
    void ReleaseTexture(Rml::TextureHandle) override {}
    void EnableScissorRegion(bool) override {}
    void SetScissorRegion(Rml::Rectanglei) override {}
};

void pump()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) ++padEvents;
        partyboard::ui::input::handle_event(event);
    }
    partyboard::ui::input::update_input();
}

void focus(Display *display, Window window, bool inLauncher)
{
    XSetInputFocus(display, window, RevertToParent, CurrentTime);
    XSync(display, False);
    for (int i = 0; i < 100; ++i) {
        pump();
        if ((SDL_GetKeyboardFocus() == launcher) == inLauncher) return;
        SDL_Delay(5);
    }
    assert(false && "X11 focus did not reach the requested window");
}

void confirm()
{
    int previous = padEvents;
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    pump();
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false));
    pump();
    assert(padEvents > previous && "SDL must deliver a real gamepad event");
}
}

namespace aurora::rmlui {
Rml::Context *get_context() noexcept { return context; }
}

int main(int argc, char **argv)
{
    using partyboard::ui::input::LobbyPadOwner;
    using partyboard::ui::input::set_lobby_pad_owner;
    assert(SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1"));
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
    launcher = SDL_CreateWindow("Lobby", 200, 100, 0);
    assert(launcher);
    auto *display = XOpenDisplay(nullptr);
    assert(display);
    auto game = XCreateSimpleWindow(display, DefaultRootWindow(display), 210, 0, 200, 100, 0, 0, 0);
    XMapWindow(display, game);
    auto lobby = static_cast<Window>(SDL_GetNumberProperty(SDL_GetWindowProperties(launcher), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    assert(lobby);
    focus(display, lobby, true);

    Renderer renderer;
    Rml::SetRenderInterface(&renderer);
    assert(Rml::Initialise());
    assert(argc == 2 && Rml::LoadFontFace(argv[1]));
    context = Rml::CreateContext("lobby", {200, 100});
    assert(context);
    auto *doc = context->LoadDocumentFromMemory("<rml><head></head><body></body></rml>");
    assert(doc);
    doc->SetProperty("font-family", "Inter");
    doc->Show();
    {
        partyboard::ui::Button leave(doc, Rml::String());
        leave.root()->SetInnerRML("Leave lobby");
        leave.root()->SetProperty("display", "block");
        leave.root()->SetProperty("width", "160px");
        leave.root()->SetProperty("height", "48px");
        leave.on_pressed([] { ++closeRequests; });
        leave.on_nav_command([](Rml::Event &, partyboard::ui::NavCommand cmd) {
            if (cmd == partyboard::ui::NavCommand::Right) ++directions;
            return false;
        });
        leave.root()->SetProperty("tab-index", "auto");
        context->Update();
        assert(leave.root()->Focus());
        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
        desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
        auto id = SDL_AttachVirtualJoystick(&desc);
        assert(id);
        auto *pad = SDL_OpenGamepad(id);
        assert(pad);
        joystick = SDL_GetGamepadJoystick(pad);
        set_lobby_pad_owner(LobbyPadOwner::ForegroundLobby);
        confirm();
        assert(closeRequests == 1);

        assert(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 28000));
        pump();
        assert(directions == 1);
        set_lobby_pad_owner(LobbyPadOwner::OnlineGame);
        SDL_Delay(400);
        pump();
        assert(directions == 1 && "a held direction must not repeat after handing the pad to the game");
        confirm();
        assert(closeRequests == 1 && "the foreground launcher accepted the game's button");
        focus(display, game, false);
        confirm();
        assert(closeRequests == 1 && "the background launcher accepted the game's button");
        set_lobby_pad_owner(LobbyPadOwner::ForegroundLobby);
        confirm();
        assert(closeRequests == 1 && "an unfocused lobby accepted a button");
        focus(display, lobby, true);
        set_lobby_pad_owner(LobbyPadOwner::OnlineGame);
        context->Update();
        const auto origin = leave.root()->GetAbsoluteOffset();
        const auto center = origin + Rml::Vector2f(leave.root()->GetClientWidth() / 2, leave.root()->GetClientHeight() / 2);
        assert(context->GetElementAtPoint(center));
        context->ProcessMouseMove(static_cast<int>(center.x), static_cast<int>(center.y), 0);
        context->ProcessMouseButtonDown(0, 0);
        context->ProcessMouseButtonUp(0, 0);
        assert(closeRequests == 2 && "the deliberate Leave lobby click did not work");
        set_lobby_pad_owner(LobbyPadOwner::Launcher);
        assert(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 0));
        pump();
        confirm();
        assert(closeRequests == 3 && "the launcher did not recover controller navigation");
        SDL_CloseGamepad(pad);
        SDL_DetachVirtualJoystick(id);
    }
    Rml::RemoveContext("lobby");
    Rml::Shutdown();
    XDestroyWindow(display, game);
    XCloseDisplay(display);
    SDL_DestroyWindow(launcher);
    SDL_Quit();
    std::puts("PASS: SDL virtual pad, background/active game, held axis, manual Leave and restored launcher.");
}
