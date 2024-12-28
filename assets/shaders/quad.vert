#version 430 core

layout (location = 0)in vec3 aPos;

void main()
{
  // Generating vertices on the GPU
  // mostly because we have a 2D Engine

  // OpenGL Coordinates
  // -1/ 1              1/ 1
  // -1/-1              1/-1

  vec2 vertices[6] =
  {
    // Top Left
    vec2(-0.5, 0.5),

    // Bottom Left
    vec2(-0.5, -0.5),

    // Top Right
    vec2( 0.5, 0.5),

    // Top Right
    vec2( 0.5, 0.5),

    // Bottom Left
    vec2(-0.5, -0.5),

    // Bottom Right
    vec2( 0.5, -0.5)
  };

/*
  float vertices[] =
  {
    -0.5, 0.5, 0.0,
    0.5, 0.5, 0.0,
    0.5, -0.5, 0.0,
    -0.5, -0.5, 0.0
  };
*/
  gl_Position = vec4(vertices[gl_VertexID], 1.0, 1.0);
 // gl_Position = vec4(a.Pos.x, aPos.y, aPos.z, 1.0);
}