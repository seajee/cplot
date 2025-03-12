// TODO: Dynamically change resolution (maybe not necessary)
// TODO: Dynamically change asymptote tolerance based on resolution
// TODO: Implement dynamic theme configuration
// TODO: I still don't understand the visible range in cartesian coordinates
// TODO: Make grid spacing a Vector2
// TODO: Auto grid spacing doesn't scale well
// TODO: Scaling moves camera towards the origin

#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MP_IMPLEMENTATION
#include "mp.h"

/* Macros */

// Constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define CAMERA_SPEED 50.0
#define ZOOM_FACTOR 5.0
#define ZOOM_DEFAULT 50.0
#define ZOOM_MIN 5.0
#define ZOOM_MAX 800.0
#define ASYMPTOTE_TOLERANCE 100.0
#define RESOLUTION_DEFAULT 0.008
#define GRID_SPACING_DEFAULT 2.0
#define TOGGLE_CONTINUOUS_DEFAULT true
#define TOGGLE_DEBUG_MENU_DEFAULT false
#define TOGGLE_GRID_DEFAULT true
#define CACHE_CAPACITY (8*1024)

// Styling
#define GRID_COLOR DARKGRAY
#define AXES_COLOR WHITE
#define NUMBER_COLOR GRAY
#define DEBUG_TEXT_COLOR LIME
#define FUNCTION_LINE_THICKNESS 2.0f
#define ASYMPTOTE_POINT_RADIUS 4.0f
#define ASYMPTOTE_POINT_COLOR LIGHTGRAY

/* Declarations */

typedef double (*func_t)(double);

Vector2 pjv(double x, double y);
double pjx(double x);
double pjy(double y);
Vector2 rpjv(double x, double y);
double rpjx(double x);
double rpjy(double y);
void plot(func_t f, Color color, double resolution);
size_t plot_parser(MP_Env *parser, Vector2 *buf, size_t buf_size, double resolution);
double max(double a, double b);
double map(double value, double x1, double x2, double y1, double y2);
bool is_near(double x, double target);

double cubic(double x);
double linear(double x);
double sine(double x);
double tangent(double x);
double asymptote1(double x);
double asymptote2(double x);
double asymptote3(double x);

/* Globals */

Vector2 camera = {0};
Vector2 scale = {ZOOM_DEFAULT, ZOOM_DEFAULT};
double resolution = RESOLUTION_DEFAULT;
double grid_spacing = GRID_SPACING_DEFAULT;
bool toggle_continuous = TOGGLE_CONTINUOUS_DEFAULT;
bool toggle_debug_menu = TOGGLE_DEBUG_MENU_DEFAULT;
bool toggle_grid = TOGGLE_GRID_DEFAULT;

Vector2 cache[CACHE_CAPACITY];
size_t cache_count = 0;
Vector2 prev_camera = {1.0f, 1.0f};
Vector2 prev_scale = {0};
Vector2 prev_window_size = {0};
bool has_panned = false;

