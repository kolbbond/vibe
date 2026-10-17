#pragma once

#include <memory>
#include <vector>

#include <Magnum/GL/Mesh.h>

namespace vibe {

typedef std::shared_ptr<class MeshSwapAnimator> ShMeshSwapAnimatorPr;

class MeshSwapAnimator {
public:
    enum PlayMode { Loop, PingPong, Once, Reverse };

    MeshSwapAnimator() = default;
    static ShMeshSwapAnimatorPr create();

    void addMesh(Magnum::GL::Mesh* mesh);
    void update(float dt);
    Magnum::GL::Mesh* currentMesh() const;
    void triggerNextFrame();

    void play();
    void pause();
    void stop();

    void setFrameRate(float fps);
    void setSpeedMultiplier(float mult);
    void setPlayMode(PlayMode mode);

    float frameRate() const { return _frameRate; }
    float speedMultiplier() const { return _speedMultiplier; }
    PlayMode playMode() const { return _mode; }
    bool isPlaying() const { return _playing; }
    int currentFrameIndex() const { return _currentFrame; }
    int frameCount() const { return static_cast<int>(_meshes.size()); }

private:
    std::vector<Magnum::GL::Mesh*> _meshes;
    float _frameRate = 12.0f;
    float _speedMultiplier = 1.0f;
    float _accumulator = 0.0f;
    int _currentFrame = 0;
    int _direction = 1;
    bool _playing = false;
    PlayMode _mode = Loop;

    void advanceFrame();
};

} // namespace vibe
