#include "MeshSwapAnimator.hxx"

namespace vibe {

ShMeshSwapAnimatorPr MeshSwapAnimator::create() {
    return std::make_shared<MeshSwapAnimator>();
}

void MeshSwapAnimator::addMesh(Magnum::GL::Mesh* mesh) {
    _meshes.push_back(mesh);
}

void MeshSwapAnimator::update(float dt) {
    if (!_playing || _meshes.size() < 2) return;

    _accumulator += dt * _frameRate * _speedMultiplier;
    while (_accumulator >= 1.0f) {
        _accumulator -= 1.0f;
        advanceFrame();
    }
}

Magnum::GL::Mesh* MeshSwapAnimator::currentMesh() const {
    if (_meshes.empty()) return nullptr;
    return _meshes[_currentFrame];
}

void MeshSwapAnimator::triggerNextFrame() {
    if (_meshes.size() < 2) return;
    advanceFrame();
    _accumulator = 0.0f;
}

void MeshSwapAnimator::play() { _playing = true; }
void MeshSwapAnimator::pause() { _playing = false; }

void MeshSwapAnimator::stop() {
    _playing = false;
    _currentFrame = 0;
    _accumulator = 0.0f;
    _direction = 1;
}

void MeshSwapAnimator::setFrameRate(float fps) { _frameRate = fps; }
void MeshSwapAnimator::setSpeedMultiplier(float mult) { _speedMultiplier = mult; }
void MeshSwapAnimator::setPlayMode(PlayMode mode) { _mode = mode; }

void MeshSwapAnimator::advanceFrame() {
    int n = static_cast<int>(_meshes.size());
    if (n < 2) return;

    switch (_mode) {
    case Loop:
        _currentFrame = (_currentFrame + 1) % n;
        break;
    case Reverse:
        _currentFrame = (_currentFrame - 1 + n) % n;
        break;
    case PingPong:
        _currentFrame += _direction;
        if (_currentFrame >= n) {
            _currentFrame = n - 2;
            _direction = -1;
        } else if (_currentFrame < 0) {
            _currentFrame = 1;
            _direction = 1;
        }
        break;
    case Once:
        if (_currentFrame < n - 1) {
            _currentFrame++;
        } else {
            _playing = false;
        }
        break;
    }
}

} // namespace vibe