int main(int argc, char **argv)
{
    /* Argv */

    if (argc <= 1) {
        fprintf(stderr, "ERROR: No expression provided\n");
        fprintf(stderr, "Usage: %s <expression>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Initialization */

    const char *expr = argv[1];
    MP_Env *parser = mp_init(expr);
    if (parser == NULL) {
        fprintf(stderr, "ERROR: Could not compile expression\n");
        return EXIT_FAILURE;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "cplot");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        Vector2 window_size = {
            .x = width,
            .y = height
        };

        /* Input */

        // Mouse drag camera movement
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            delta.x *= -1.0f;
            camera = Vector2Add(camera, delta);
        }

        // Keyboard camera movement
        if (IsKeyDown(KEY_W)) // Up
            camera.y += CAMERA_SPEED * dt;
        if (IsKeyDown(KEY_A)) // Left
            camera.x -= CAMERA_SPEED * dt;
        if (IsKeyDown(KEY_S)) // Down
            camera.y -= CAMERA_SPEED * dt;
        if (IsKeyDown(KEY_D)) // Right
            camera.x += CAMERA_SPEED * dt;
        if (IsKeyPressed(KEY_O)) { // Back to origin
            camera.x = 0.0f;
            camera.y = 0.0f;
            scale.x = ZOOM_DEFAULT;
            scale.y = ZOOM_DEFAULT;
            grid_spacing = GRID_SPACING_DEFAULT;
        }

        // Zoom
        if (GetMouseWheelMove() > 0.0f) {
            scale.x += ZOOM_FACTOR;
            scale.y += ZOOM_FACTOR;

            // Adapt grid spacing based on scaling
            if (is_near(fmod(scale.x, 100.0), 0.0))
                grid_spacing /= 2.0;
        }
        if (GetMouseWheelMove() < 0.0f) {
            scale.x = max(ZOOM_MIN, scale.x - ZOOM_FACTOR);
            scale.y = max(ZOOM_MIN, scale.y - ZOOM_FACTOR);

            // Adapt grid spacing based on scaling
            if (is_near(fmod(scale.x, 100.0), 0.0))
                grid_spacing *= 2.0;
        }

        // Handle movement events
        if (!Vector2Equals(prev_camera, camera)
            || !Vector2Equals(prev_scale, scale)
            || !Vector2Equals(prev_window_size, window_size)) {
            has_panned = true;
            prev_scale = scale;
            prev_camera = camera;
            prev_window_size = window_size;
        } else {
            has_panned = false;
        }

        // Resolution
        if (IsKeyPressed(KEY_R))
            resolution /= 2.0;
        if (IsKeyPressed(KEY_F))
            resolution *= 2.0;

        // Grid spacing
        if (IsKeyPressed(KEY_P))
            grid_spacing *= 2.0;
        if (IsKeyPressed(KEY_L))
            grid_spacing /= 2.0;

        // Toggles
        if (IsKeyPressed(KEY_C))
            toggle_continuous = !toggle_continuous;
        if (IsKeyPressed(KEY_B))
            toggle_debug_menu = !toggle_debug_menu;
        if (IsKeyPressed(KEY_G))
            toggle_grid = !toggle_grid;

        /* Rendering */

        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));

        // Visible range in cartesian coordinates
        double left = rpjx(0) - (double)width / 2.0;
        double right = rpjx(0) + (double)width / 2.0;
        double top = rpjy(0) + (double)height / 2.0;
        double bottom = rpjy(0) - (double)height / 2.0;

        left = floor(left / grid_spacing) * grid_spacing;
        right = ceil(right / grid_spacing) * grid_spacing; 
        top = ceil(top / grid_spacing) * grid_spacing;
        bottom = floor(bottom / grid_spacing) * grid_spacing;

        // Grid
        if (toggle_grid) {
            // Horizontal lines
            for (double y = bottom; y <= top; y += grid_spacing) {
                double y1 = pjy(y);
                DrawLine(0, y1, width, y1, GRID_COLOR);
            }

            // Vertical lines
            for (double x = left; x <= right; x += grid_spacing) {
                double x1 = pjx(x);
                DrawLine(x1, 0, x1, height, GRID_COLOR);
            }
        }

        // Axes
        DrawLine(0, pjy(0.0), width, pjy(0.0), AXES_COLOR); // x axis
        DrawLine(pjx(0.0), 0, pjx(0.0), height, AXES_COLOR); // y axis

        // Numbers on x axis
        for (double x = left; x <= right; x += grid_spacing) {
            int offset = 20;
            if (x < 0.0)
                offset += 10;

            if (is_near(x, 0.0))
                x = 0.0;

            DrawText(TextFormat("%.1f", x), pjx(x) - offset, pjy(0) + 5, 14,
                     NUMBER_COLOR);
        }

        // Numbers on y axis
        for (double y = bottom; y <= top; y += grid_spacing) {
            if (is_near(y, 0.0))
                continue;

            int offset = 20;
            if (y < 0.0)
                offset += 10;

            DrawText(TextFormat("%.1f", y), pjx(0.0) - offset, pjy(y), 14,
                     NUMBER_COLOR);
        }

        // Plot functions
        // plot(linear, GREEN, resolution);
        // plot(cubic, RED, resolution);
        // plot(sine, YELLOW, resolution);
        // plot(tangent, BLUE, resolution);
        // plot(asymptote1, PURPLE, resolution);
        // plot(asymptote2, WHITE, resolution);
        // plot(asymptote3, YELLOW, resolution);
        if (has_panned) {
            cache_count = plot_parser(parser, cache, CACHE_CAPACITY, resolution);
        }
        for (int i = 0; i < (int)cache_count - 1; ++i) {
            double y1 = cache[i].y;
            double y2 = cache[i + 1].y;

            double dy = y2 - y1;
            double dx = resolution;
            double slope = dy / dx;

            if (slope <= -ASYMPTOTE_TOLERANCE * 1.0 / resolution ||
                slope >= ASYMPTOTE_TOLERANCE * 1.0 / resolution) {
                DrawCircleLines(pjx(cache[i].x), pjy(0.0), ASYMPTOTE_POINT_RADIUS,
                                ASYMPTOTE_POINT_COLOR);
                continue;
            }

            if (toggle_continuous)
                DrawLineEx(pjv(cache[i].x, y1), pjv(cache[i].x + resolution, y2),
                           FUNCTION_LINE_THICKNESS, YELLOW);
            else
                DrawCircleV(pjv(cache[i].x, y1), 2.0f, YELLOW);
        }


        // Debug menu
        if (toggle_debug_menu) {
            const char *text = TextFormat(
                "Camera: x=%f y=%f\nScale: x=%f y=%f\n"
                "Resolution: %f\nGrid spacing: %f\nContinuous: %d\nGrid: %d",
                camera.x, camera.y, scale.x, scale.y,
                resolution, grid_spacing, toggle_continuous, toggle_grid);
            DrawText(text, 10, 10, 23, DEBUG_TEXT_COLOR);
        }

        // Mouse coordinates
        Vector2 mouse = GetMousePosition();
        DrawText(TextFormat("(%.2f ; %.2f)", rpjx(mouse.x), rpjy(mouse.y)),
                 // 30, height - 30, 20, WHITE);
                 mouse.x - 60.0f, mouse.y + 20.0f, 20, WHITE);

        EndDrawing();
    }

    mp_free(parser);
    CloseWindow();

    return EXIT_SUCCESS;
}

