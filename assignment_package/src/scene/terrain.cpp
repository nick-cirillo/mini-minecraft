
#include "terrain.h"
#include "cube.h"
#include "mygl.h"
#include <stdexcept>
#include <iostream>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), m_geomCube(context), mp_context(context)
{}

Terrain::~Terrain() {
    m_geomCube.destroyVBOdata();
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}

bool Terrain::hasTerrainZoneAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 64.f));
    int zFloor = static_cast<int>(glm::floor(z / 64.f));
    return m_generatedTerrain.find(toKey(64 * xFloor, 64 * zFloor)) != m_generatedTerrain.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context, x, z);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }

    m_setupChunks.insert({toKey(x, z), false});

    return cPtr;
    return cPtr;
}

void Terrain::checkForNewChunks() {
    // Get player position
    glm::vec2 chunkPos = getChunkPos();

    int chunkX = static_cast<int>(chunkPos.x);
    int chunkZ = static_cast<int>(chunkPos.y);

    // Check whether the player is within 16 blocks of the edge of a Chunk which does not have another neighboring Chunk loaded,
    // and if so, insert a new Chunk into the Terrain, and set up the VBOs.
    checkAndLoadChunk(chunkX, chunkZ); // curr
    checkAndLoadChunk(chunkX + 16, chunkZ); // east
    checkAndLoadChunk(chunkX - 16, chunkZ); // west
    checkAndLoadChunk(chunkX, chunkZ + 16); // north
    checkAndLoadChunk(chunkX, chunkZ - 16); // south
    checkAndLoadChunk(chunkX + 16, chunkZ + 16); // ne
    checkAndLoadChunk(chunkX + 16, chunkZ - 16); // nw
    checkAndLoadChunk(chunkX - 16, chunkZ - 16); // sw
    checkAndLoadChunk(chunkX - 16, chunkZ + 16); // se
}

void Terrain::checkAndLoadChunk(int x, int z) {
    if (!hasChunkAt(x, z)) {
        //do everyting to initialize the chunk
        instantiateChunkAt(x, z);

        const uPtr<Chunk> &newChunk = getChunkAt(x, z);
        newChunk->createVBOdata();
        m_setupChunks[toKey(x, z)] = true;
    } else if (m_setupChunks.count(toKey(x, z)) == 0) {
        const uPtr<Chunk> &newChunk = getChunkAt(x, z);
        newChunk->createVBOdata();
        m_setupChunks.insert({toKey(x, z), true});
    } else if (!m_setupChunks.at(toKey(x, z))) {
        const uPtr<Chunk> &newChunk = getChunkAt(x, z);
        newChunk->createVBOdata();
        m_setupChunks[toKey(x, z)] = true;
    }
}

glm::vec2 Terrain::getChunkPos() {
    // Get player position
    glm::vec3 playerPos = static_cast<MyGL*>(mp_context)->m_player.mcr_position;
    // Check whether the player is within 16 blocks of the edge of a Chunk which does not have another neighboring Chunk loaded,
    // and if so, insert a new Chunk into the Terrain, and set up the VBOs.

    glm::vec2 chunkPos = 16.f * glm::vec2(glm::floor(playerPos.x / 16.f), glm::floor(playerPos.z / 16.f));

    return chunkPos;
}

