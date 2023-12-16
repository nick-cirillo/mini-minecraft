#include "chunk.h"

using namespace std;
using namespace glm;

Chunk::Chunk(OpenGLContext* context, int x, int z) : Drawable(context), m_blocks(), minX(x), minZ(z), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);

    neighboringFaces[0].direction = XPOS;
    neighboringFaces[1].direction = XNEG;
    neighboringFaces[2].direction = YPOS;
    neighboringFaces[3].direction = YNEG;
    neighboringFaces[4].direction = ZPOS;
    neighboringFaces[5].direction = ZNEG;

    neighboringFaces[0].dirVec = vec3(1, 0, 0);
    neighboringFaces[1].dirVec = vec3(-1, 0, 0);
    neighboringFaces[2].dirVec = vec3(0, 1, 0);
    neighboringFaces[3].dirVec = vec3(0, -1, 0);
    neighboringFaces[4].dirVec = vec3(0, 0, 1);
    neighboringFaces[5].dirVec = vec3(0, 0, -1);

    neighboringFaces[0].pos = {vec4(1, 0, 0, 1), vec4(1, 0, 1, 1), vec4(1, 1, 1, 1), vec4(1, 1, 0, 1)};
    neighboringFaces[1].pos = {vec4(0, 0, 1, 1), vec4(0, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 1, 1, 1)};
    neighboringFaces[2].pos = {vec4(0, 1, 0, 1), vec4(1, 1, 0, 1), vec4(1, 1, 1, 1), vec4(0, 1, 1, 1)};
    neighboringFaces[3].pos = {vec4(0, 0, 0, 1), vec4(0, 0, 1, 1), vec4(1, 0, 1, 1), vec4(1, 0, 0, 1)};
    neighboringFaces[4].pos = {vec4(1, 0, 1, 1), vec4(0, 0, 1, 1), vec4(0, 1, 1, 1), vec4(1, 1, 1, 1)};
    neighboringFaces[5].pos = {vec4(0, 0, 0, 1), vec4(1, 0, 0, 1), vec4(1, 1, 0, 1), vec4(0, 1, 0, 1)};

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            neighboringFaces[i].nor[j] = vec4(neighboringFaces[i].dirVec, 0);
        }
    }
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::createVBOdata() {
    vector<int> idx;
    vector<vec4> vbo;
    /*
     * Structure of vbo:
     * pos0 nor0 col0
     * pos1 nor1 col1 uv1
     * pos2 nor2 col2 uv2
     * etc
     */

    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 16; x++) {
                BlockType curr = getBlockAt(x, y, z);
                if (curr != EMPTY && transparentBlocks.count(curr) == 0) {
                    for (auto &face : neighboringFaces) {
                        // Set position of neighboring block
                        vec3 neighborPos = vec3(x, y, z) + face.dirVec;
                        BlockType neighbor;

                        // Go into neighboring chunks if necessary and get neighboring block type
                        if (neighborPos.x < 0) {
                            Chunk* neighborChunk = m_neighbors.at(XNEG);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.x += 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else if (neighborPos.x >= 16) {
                            Chunk* neighborChunk = m_neighbors.at(XPOS);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.x -= 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else if (neighborPos.y < 0) {
                            neighbor = EMPTY;
                        } else if (neighborPos.y >= 16) {
                            neighbor = EMPTY;
                        } else if (neighborPos.z < 0) {
                            Chunk* neighborChunk = m_neighbors.at(ZNEG);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.z += 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else if (neighborPos.z >= 16) {
                            Chunk* neighborChunk = m_neighbors.at(ZPOS);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.z -= 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else {
                            neighbor = getBlockAt((int) neighborPos.x, (int) neighborPos.y, (int) neighborPos.z);
                        }

                        // If the neighboring block is empty, set up the VBO
                        if (neighbor == EMPTY || transparentBlocks.count(neighbor) > 0) {
                            int start = vbo.size() / 4;


                            vec4 color;
                            if (colorMap.count(curr) == 0) {
                                color = vec4(1, 0, 1, 1);
                            } else {
                                color = colorMap.at(curr);
                            }

                            float animated = 0.f;
                            if (animatedBlocks.count(curr) > 0) {
                                animated = 1.f;
                            }

                            vec4 tex;
                            if (texMap.count(curr) == 0) {
                                tex = vec4(texMap.at(OTHER).at(face.direction), 0, 0);
                            } else {
                                tex = vec4(texMap.at(curr).at(face.direction), 0, animated);
                            }

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[0]), 1));
                            vbo.push_back(face.nor[0]);
                            vbo.push_back(color);
                            vbo.push_back(tex + vec4(BLK_UV, 0, 0, 0));

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[1]), 1));
                            vbo.push_back(face.nor[1]);
                            vbo.push_back(color);
                            vbo.push_back(tex);

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[2]), 1));
                            vbo.push_back(face.nor[2]);
                            vbo.push_back(color);
                            vbo.push_back(tex + vec4(0, BLK_UV, 0, 0));

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[3]), 1));
                            vbo.push_back(face.nor[3]);
                            vbo.push_back(color);
                            vbo.push_back(tex + vec4(BLK_UV, BLK_UV, 0, 0));

                            idx.push_back(start);
                            idx.push_back(start + 1);
                            idx.push_back(start + 2);
                            idx.push_back(start);
                            idx.push_back(start + 2);
                            idx.push_back(start + 3);
                        }
                    }
                }
            }
        }
    }

    bufferVBOdata(idx, vbo);

    createTpVBOdata();
}

