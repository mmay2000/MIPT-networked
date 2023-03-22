#pragma once
#include <cstdint>

constexpr uint16_t invalid_entity = -1;
constexpr float min_size = 6;
constexpr float init_size = 10;
constexpr int win_width = 700;
constexpr int win_height = 700;
constexpr float ai_speed = 0.1;

enum class EntityType
{
	NONE,
	AI,
	PLAYER
};

struct Entity
{
  uint32_t color = 0xff00ffff;
  float x = 0.f;
  float y = 0.f;
  uint16_t eid = invalid_entity;
  float bodySize = 10.f;
  float size = min_size;
  EntityType type = EntityType::NONE;
  float destinationX = 0.f; //destination for AI
  float destinationY = 0.f;
};