glm::vec2 Terrain::getTerrainPos() {
    // Get player position
    glm::vec3 playerPos = static_cast<MyGL*>(mp_context)->m_player.mcr_position;
    // Check whether the player is within 16 blocks of the edge of a Chunk which does not have another neighboring Chunk loaded,
    // and if so, insert a new Chunk into the Terrain, and set up the VBOs.

    glm::vec2 chunkPos = 64.f * glm::vec2(glm::floor(playerPos.x / 64.f), glm::floor(playerPos.z / 64.f));

    return chunkPos;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {
    for(int z = minZ; z < maxZ; z += 16) {
        for(int x = minX; x < maxX; x += 16) {
            if (hasChunkAt(x, z)) {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                if (m_setupChunks.count(toKey(x, z)) > 0 && !m_setupChunks.at(toKey(x, z))) {

                    chunk->createVBOdata();
                    m_setupChunks[toKey(x, z)] = true;
                }

                shaderProgram->setModelMatrix(glm::translate(mat4(1.f), vec3(x, 0.f, z)));
                shaderProgram->drawInterleaved(static_cast<Drawable&>(*chunk));
            }
        }
    }

    // Transparent
    for(int z = minZ; z < maxZ; z += 16) {
        for(int x = minX; x < maxX; x += 16) {
            if (hasChunkAt(x, z)) {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                if (m_setupChunks.count(toKey(x, z)) > 0 && !m_setupChunks.at(toKey(x, z))) {

                    chunk->createVBOdata();
                    m_setupChunks[toKey(x, z)] = true;
                }

                shaderProgram->setModelMatrix(glm::translate(mat4(1.f), vec3(x, 0.f, z)));
                shaderProgram->drawTpInterleaved(static_cast<Drawable&>(*chunk));
            }
        }
    }
}

void Terrain::CreateTestScene()
{
    m_geomCube.createVBOdata();

    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for(int x = 0; x < 64; x += 16) {
        for(int z = 0; z < 64; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    // Tell our existing terrain set that
    // the "generated terrain zone" at (0,0)
    // now exists.
    m_generatedTerrain.insert(toKey(0, 0));

    // Create the basic terrain floor
    for(int x = 0; x < 64; ++x) {
        for(int z = 0; z < 64; ++z) {
            if((x + z) % 2 == 0) {
                setBlockAt(x, 128, z, STONE);
            }
            else {
                setBlockAt(x, 128, z, DIRT);
            }
        }
    }
    // Add "walls" for collision testing
    for(int x = 0; x < 64; ++x) {
        setBlockAt(x, 129, 0, GRASS);
        setBlockAt(x, 130, 0, GRASS);
        setBlockAt(x, 129, 63, GRASS);
        setBlockAt(0, 130, x, GRASS);
    }
    // Add a central column
    for(int y = 129; y < 140; ++y) {
        setBlockAt(32, y, 32, GRASS);
    }
}

// This function creates new terrain for a specific terrain
// generation zone, ie:
void Terrain::CreateProceduralTerrain(int minX,int maxX, int minZ, int maxZ)
{
    // TODO: DELETE THIS LINE WHEN YOU DELETE m_geomCube!
    m_geomCube.createVBOdata();

    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    // Tell our existing terrain set that
    // the "generated terrain zones"
    // now exist.
    for (int z = minZ; z < maxZ; z += 64) {
        for (int x = minX; x < maxX; x += 64) {
            m_generatedTerrain.insert(toKey(x, z));
        }
    }

    // Create the basic terrain floor
    for(int x = minX; x < maxX; x ++) {
        for(int z = minZ; z < maxZ; z ++) {
            float b1 = glm::smoothstep(0.25f, 0.75f, PerlinNoise(vec2(x,z)/1.f));
            float b2 = glm::smoothstep(0.25f, 0.75f, PerlinNoise(vec2(x,z)/1024.f + 1323112334432432.f));
            int Y = calcHeight(x,z,b1,b2);
            for(int y = 0;y<=std::max(Y,138);y++){
//                if (y<std::max(Y,138))
//                    continue;
                setBlockAt(x, y, z, biomeBlock(x,y,z,Y,b1,b2));
            }
        }
    }
}

vec2 Terrain::smoothF(vec2 uv)
{
    return uv*uv*(3.f-2.f*uv);
}

float Terrain::noise(vec2 uv)
{
    const float k = 257.;
    vec4 l  = vec4(floor(uv),fract(uv));
    float u = l.x + l.y * k;
    vec4 v  = vec4(u, u+1.,u+k, u+k+1.);
    v       = fract(fract(1.23456789f *v)*v/.987654321f);
    vec2 L    = smoothF(vec2(l[2],l[3]));
    l.x     = mix(v.x, v.y, L[0]);
    l.y     = mix(v.z, v.w, L[0]);
    return    mix(l.x, l.y, L[1]);
}

float Terrain::fbm(vec2 uv)
{
    float a = 0.5;
    float f = 5.0;
    float n = 0.;
    int it = 8;
    for(int i = 0; i < 32; i++)
    {
        if(i<it)
        {
            n += noise(uv*f)*a;
            a *= .5;
            f *= 2.;
        }
    }
    return n;
}

float Terrain::WorleyNoise(vec2 uv)
{
    // Tile the space
    //    uv = uv + fbm2(uv / 4) * 5.f;
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0; // Minimum distance initialized to max.
    float secondMinDist = 1.0;
    vec2 closestPoint;

    // Search all neighboring cells and this cell for their point
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            // Random point inside current neighboring cell
            vec2 point = random2(uvInt + neighbor);

            // Compute the distance b/t the point and the fragment
            // Store the min dist thus far
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            if(dist < minDist) {
                secondMinDist = minDist;
                minDist = dist;
                closestPoint = point;
            }
            else if(dist < secondMinDist) {
                secondMinDist = dist;
            }
        }
    }
    float height;// = 0.5 * minDist + 0.5 * secondMinDist;
    height = minDist;
    //    height = height * height;
    return height;
}

int Terrain::calcHeight(int x, int z, float b1, float b2){
    vec2 xz = vec2(x,z);

    float h = 0;

    float amp = 1/2.0;
    float freq1 = 256;
    float freq2 = 64;

    for(int i = 0; i < 4; ++i) {
        vec2 offset = vec2((float)fbm(xz / 256.f), (float)fbm(xz / 300.f))+ vec2(1000);

        float h1 = PerlinNoise((xz + offset * 75.f) / freq1);

        h += h1 * amp;

        amp *= 0.5;
        freq1 *= 0.5;
    }

    float H = 0;
    amp = 1/2.0;
    for(int i = 0; i < 4; ++i) {
        float h2 = WorleyNoise(xz / freq2);

        H += h2 * amp;

        amp *= 0.5;
        freq2 *= 0.5;
    }

    float v = mix(h,H,b1);


    h = 0;
    amp = 1/2.0;
    freq1 = 512;
    freq2 = 128;

    for(int i = 0; i < 4; ++i) {
        vec2 offset = vec2((float)fbm(xz / 2560.f), (float)fbm(xz / 3000.f));

        float h1 = PerlinNoise((xz + offset * 175.f) / freq1);

        h += h1 * amp;

        amp *= 0.5;
        freq1 *= 0.5;
    }

    H = 0;
    amp = 1/2.0;
    for(int i = 0; i < 4; ++i) {
        float h2 = WorleyNoise(xz / freq2)*0.2;

        H += h2 * amp;

        amp *= 0.5;
        freq2 *= 0.5;
    }

    v = floor(128+mix(v,mix(h,H,b1), b2)*128);
    return v;
}

float Terrain::surflet(vec2 P, vec2 gridPoint)
{
    // Compute falloff function by converting linear distance to a polynomial (quintic smootherstep function)
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1 - 6 * std::pow(distX, 5.0) + 15 * std::pow(distX, 4.0) - 10 * std::pow(distX, 3.0);
    float tY = 1 - 6 * std::pow(distY, 5.0) + 15 * std::pow(distY, 4.0) - 10 * std::pow(distY, 3.0);

    // Get the random vector for the grid point
    vec2 gradient = random2(gridPoint);
    // Get the vector from the grid point to P
    vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

vec2 Terrain::random2( vec2 p ) {
    return normalize(2.f * (glm::fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))
                                       * 43758.5453f)) - 1.f);
}

