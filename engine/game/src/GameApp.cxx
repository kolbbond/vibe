#include "vibe/game/GameApp.hxx"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>

#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Image.h>
#include <Magnum/PixelFormat.h>

#include <Magnum/Math/Color.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

namespace vibe::game {

namespace {
const Color4 kGutterColor = 0x2a2f3aff_rgbaf;
const Color4 kViewColor = 0x0b0d12ff_rgbaf;
const Color4 kWhite = 0xffffffff_rgbaf;
const Color4 kGrey = 0x9aa0aaff_rgbaf;
} // namespace

AppOptions parseAppOptions(int argc, char** argv) {
    AppOptions options;
    for(int i = 1; i < argc; ++i) {
        const bool hasValue = i + 1 < argc;
        if(std::strcmp(argv[i], "--screenshot") == 0 && hasValue)
            options.screenshotPath = argv[++i];
        else if(std::strcmp(argv[i], "--frames") == 0 && hasValue)
            options.screenshotFrames = std::max(1, std::atoi(argv[++i]));
        else if(std::strcmp(argv[i], "--script") == 0 && hasValue)
            options.scriptPath = argv[++i];
        else if(std::strcmp(argv[i], "--mute") == 0)
            options.mute = true;
    }
    return options;
}

GameApp::GameApp(const Arguments& arguments, GameConfig config, std::unique_ptr<Game> game, AppOptions options)
    : smg::GuiBase{arguments},
      _config{std::move(config)},
      _options{std::move(options)},
      _game{std::move(game)},
      _input{_actions, _config.maxPlayers},
      _lobby{_input, _config.minPlayers},
      _sdl{_actions},
      _stepper{_config.tickHz} {
    _game->declareActions(_actions);
    setWindowTitle(_config.title.c_str());
    GL::Renderer::setClearColor(kGutterColor);
    // game keys must not drive ImGui navigation
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    if(!_options.scriptPath.empty()) loadScript();
    // no music for automated screenshot runs or with --mute
    if(!_config.music.empty() && !_options.mute && _options.screenshotPath.empty())
        _music.play(std::string{VIBE_ASSETS_DIR} + "/" + _config.music, _config.musicVolume);
}

void GameApp::loadScript() {
    std::ifstream file{_options.scriptPath};
    if(!file) {
        Corrade::Utility::Error{} << "script: cannot open" << _options.scriptPath.c_str();
        exit(1);
        return;
    }
    std::stringstream text;
    text << file.rdbuf();
    std::string error;
    if(!parseInputScript(text.str(), _script, error)) {
        Corrade::Utility::Error{} << "script:" << error.c_str();
        exit(1);
    }
}

void GameApp::anyEvent(SDL_Event& event) { _sdl.handleEvent(event, _input); }

bool GameApp::anyPlayerPressed(ButtonId b) const {
    return std::any_of(_players.begin(), _players.end(), [b](const Player& p) { return p.pressed(b); });
}

void GameApp::startMatch() {
    _players.clear();
    for(const int slot : _lobby.joinedSlots()) _players.emplace_back(_input, _lobby, slot);
    _game->start(_players);
    _stepper.reset();
    _state = State::Playing;
}

void GameApp::updateLobby() {
    _input.beginTick();
    if(_lobby.update()) startMatch();
}

void GameApp::updatePlaying(double frameSeconds) {
    // deliberately no pause on focus loss: friends play on the TV while the developer
    // works on another monitor; pads keep working through SdlInput's background hint
    const int ticks = _stepper.advance(frameSeconds, 1.0);
    const double dt = 1.0 / _config.tickHz;
    for(int t = 0; t < ticks; ++t) {
        _input.beginTick();
        _lobby.updateInMatch();
        if(_lobby.anyDisconnected() || anyPlayerPressed(ActionMap::Start)) {
            _state = State::Paused;
            return;
        }
        if(_game->tick(dt, _players) == Status::Finished) {
            _state = State::Finished;
            return;
        }
    }
}

void GameApp::updatePaused() {
    _input.beginTick();
    _lobby.updateInMatch();
    if(anyPlayerPressed(ActionMap::Back)) {
        _lobby.returnToLobby();
        _state = State::Lobby;
    } else if(!_lobby.anyDisconnected() && (anyPlayerPressed(ActionMap::Confirm) || anyPlayerPressed(ActionMap::Start))) {
        _stepper.reset();
        _state = State::Playing;
    }
}

void GameApp::updateFinished() {
    _input.beginTick();
    _lobby.updateInMatch();
    if(anyPlayerPressed(ActionMap::Back)) {
        _lobby.returnToLobby();
        _state = State::Lobby;
    } else if(anyPlayerPressed(ActionMap::Confirm)) {
        _game->start(_players);
        _stepper.reset();
        _state = State::Playing;
    }
}

View GameApp::fullView() const {
    const Vector2i fb = framebufferSize();
    return View{Range2Di{{}, fb}, -1, fb.y() > 0 ? float(fb.x()) / float(fb.y()) : 1.0f};
}

void GameApp::drawViews(double alpha) {
    const Vector2i fb = framebufferSize();
    _views = layoutViews(fb, static_cast<int>(_players.size()), _config.layout, _config.gutterPx);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::setClearColor(kViewColor);
    for(const View& view : _views) {
        GL::defaultFramebuffer.setViewport(view.viewport);
        GL::Renderer::setScissor(view.viewport);
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
        _game->drawView(view, alpha);
    }
    // smg's ImGui pass assumes the full viewport and no scissor
    GL::Renderer::setClearColor(kGutterColor);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::defaultFramebuffer.setViewport({{}, fb});
}

void GameApp::drawLobby(const View& full) {
    const Vector2 size = _hud.viewSize(full);
    _hud.textCentered(full, {size.x() * 0.5f, size.y() * 0.15f}, kWhite, _config.title, 3.0f);

    const int n = _lobby.maxPlayers();
    const float gap = 24.0f;
    const float panelW = (size.x() - gap * float(n + 1)) / float(n);
    const float panelH = size.y() * 0.4f;
    for(int i = 0; i < n; ++i) {
        const PlayerSlot& slot = _lobby.slot(i);
        const Color4 color = Lobby::slotColor(i);
        const Vector2 min{gap + float(i) * (panelW + gap), size.y() * 0.3f};
        _hud.panel(full, min, {panelW, panelH}, slot.joined ? Color4{color.rgb() * 0.35f, 1.0f} : 0x1a1d24ff_rgbaf);
        const Vector2 center = min + Vector2{panelW, panelH} * 0.5f;
        _hud.textCentered(full, center - Vector2{0.0f, 24.0f}, color, "P" + std::to_string(i + 1), 2.0f);
        _hud.textCentered(full, center + Vector2{0.0f, 24.0f}, slot.joined ? kWhite : kGrey,
            slot.joined ? sourceName(slot.source) : "press confirm to join");
    }

    const int missing = _lobby.minPlayers() - _lobby.joinedCount();
    _hud.textCentered(full, {size.x() * 0.5f, size.y() * 0.8f}, kWhite,
        missing > 0 ? "waiting for " + std::to_string(missing) + " more player(s)" : "press start to begin", 1.5f);
    _hud.textCentered(full, {size.x() * 0.5f, size.y() * 0.9f}, kGrey,
        "confirm: Space / Enter / A     back: Q / Backspace / B     start: Esc / Start");
}

void GameApp::drawOverlay() {
    const View full = fullView();
    const Vector2 size = _hud.viewSize(full);
    switch(_state) {
    case State::Lobby: drawLobby(full); return;
    case State::Playing:
        for(const View& view : _views) _game->drawHud(_hud, view);
        return;
    case State::Paused:
        for(const View& view : _views) _game->drawHud(_hud, view);
        _hud.panel(full, {0.0f, 0.0f}, size, 0x000000aa_rgbaf);
        _hud.textCentered(full, size * 0.5f - Vector2{0.0f, 40.0f}, kWhite, "paused", 3.0f);
        _hud.textCentered(full, size * 0.5f + Vector2{0.0f, 20.0f}, kGrey,
            _lobby.anyDisconnected() ? "a controller disconnected: press confirm on a free controller or keyboard half"
                                     : "confirm: resume     back: lobby");
        return;
    case State::Finished:
        for(const View& view : _views) _game->drawHud(_hud, view);
        _hud.textCentered(full, {size.x() * 0.5f, size.y() * 0.9f}, kWhite, "confirm: rematch     back: lobby", 1.5f);
        return;
    }
}

void GameApp::writeScreenshot() {
    const Range2Di vp{{}, framebufferSize()};
    Image2D image = GL::defaultFramebuffer.read(vp, {PixelFormat::RGBA8Unorm});
    const int w = image.size().x();
    const int h = image.size().y();
    std::FILE* out = std::fopen(_options.screenshotPath.c_str(), "wb");
    if(!out) {
        Corrade::Utility::Error{} << "screenshot: cannot open" << _options.screenshotPath.c_str();
        return;
    }
    std::fprintf(out, "P6\n%d %d\n255\n", w, h);
    const auto pixels = image.pixels<Color4ub>();
    std::vector<unsigned char> row(static_cast<std::size_t>(w) * 3);
    for(int y = h - 1; y >= 0; --y) { // GL rows are bottom-up
        for(int x = 0; x < w; ++x) {
            const Color4ub c = pixels[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            row[static_cast<std::size_t>(x) * 3 + 0] = c.r();
            row[static_cast<std::size_t>(x) * 3 + 1] = c.g();
            row[static_cast<std::size_t>(x) * 3 + 2] = c.b();
        }
        std::fwrite(row.data(), 1, row.size(), out);
    }
    std::fclose(out);
    Corrade::Utility::Debug{} << "screenshot: wrote" << _options.screenshotPath.c_str() << w << "x" << h;
}

void GameApp::drawEvent() {
    const auto now = std::chrono::steady_clock::now();
    const double frameSeconds = _firstFrame ? 0.0 : std::chrono::duration<double>(now - _lastFrame).count();
    _lastFrame = now;
    _firstFrame = false;

    drawBegin();
    // HUD draws through ImGui, whose coordinates are window size / DPI scaling (smg sets DisplaySize)
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    _hud.setFrame(framebufferSize(), Vector2{display.x, display.y});

    while(_scriptNext < _script.size() && _script[_scriptNext].frame <= _frame) _input.handle(_script[_scriptNext++].event);
    _sdl.poll(_input);

    switch(_state) {
    case State::Lobby: updateLobby(); break;
    case State::Playing: updatePlaying(frameSeconds); break;
    case State::Paused: updatePaused(); break;
    case State::Finished: updateFinished(); break;
    }

    if(_state != State::Lobby) drawViews(_stepper.alpha());

    drawOverlay();

    if(!_options.screenshotPath.empty() && _frame == _options.screenshotFrames) {
        // smg's drawEnd() ImGui pass, but read back before the swap so the HUD is in the image
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
        _imgui.drawFrame();
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
        GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
        GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        writeScreenshot();
        exit();
        swapBuffers();
        return;
    }

    ++_frame;
    drawEnd();
}

} // namespace vibe::game
