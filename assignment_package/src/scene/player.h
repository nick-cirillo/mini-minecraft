#pragma once
#include "entity.h"
#include "camera.h"
#include "terrain.h"

class Player : public Entity {
private:
    glm::vec3 m_velocity, m_acceleration;
    Camera m_camera;
    const Terrain &mcr_terrain;

    bool flightMode;
    bool flightModeSet;

    bool jumping;
    bool swimming;

    // XNEG, XPOS, YNEG, YPOS, ZNEG, ZPOS
    std::array<bool, 6> huggingWall;

    void processInputs(InputBundle &inputs);
    void computePhysics(float dT, const Terrain &terrain);

    void handleCollision(const Terrain& terrain);
    bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain& terrain,
                   bool huggingWall, float* out_dist, glm::ivec3* out_BlockHit);

    std::array<glm::vec3, 12> getCollisionVertices();

public:
    // Readonly public reference to our camera
    // for easy access from MyGL
    const Camera& mcr_camera;

    Player(glm::vec3 pos, const Terrain &terrain);
    virtual ~Player() override;

    void setCameraWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &input) override;

    // Player overrides all of Entity's movement
    // functions so that it transforms its camera
    // by the same amount as it transforms itself.
    void moveAlongVector(glm::vec3 dir) override;
    void moveForwardLocal(float amount) override;
    void moveRightLocal(float amount) override;
    void moveUpLocal(float amount) override;
    void moveForwardGlobal(float amount) override;
    void moveRightGlobal(float amount) override;
    void moveUpGlobal(float amount) override;
    void rotateOnForwardLocal(float degrees) override;
    void rotateOnRightLocal(float degrees) override;
    void rotateOnUpLocal(float degrees) override;
    void rotateOnForwardGlobal(float degrees) override;
    void rotateOnRightGlobal(float degrees) override;
    void rotateOnUpGlobal(float degrees) override;

    // For sending the Player's data to the GUI
    // for display
    QString posAsQString() const;
    QString velAsQString() const;
    QString accAsQString() const;
    QString lookAsQString() const;

    // For getting and setting the player's flight mode
    bool getFlightMode() const;
    void setFlightMode(bool flightMode);
    bool getFlightModeSet() const;
    void setFlightModeSet(bool modified);

    // For getting and setting player jumping
    bool isJumping() const;
    void setJumping(bool jumping);
    bool isSwimming() const;

    // For adding and removing blocks
    void addBlock(Terrain& terrain);
    void removeBlock(Terrain& terrain);

    int medium = 0;
};