float Terrain::PerlinNoise(vec2 uv)
{
    // Tile the space
    vec2 uvXLYL = floor(uv);
    vec2 uvXHYL = uvXLYL + vec2(1,0);
    vec2 uvXHYH = uvXLYL + vec2(1,1);
    vec2 uvXLYH = uvXLYL + vec2(0,1);

    return surflet(uv, uvXLYL) + surflet(uv, uvXHYL) + surflet(uv, uvXHYH) + surflet(uv, uvXLYH);
}

vec3 Terrain::random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 321.5)),
                          dot(p,vec3(269.5, 183.3,43243.0)),
                          dot(p, vec3(420.6, 631.2,321.43))
                          )) * 43758.5453f);
}


vec3 Terrain::pow(vec3 v, float f)
{
    return vec3(std::pow(v[0],f),std::pow(v[1],f),std::pow(v[2],f));
}

float Terrain::surflet(vec3 p, vec3 gridPoint) {
    // Compute the distance between p and the grid point along each axis, and warp it with a
    // quintic function so we can smooth our cells
    vec3 t2 = abs(p - gridPoint);
    vec3 t = vec3(1.f) - 6.f * pow(t2, 5.f) + 15.f * pow(t2, 4.f) - 10.f * pow(t2, 3.f);//+20.f*pow(t2,2.f);
    // Get the random vector for the grid point (assume we wrote a function random2
    // that returns a vec2 in the range [0, 1])
    vec3 gradient = random3(gridPoint) * 2.f - vec3(1., 1., 1.);
    // Get the vector from the grid point to P
    vec3 diff = p - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    //    std::cout<<height<<t[0]<<t[1]<<t[2]<<std::endl;
    return height * t.x * t.y * t.z;
}


