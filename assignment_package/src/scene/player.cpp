#include "player.h"
#include <QString>
#include <iostream>
#include <ostream>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      flightMode(true), flightModeSet(false),
      huggingWall{{false, false, false, false, false, false}},
      mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    m_acceleration = glm::vec3(0.f, 0.f, 0.f);

    if (flightMode) {
        if (inputs.wPressed && !inputs.sPressed) {
            m_acceleration += m_forward;
        } else if (inputs.sPressed && !inputs.wPressed) {
            m_acceleration -= m_forward;
        }

        if (inputs.dPressed && !inputs.aPressed) {
            m_acceleration += m_right;
        } else if (inputs.aPressed && !inputs.dPressed) {
            m_acceleration -= m_right;
        }

        if (inputs.ePressed && !inputs.qPressed) {
            m_acceleration += m_up;
        } else if (inputs.qPressed && !inputs.ePressed) {
            m_acceleration -= m_up;
        }
    } else {
        glm::vec3 forward = glm::vec3(m_forward.x, 0.f, m_forward.z);
        forward = glm::length(m_forward) * glm::normalize(forward);

        glm::vec3 right = glm::vec3(m_right.x, 0.f, m_right.z);
        right = glm::length(m_right) * glm::normalize(right);

        if (inputs.wPressed && !inputs.sPressed) {
            m_acceleration += forward;
        } else if (inputs.sPressed && !inputs.wPressed) {
            m_acceleration -= forward;
        }

        if (inputs.dPressed && !inputs.aPressed) {
            m_acceleration += right;
        } else if (inputs.aPressed && !inputs.dPressed) {
            m_acceleration -= right;
        }

        m_acceleration.y = -1.f;
    }
}

void Player::computePhysics(float dT, const Terrain &terrain) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.
    m_velocity = 0.8f * m_velocity + dT * m_acceleration;
    handleCollision(terrain);
    moveAlongVector(m_velocity);
}

void Player::handleCollision(const Terrain& terrain) {
    std::array<glm::vec3, 12> vertices = getCollisionVertices();

    // one ray per cardinal direction
    std::array<glm::vec3, 3> rays = {
        glm::vec3(m_velocity.x, 0.f, 0.f),
        glm::vec3(0.f, m_velocity.y, 0.f),
        glm::vec3(0.f, 0.f, m_velocity.z)
    };

    for (int i = 0; i < 3; ++i) {
        int wallIdx = i * 2 + glm::max(0.f, glm::sign(rays[i][i]));
        glm::vec3 offset = glm::vec3(0.f, 0.f, 0.f);
        offset[i] = wallIdx % 2 - 1;

        for (int j = 0; j < 12; ++j) {
            if (!huggingWall[wallIdx]) {
                continue;
            }
            if (terrain.getBlockAt(vertices[j] + offset) != EMPTY) {
                huggingWall[wallIdx] = true;
            }
        }
    }

    // unset huggingWall values
    for (int i = 0; i < 3; ++i) {
        if (rays[i][i] == 0.f) {
            continue;
        } else if (rays[i][i] > 0.f) {
            // if moving in pos. dir, unset neg.
            huggingWall[i * 2] = false;
        } else {
            // if moving in neg. dir, unset pos.
            huggingWall[i * 2 + 1] = false;
        }
    }

    for (int i = 0; i < 3; ++i) {
        BlockType blockHitType = EMPTY;
        int wallIdx = i * 2 + glm::max(0.f, glm::sign(rays[i][i]));
        bool wallExists = false;
        glm::ivec3 blockHit;

        for (int j = 0; j < 12; ++j) {
            float dist;
            if (!gridMarch(vertices[j], rays[i], terrain,
                           huggingWall[wallIdx], &dist, &blockHit)) {
                continue;
            }

            blockHitType = terrain.getBlockAt(blockHit.x, blockHit.y, blockHit.z);

            if (i == 1) {
                if (jumping && (blockHitType == WATER || blockHitType == LAVA))
                    m_velocity[i]=0.25f;
                else
                    jumping = false;
            }

            if (blockHitType == LAVA || blockHitType == WATER) {
                m_velocity *= 7 / 8.0f;
                wallExists = false;
                swimming = true;
            } else {
                m_velocity[i] = glm::sign(m_velocity[i]) * glm::min(glm::abs(m_velocity[i]), dist);
                wallExists = true;
                swimming = false;
            }
        }

        huggingWall[wallIdx] = wallExists;

        if(i==1)
            if (blockHitType == LAVA && blockHit.y+1>=m_position.y+1)
                medium = 2;
            else if (blockHitType == WATER && blockHit.y+1>=m_position.y+1)
                medium = 1;
            else
                medium = 0;

    }

}
bool Player::gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain& terrain,
               bool huggingWall, float* out_dist, glm::ivec3* out_blockHit) {
    float maxLen = glm::length(rayDirection);
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection);
    float curr_t = 0.f;

    while (curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);

        float interfaceAxis = -1;
        for (int i = 0; i < 3; ++i) {
            // if ray is travelling parallel to axis, will never intersect
            if (rayDirection[i] == 0.f) {
                continue;
            }

            float offset = glm::max(0.f, glm::sign(rayDirection[i]));

            if (huggingWall) {
                huggingWall = false;
                if (rayOrigin[i] == currCell[i] && offset == 1.f) {
                    offset = 0.f;
                }
            } else if (rayOrigin[i] == currCell[i] && offset == 0.f) {
                offset = -1.f;
            }

            int nextIntercept = currCell[i] + offset;
            float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
            axis_t = glm::min(axis_t, maxLen);

            if (axis_t < min_t) {
                min_t = axis_t;
                interfaceAxis = i;
            }
        }

        // no intersection between any axis and ray
        if (interfaceAxis == -1) {
            return false;
        }

        curr_t += min_t;
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0, 0, 0);

        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;

        if (terrain.getBlockAt(currCell.x, currCell.y, currCell.z) != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }

    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

