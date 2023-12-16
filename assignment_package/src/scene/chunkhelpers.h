#ifndef CHUNKHELPERS_H
#define CHUNKHELPERS_H
#include <unordered_set>
#pragma once


#include "glm_includes.h"
#include <array>
#include <unordered_map>

using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};


// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Block face data
struct BlockFace {
    Direction direction;
    glm::vec3 dirVec;
    std::array<glm::vec4, 4> pos;
    std::array<glm::vec4, 4> nor;
};

enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, LAVA, SNOW, BEDROCK, SAND, ICE, OTHER
};

const static unordered_set<BlockType> animatedBlocks = {
    WATER, LAVA
};

const static unordered_set<BlockType> transparentBlocks = {
    WATER, ICE
};

const static unordered_map<BlockType, glm::vec4> colorMap = {
    {GRASS, (glm::vec4(95.f, 159.f, 53.f, 255.f) / 255.f)},
    {DIRT, (glm::vec4(121.f, 85.f, 58.f, 255.f) / 255.f)},
    {STONE, glm::vec4(0.5, 0.5, 0.5, 1)},
    {WATER, glm::vec4(0, 0, 0.75, 0.5)},    
    {ICE, glm::vec4(0, 0, 1, 0.5)},
    {LAVA, glm::vec4(1, 0, 0, 1)},
    {SNOW, glm::vec4(1, 1, 1, 1)},
    {SAND, glm::vec4(1, 1, 0, 1)},
    {BEDROCK, glm::vec4(0, 0, 0, 1)}
};

#define BLK_UV 0.0625f
#define BLK_UVX * 0.0625f
#define BLK_UVY * 0.0625f