Vector2 pjv(double x, double y)
{
    return (Vector2){pjx(x), pjy(y)};
}

double pjx(double x)
{
    double w = GetScreenWidth();
    x = w/2.0 + x * scale.x;
    x -= camera.x;
    return x;
}

double pjy(double y)
{
    double h = GetScreenHeight();
    y = h/2.0 - y * scale.y;
    y += camera.y;
    return y;
}

Vector2 rpjv(double x, double y)
{
    return (Vector2){rpjx(x), rpjy(y)};
}

double rpjx(double x)
{
    double w = GetScreenWidth();
    x += camera.x;
    x += w/2.0 - w;
    x /= scale.x;
    return x;
}

double rpjy(double y)
{
    double h = GetScreenHeight();
    y -= camera.y;
    y += h/2.0 - h;
    y /= scale.y;
    return -y;
}


void plot(func_t f, Color color, double resolution)
{
    double x1 = rpjx(0.0);
    double x2 = rpjx(GetScreenWidth());

    for (double x = x1; x <= x2; x += resolution) {
        double y1 = f(x);
        double y2 = f(x + resolution);

        double dy = y2 - y1;
        double dx = resolution;
        double slope = dy / dx;

        if (slope <= -ASYMPTOTE_TOLERANCE * 1.0 / resolution ||
            slope >= ASYMPTOTE_TOLERANCE * 1.0 / resolution) {
            DrawCircleLines(pjx(x), pjy(0.0), ASYMPTOTE_POINT_RADIUS,
                            ASYMPTOTE_POINT_COLOR);
            continue;
        }

        if (toggle_continuous)
            DrawLineEx(pjv(x, y1), pjv(x + resolution, y2),
                       FUNCTION_LINE_THICKNESS, color);
        else
            DrawCircleV(pjv(x, y1), 2.0f, color);
    }
}

size_t plot_parser(MP_Env *parser, Vector2 *buf, size_t buf_size, double resolution)
{
    double x1 = rpjx(0.0);
    double x2 = rpjx(GetScreenWidth());

    size_t point_count = 0;
    for (double x = x1; x <= x2 && point_count < buf_size; x += resolution) {
        mp_variable(parser, 'x', x);
        buf[point_count].x = x;
        buf[point_count].y = mp_evaluate(parser).value;
        ++point_count;
    }

    return point_count;
}

double max(double a, double b)
{
    return a > b ? a : b;
}

double map(double value, double x1, double x2, double y1, double y2)
{
    if (fabs(x2 - x1) < EPSILON)
        return (y1 + y2) / 2.0;

    return (value - x1) / (x2 - x1) * (y2 - y1) + y1;
}

bool is_near(double x, double target)
{
    return (target - EPSILON < x) && (x < target + EPSILON);
}

double cubic(double x)
{
    return x*x*x - 3*x*x + 4;
}

double linear(double x)
{
    return 2*x - 3;
}

double sine(double x)
{
    return sin(x);
}

double tangent(double x)
{
    return tan(x);
}

double asymptote1(double x)
{
    return (2*x + 1) / (x*x - 4);
}

double asymptote2(double x)
{
    return (x*x*x - 2*x + 1) / (x*x - 1);
}

double asymptote3(double x)
{
    return (x*x + 1) / ((x*x - 1) * (x - 3));
}
