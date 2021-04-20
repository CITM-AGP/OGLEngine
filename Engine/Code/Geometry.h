#pragma once

struct VertexV3V2
{
	glm::vec3 pos;
	glm::vec2 uv;
};

const VertexV3V2 vertices[] =
{
	{ glm::vec3(-0.5, -0.5, 0.0), glm::vec2(0.0, 0.0)}, // bottom-left vertex
	{ glm::vec3(0.5, -0.5, 0.0), glm::vec2(1.0, 0.0)}, // bottom-right vertex
	{ glm::vec3(0.5, 0.5, 0.0), glm::vec2(1.0, 1.0)}, // top-right vertex
	{ glm::vec3(-0.5, 0.5, 0.0), glm::vec2(0.0, 1.0)}, // top-left vertex
};

const u16 indices[] =
{
	0, 1, 2,
	0, 2, 3
};