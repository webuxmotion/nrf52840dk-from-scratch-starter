#include "render.h"
#include <zephyr/display/cfb.h>
#include <math.h>
#include <string.h>

typedef struct { float x; float y; } Point2D;
typedef struct { int16_t x; int16_t y; float radius; float z; bool visible; } RenderObject;

void render_frame(const struct device *display, float angle) {
  struct cfb_position pA = {
    x: 10,
    y: 10
  };

  struct cfb_position pB = {
    x: pA.x + (int16_t)lroundf(cosf(angle) * 20.0f),
    y: pA.y + (int16_t)lroundf(sinf(angle) * 20.0f)
  };

  cfb_draw_line(display, &pA, &pB);
}
