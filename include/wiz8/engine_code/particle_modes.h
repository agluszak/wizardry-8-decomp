#pragma once

/* Runtime particle selectors are four-byte domains. File emission/speed
   selectors use the same values; ReadWorldParticles normalizes other values. */
enum W8ParticleBoundsMode {
    W8_PARTICLE_BOUNDS_NONE = 0,
    W8_PARTICLE_BOUNDS_BOX = 1,
    W8_PARTICLE_BOUNDS_SPHERE = 2
};
enum W8ParticleEmissionMode {
    W8_PARTICLE_EMISSION_NONE = 0,
    W8_PARTICLE_EMISSION_SINGLE = 1,
    W8_PARTICLE_EMISSION_CATCH_UP = 2
};
enum W8ParticleDirectionMode {
    W8_PARTICLE_DIRECTION_STATIONARY = 0,
    W8_PARTICLE_DIRECTION_FIXED = 1,
    W8_PARTICLE_DIRECTION_NODE_FORWARD = 2,
    W8_PARTICLE_DIRECTION_CONE = 3,
    W8_PARTICLE_DIRECTION_RANDOM = 4
};
enum W8ParticleSpeedMode {
    W8_PARTICLE_SPEED_ZERO = 0,
    W8_PARTICLE_SPEED_FIXED = 1,
    W8_PARTICLE_SPEED_RANDOM = 2
};
enum W8ParticleFlutterMode {
    W8_PARTICLE_FLUTTER_NONE = 0,
    W8_PARTICLE_FLUTTER_VELOCITY_SCALED = 2
};
/* Zero expires by birth tick/lifetime; one follows texture playback and
   falls back to timed expiry if no texture-frame bank exists. */
enum W8ParticleExpiryMode { W8_PARTICLE_EXPIRY_TIMED = 0, W8_PARTICLE_EXPIRY_TEXTURE = 1 };
enum W8ParticleUpdateFlags { W8_PARTICLE_ACTIVE_TRIANGLES_DIRTY = 2 };
static_assert(sizeof(W8ParticleExpiryMode) == 4, "W8ParticleExpiryMode_size");
static_assert(sizeof(W8ParticleBoundsMode) == 4, "W8ParticleBoundsMode_size");
static_assert(sizeof(W8ParticleEmissionMode) == 4, "W8ParticleEmissionMode_size");
static_assert(sizeof(W8ParticleDirectionMode) == 4, "W8ParticleDirectionMode_size");
static_assert(sizeof(W8ParticleSpeedMode) == 4, "W8ParticleSpeedMode_size");
static_assert(sizeof(W8ParticleFlutterMode) == 4, "W8ParticleFlutterMode_size");