const static unordered_map<BlockType, unordered_map<Direction, glm::vec2, EnumHash>, EnumHash> texMap = {
    {GRASS, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(3.f BLK_UVX, 15.f BLK_UVY)},
                                                          {XNEG, glm::vec2(3.f BLK_UVX, 15.f BLK_UVY)},
                                                          {YPOS, glm::vec2(8.f BLK_UVX, 13.f BLK_UVY)},
                                                          {YNEG, glm::vec2(2.f BLK_UVX, 15.f BLK_UVY)},
                                                          {ZPOS, glm::vec2(3.f BLK_UVX, 15.f BLK_UVY)},
                                                          {ZNEG, glm::vec2(3.f BLK_UVX, 15.f BLK_UVY)}}},
    {DIRT, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(2.f BLK_UVX, 15.f BLK_UVY)},
                                                         {XNEG, glm::vec2(2.f BLK_UVX, 15.f BLK_UVY)},
                                                         {YPOS, glm::vec2(2.f BLK_UVX, 15.f BLK_UVY)},
                                                         {YNEG, glm::vec2(2.f BLK_UVX, 15.f BLK_UVY)},
                                                         {ZPOS, glm::vec2(2.f BLK_UVX, 15.f BLK_UVY)},
                                                         {ZNEG, glm::vec2(2.f BLK_UVX, 15.f BLK_UVY)}}},
    {STONE, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(1.f BLK_UVX, 15.f BLK_UVY)},
                                                          {XNEG, glm::vec2(1.f BLK_UVX, 15.f BLK_UVY)},
                                                          {YPOS, glm::vec2(1.f BLK_UVX, 15.f BLK_UVY)},
                                                          {YNEG, glm::vec2(1.f BLK_UVX, 15.f BLK_UVY)},
                                                          {ZPOS, glm::vec2(1.f BLK_UVX, 15.f BLK_UVY)},
                                                          {ZNEG, glm::vec2(1.f BLK_UVX, 15.f BLK_UVY)}}},
    {WATER, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(13.f BLK_UVX, 3.f BLK_UVY)},
                                                          {XNEG, glm::vec2(13.f BLK_UVX, 3.f BLK_UVY)},
                                                          {YPOS, glm::vec2(13.f BLK_UVX, 3.f BLK_UVY)},
                                                          {YNEG, glm::vec2(13.f BLK_UVX, 3.f BLK_UVY)},
                                                          {ZPOS, glm::vec2(13.f BLK_UVX, 3.f BLK_UVY)},
                                                          {ZNEG, glm::vec2(13.f BLK_UVX, 3.f BLK_UVY)}}},
    {LAVA, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(13.f BLK_UVX, 1.f BLK_UVY)},
                                                         {XNEG, glm::vec2(13.f BLK_UVX, 1.f BLK_UVY)},
                                                         {YPOS, glm::vec2(13.f BLK_UVX, 1.f BLK_UVY)},
                                                         {YNEG, glm::vec2(13.f BLK_UVX, 1.f BLK_UVY)},
                                                         {ZPOS, glm::vec2(13.f BLK_UVX, 1.f BLK_UVY)},
                                                         {ZNEG, glm::vec2(13.f BLK_UVX, 1.f BLK_UVY)}}},
    {SNOW, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(2.f BLK_UVX, 11.f BLK_UVY)},
                                                         {XNEG, glm::vec2(2.f BLK_UVX, 11.f BLK_UVY)},
                                                         {YPOS, glm::vec2(2.f BLK_UVX, 11.f BLK_UVY)},
                                                         {YNEG, glm::vec2(2.f BLK_UVX, 11.f BLK_UVY)},
                                                         {ZPOS, glm::vec2(2.f BLK_UVX, 11.f BLK_UVY)},
                                                         {ZNEG, glm::vec2(2.f BLK_UVX, 11.f BLK_UVY)}}},
    {BEDROCK, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(1.f BLK_UVX, 14.f BLK_UVY)},
                                                            {XNEG, glm::vec2(1.f BLK_UVX, 14.f BLK_UVY)},
                                                            {YPOS, glm::vec2(1.f BLK_UVX, 14.f BLK_UVY)},
                                                            {YNEG, glm::vec2(1.f BLK_UVX, 14.f BLK_UVY)},
                                                            {ZPOS, glm::vec2(1.f BLK_UVX, 14.f BLK_UVY)},
                                                            {ZNEG, glm::vec2(1.f BLK_UVX, 14.f BLK_UVY)}}},
    {OTHER, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(7.f BLK_UVX, 1.f BLK_UVY)},
                                                          {XNEG, glm::vec2(7.f BLK_UVX, 1.f BLK_UVY)},
                                                          {YPOS, glm::vec2(7.f BLK_UVX, 1.f BLK_UVY)},
                                                          {YNEG, glm::vec2(7.f BLK_UVX, 1.f BLK_UVY)},
                                                          {ZPOS, glm::vec2(7.f BLK_UVX, 1.f BLK_UVY)},
                                                          {ZNEG, glm::vec2(7.f BLK_UVX, 1.f BLK_UVY)}}},
    {SAND, unordered_map<Direction, glm::vec2, EnumHash>   {{XPOS, glm::vec2(0.f BLK_UVX, 4.f BLK_UVY)},
                                                            {XNEG, glm::vec2(0.f BLK_UVX, 4.f BLK_UVY)},
                                                            {YPOS, glm::vec2(0.f BLK_UVX, 4.f BLK_UVY)},
                                                            {YNEG, glm::vec2(0.f BLK_UVX, 4.f BLK_UVY)},
                                                            {ZPOS, glm::vec2(0.f BLK_UVX, 4.f BLK_UVY)},
                                                            {ZNEG, glm::vec2(0.f BLK_UVX, 4.f BLK_UVY)}}},
    {ICE, unordered_map<Direction, glm::vec2, EnumHash> {{XPOS, glm::vec2(3.f BLK_UVX, 11.f BLK_UVY)},
                                                         {XNEG, glm::vec2(3.f BLK_UVX, 11.f BLK_UVY)},
                                                         {YPOS, glm::vec2(3.f BLK_UVX, 11.f BLK_UVY)},
                                                         {YNEG, glm::vec2(3.f BLK_UVX, 11.f BLK_UVY)},
                                                         {ZPOS, glm::vec2(3.f BLK_UVX, 11.f BLK_UVY)},
                                                         {ZNEG, glm::vec2(3.f BLK_UVX, 11.f BLK_UVY)}}}
};
#endif // CHUNKHELPERS_H