void Chunk::bufferVBOdata(vector<int> idx, vector<vec4> vbo) {
    m_count = idx.size();

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(int), idx.data(), GL_STATIC_DRAW);

    generateVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vbo.size() * sizeof(vec4), vbo.data(), GL_STATIC_DRAW);
}



void Chunk::createTpVBOdata() {
    vector<int> idx;
    vector<vec4> vbo;
    /*
     * Structure of vbo:
     * pos0 nor0 col0
     * pos1 nor1 col1 uv1
     * pos2 nor2 col2 uv2
     * etc
     */

    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 16; x++) {
                BlockType curr = getBlockAt(x, y, z);
                if (curr != EMPTY && transparentBlocks.count(curr) == 1) {
                    for (auto &face : neighboringFaces) {
                        // Set position of neighboring block
                        vec3 neighborPos = vec3(x, y, z) + face.dirVec;
                        BlockType neighbor;

                        // Go into neighboring chunks if necessary and get neighboring block type
                        if (neighborPos.x < 0) {
                            Chunk* neighborChunk = m_neighbors.at(XNEG);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.x += 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else if (neighborPos.x >= 16) {
                            Chunk* neighborChunk = m_neighbors.at(XPOS);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.x -= 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else if (neighborPos.y < 0) {
                            neighbor = EMPTY;
                        } else if (neighborPos.y >= 16) {
                            neighbor = EMPTY;
                        } else if (neighborPos.z < 0) {
                            Chunk* neighborChunk = m_neighbors.at(ZNEG);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.z += 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else if (neighborPos.z >= 16) {
                            Chunk* neighborChunk = m_neighbors.at(ZPOS);
                            if (neighborChunk == nullptr) {
                                neighbor = EMPTY;
                            } else {
                                neighborPos.z -= 16;
                                neighbor = neighborChunk->getBlockAt((int) neighborPos.x,
                                                                     (int) neighborPos.y, (int) neighborPos.z);
                            }
                        } else {
                            neighbor = getBlockAt((int) neighborPos.x, (int) neighborPos.y, (int) neighborPos.z);
                        }

                        // If the neighboring block is empty, set up the VBO
                        if (neighbor == EMPTY || transparentBlocks.count(neighbor) > 0) {
                            int start = vbo.size() / 4;


                            vec4 color;
                            if (colorMap.count(curr) == 0) {
                                color = vec4(1, 0, 1, 1);
                            } else {
                                color = colorMap.at(curr);
                            }

                            float animated = 0.f;
                            if (animatedBlocks.count(curr) > 0) {
                                animated = 1.f;
                            }

                            vec4 tex;
                            if (texMap.count(curr) == 0) {
                                tex = vec4(texMap.at(OTHER).at(face.direction), 0, 0);
                            } else {
                                tex = vec4(texMap.at(curr).at(face.direction), 0, animated);
                            }

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[0]), 1));
                            vbo.push_back(face.nor[0]);
                            vbo.push_back(color);
                            vbo.push_back(tex + vec4(BLK_UV, 0, 0, 0));

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[1]), 1));
                            vbo.push_back(face.nor[1]);
                            vbo.push_back(color);
                            vbo.push_back(tex);

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[2]), 1));
                            vbo.push_back(face.nor[2]);
                            vbo.push_back(color);
                            vbo.push_back(tex + vec4(0, BLK_UV, 0, 0));

                            vbo.push_back(vec4(vec3(x, y, z) + vec3(face.pos[3]), 1));
                            vbo.push_back(face.nor[3]);
                            vbo.push_back(color);
                            vbo.push_back(tex + vec4(BLK_UV, BLK_UV, 0, 0));

                            idx.push_back(start);
                            idx.push_back(start + 1);
                            idx.push_back(start + 2);
                            idx.push_back(start);
                            idx.push_back(start + 2);
                            idx.push_back(start + 3);
                        }
                    }
                }
            }
        }
    }

    bufferTpVBOdata(idx, vbo);
}

void Chunk::bufferTpVBOdata(vector<int> idx, vector<vec4> vbo) {
    m_tpCount = idx.size();

    generateTpIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufTpIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(int), idx.data(), GL_STATIC_DRAW);

    generateTpVBO();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufTpVBO);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vbo.size() * sizeof(vec4), vbo.data(), GL_STATIC_DRAW);
}