std::array<glm::vec3, 12> Player::getCollisionVertices() {
    std::array<glm::vec3, 12> collisionVertices;

    for (int i = 0; i < 3; ++i) {
        // front-right, front-left, back-left, back-right
        collisionVertices[i * 4] = glm::vec3(m_position.x + 0.5f, m_position.y + i, m_position.z + 0.5f);
        collisionVertices[i * 4 + 1] = glm::vec3(m_position.x - 0.5f, m_position.y + i, m_position.z + 0.5f);
        collisionVertices[i * 4 + 2] = glm::vec3(m_position.x - 0.5f, m_position.y + i, m_position.z - 0.5f);
        collisionVertices[i * 4 + 3] = glm::vec3(m_position.x + 0.5f, m_position.y + i, m_position.z - 0.5f);
    }

    return collisionVertices;
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}

bool Player::getFlightMode() const {
    return flightMode;
}

void Player::setFlightMode(bool flightMode) {
    this->flightMode = flightMode;
}

bool Player::getFlightModeSet() const {
    return flightModeSet;
}

void Player::setFlightModeSet(bool modified) {
    flightModeSet = modified;
}

bool Player::isJumping() const {
    return jumping;
}

bool Player::isSwimming() const{
    return swimming;
}

void Player::setJumping(bool jumping) {
    if(jumping)
        m_velocity.y = 0.5f;
    this->jumping = jumping;
}

void Player::addBlock(Terrain &terrain) {
    glm::vec3 source = glm::vec3(m_position.x, m_position.y + 1.5, m_position.z);
    glm::vec3 ray = glm::normalize(m_forward) * 3.f;

    float dist;
    glm::ivec3 blockHit;

    if (!gridMarch(source, ray, terrain, true, &dist, &blockHit)) {
        return;
    }

    BlockType clicked = terrain.getBlockAt(blockHit.x, blockHit.y, blockHit.z);
    terrain.setBlockAt(blockHit.x, blockHit.y, blockHit.z + 1, clicked);

}

void Player::removeBlock(Terrain &terrain) {
    glm::vec3 source = glm::vec3(m_position.x, m_position.y + 1.5, m_position.z);
    glm::vec3 ray = glm::normalize(m_forward) * 3.f;

    float dist;
    glm::ivec3 blockHit;

    if (!gridMarch(source, ray, terrain, true, &dist, &blockHit)) {
            return;
    }

    if(terrain.getBlockAt(blockHit.x,blockHit.y,blockHit.z) != BEDROCK)
    {
        terrain.setBlockAt(blockHit.x, blockHit.y, blockHit.z, EMPTY);
        terrain.getChunkAt(blockHit.x,blockHit.z)->createVBOdata();
        switch(blockHit.x%16)
        {
        case 0:
            terrain.getChunkAt(blockHit.x-1,blockHit.z)->createVBOdata();
            break;
        case 15:
            terrain.getChunkAt(blockHit.x+1,blockHit.z)->createVBOdata();
            break;
        }
        switch(blockHit.z%16)
        {
        case 0:
            terrain.getChunkAt(blockHit.x,blockHit.z-1)->createVBOdata();
            break;
        case 15:
            terrain.getChunkAt(blockHit.x,blockHit.z+1)->createVBOdata();
            break;
        }
    }
}
