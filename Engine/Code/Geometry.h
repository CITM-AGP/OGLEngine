#pragma once

struct VertexV3V2
{
	glm::vec3 pos;
	glm::vec2 uv;
};

const VertexV3V2 vertices[] =
{
	{ glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)}, // bottom-left vertex
	{ glm::vec3(1.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)}, // bottom-right vertex
	{ glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)}, // top-right vertex
	{ glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)}, // top-left vertex
};

const u16 indices[] =
{
	0, 1, 2,
	0, 2, 3
};