float Terrain::perlinNoise3D(vec3 p) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                float s = surflet(p, floor(p) + vec3(dx, dy, dz));
                //                std::cout<<s<<std::endl;
                surfletSum += s;
            }
        }
    }
    return surfletSum;
}


BlockType Terrain::biomeBlock(int x, int y, int z, int maxY, float b1, float b2){
    if(y==0)
        return BEDROCK;
    if(b2<0.5){
        if (y<=128 && y<=maxY)
        {
            vec3 v = vec3(x,y,z);
            float freq1 = 64;
            float freq2 = 16;
            vec2 xz = vec2(x,z);
            vec3 offset = vec3((float)fbm(xz / 256.f),0, (float)fbm(xz / 300.f))+ vec3(1000);
            float p1 =perlinNoise3D((v+offset)/freq1);
            float p2 =perlinNoise3D((v+offset)/freq2);
            return p1<= -0.35 || abs(p2)<0.125? y<=25?LAVA: maxY<128?WATER: EMPTY  :STONE;
        }
        if(y>maxY)
            return WATER;
        if(b1<0.5){
            if(y==maxY)
                return GRASS;
            return DIRT;
        }
        else{
            if (y>=200 && y==maxY)
                return SNOW;
            float h = WorleyNoise(vec2(x,z)/64.f);
            return h<0.95?STONE:DIRT;
        }
    }
    else{
        if (y<=128 && y<=maxY)
        {
            vec3 v = vec3(x,y,z);
            float freq1 = 64;
            float freq2 = 16;
            vec2 xz = vec2(x,z);
            vec3 offset = vec3((float)fbm(xz / 256.f),0, (float)fbm(xz / 300.f))+ vec3(1000);
            float p1 =perlinNoise3D((v+offset)/freq1);
            float p2 =perlinNoise3D((v+offset)/freq2);
            return p1<= -0.35 || abs(p2)<0.125? y<=25?LAVA: EMPTY  :STONE;
        }
        if(b1<0.5){
            if(y>maxY)
                return EMPTY;
            return y<maxY-3?STONE:SAND;
        }
        else{
            if(y>maxY)
                return ICE;
            return y<maxY-5?STONE:SNOW;
        }
    }
}
