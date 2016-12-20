#include <stdlib.h>
#include "SDL.h"

#ifdef main
#undef main
#endif

#define USE_BACK_TEXTURE 0

void InitVideo();
void InitData();
void UpdateData();
void CleanUp();
bool Idle();
void DrawAll();
void DrawTexture();
void DrawPoints();
void DrawLines();
void DrawEmptyRectangles();
void DrawFillRectangles();

[[noreturn]] void ProgramDie(const char* msg = NULL);

//--------------------------------------------------
// MAIN
//--------------------------------------------------
int main(int argc, char *argv[])
{
    InitVideo();
    InitData();

    while (Idle());

    CleanUp();

    return 0;
}
//--------------------------------------------------

enum Test_t
{
    DRAW_TEXTURE,
    DRAW_POINTS,
    DRAW_LINES,
    DRAW_EMPTY_RECTANGLES,
    DRAW_FILL_RECTANGLES,
    DRAW_MAX_CASE
};

struct Texture
{
private:
    SDL_Texture* texture = nullptr;
    SDL_Renderer* renderer = nullptr;
    int w;
    int h;
    int xCount;
    int yCount;

public:
    Texture(const char* _name, SDL_Renderer* _renderer, int _xcount, int _ycount)
        : renderer(_renderer)
        , xCount(_xcount)
        , yCount(_ycount)
    {
        if ((xCount == 0) || (yCount == 0))
            ProgramDie("DrawTexture: wrong texture count");

        SDL_Surface* sfc = SDL_LoadBMP(_name);
        SDL_SetColorKey(sfc, 1, SDL_MapRGB(sfc->format, 0, 255, 0));
        texture = SDL_CreateTextureFromSurface(renderer, sfc);
        w = sfc->w / xCount;
        h = sfc->h / yCount;
        SDL_FreeSurface(sfc);
    }

    ~Texture()
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    int GetWidth() const { return w; };
    int GetHeight() const { return h; };
    int GetXCount() const { return xCount; }
    int GetYCount() const { return yCount; }

    void DrawTexture(SDL_Rect* dst, int xIndex, int yIndex, float angle)
    {
        if (xIndex >= xCount || yIndex >= yCount)
            ProgramDie("DrawTexture: wrong texture indexes");

        SDL_Rect src = { xIndex * w / xCount, yIndex * h / yCount, w, h };
        SDL_SetTextureBlendMode(texture, ((rand() % 10) ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_ADD));
        SDL_RenderCopyEx(renderer, texture, &src, dst, angle, NULL, SDL_FLIP_NONE);
    }

    SDL_Texture* GetTexture() { return texture; };
};

const int WINDOW_W = 1820;
const int WINDOW_H = 980;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* back_texture = nullptr;
Texture* texture = nullptr;
Texture* texture_atlas = nullptr;
SDL_Color* COLORS;
SDL_Rect* DESTINATIONS;
float* ANGLES;
int BATCH_SIZE;
bool DRAW_BATCH = false;
Test_t TEST_CASE = DRAW_TEXTURE;
SDL_Point* POINTS;

void InitVideo()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        ProgramDie("Cannon init video");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    window = SDL_CreateWindow("TEST", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (!window)
        ProgramDie();
    const int renderer_driver_index = 3;
    renderer = SDL_CreateRenderer(window, renderer_driver_index, SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_PRESENTVSYNC*/);
    if (!renderer)
        ProgramDie();

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Renderer: %s", info.name);

#if USE_BACK_TEXTURE
    back_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WINDOW_W, WINDOW_H);
    SDL_SetRenderTarget(renderer, back_texture);
#endif
}

void InitData()
{
    texture = new Texture("awesomeface.bmp", renderer, 1, 1);
    //texture_atlas = new Texture("awesomeface_atlas.bmp", renderer, 2, 1);
    texture_atlas = texture;

    int w = texture_atlas->GetWidth() / 10;
    int h = texture_atlas->GetHeight() / 10;
    int iCount = WINDOW_H / h;
    int jCount = WINDOW_W / w;
    int x = 10;
    int y = 10;
    int cx = texture_atlas->GetXCount();
    int cy = texture_atlas->GetYCount();
    const int dst_count = iCount * jCount;
    const int src_count = dst_count;// 1;
    BATCH_SIZE = dst_count;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Batch count = %d", BATCH_SIZE);

    DESTINATIONS = (SDL_Rect*)SDL_malloc(sizeof(SDL_Rect) * dst_count);
    COLORS = (SDL_Color*)SDL_malloc(sizeof(SDL_Color) * dst_count);
    ANGLES = (float*)SDL_malloc(sizeof(float) * dst_count);
    POINTS = (SDL_Point*)SDL_malloc(BATCH_SIZE * sizeof(SDL_Point));

    SDL_Rect* local_dst = DESTINATIONS;
    SDL_Color* local_color = COLORS;
    float* local_angles = ANGLES;
    SDL_Point* point = POINTS;

    for (int i = 0; i < iCount; ++i)
    {
        for (int j = 0; j < jCount; ++j)
        {
            local_dst->x = x;
            local_dst->y = y;
            local_dst->w = w;
            local_dst->h = h;

            *local_angles = 0.0f;
            *local_color = { 255, 255, 255, 255 };
            //*local_angles = (float)(rand() % 360);
            //*local_color = { Uint8(rand() % 255), Uint8(rand() % 255), Uint8(rand() % 255), Uint8(rand() % 255) };
            point->x = x;
            point->y = y;

            x += w;

            ++local_dst;
            ++local_angles;
            ++local_color;
            ++point;
        }
        x = 10;
        y += h;
    }
}

void UpdateData()
{
    return;
    for (int i = 0; i < BATCH_SIZE; ++i)
    {
        ANGLES[i] += (rand() % 100 ) / 100.0f;
        COLORS[i].r += rand() % 2;
        COLORS[i].g += rand() % 2;
        COLORS[i].b += rand() % 2;
        COLORS[i].a += rand() % 2;
//         DESTINATIONS[i].x += rand() % 5 - 2;
//         DESTINATIONS[i].y += rand() % 5 - 2;

        if (DESTINATIONS[i].x < 0)
            DESTINATIONS[i].x = 0;
        if (DESTINATIONS[i].x >= WINDOW_W)
            DESTINATIONS[i].x = WINDOW_W - DESTINATIONS[i].w - 1;
        if (DESTINATIONS[i].y < 0)
            DESTINATIONS[i].y = 0;
        if (DESTINATIONS[i].y >= WINDOW_H)
            DESTINATIONS[i].y = WINDOW_H - DESTINATIONS[i].h - 1;
    }
}

void CleanUp()
{
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}

bool Idle()
{
    SDL_Event event;
    const int MAX_EVENTS = 100;
    SDL_Event events_array[MAX_EVENTS + 1];

    while (true)
    {
        UpdateData();
        DrawAll();
        if (!SDL_WaitEventTimeout(&event, 0))
            continue;

        int count = 0;
        events_array[0] = event;
        count = SDL_PeepEvents(events_array + 1, MAX_EVENTS, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);

        for (int i = 0; i < count + 1; ++i)
        {
            auto& ev = events_array[i];
            switch (ev.type)
            {
            case SDL_QUIT:
                return false;
            case SDL_MOUSEBUTTONDOWN:     
                DRAW_BATCH = !DRAW_BATCH;
                if (DRAW_BATCH)
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Switch to batch drawing");
                else
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Switch to single drawing");
                break;
            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_SPACE)
                {
                    TEST_CASE = Test_t(TEST_CASE + 1);
                    if (TEST_CASE == DRAW_MAX_CASE)
                        TEST_CASE = DRAW_TEXTURE;
                }
                break;
            }
        }

    }
}

void DrawAll()
{
    static int frames = 0;
    frames++;
    static auto last_tick = SDL_GetTicks();
    auto current_tick = SDL_GetTicks();
    if ((current_tick - last_tick) >= 1000)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "FPS: %d", frames);
        last_tick = current_tick;
        frames = 0;
    }    
    
#if !USE_BACK_TEXTURE
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
#endif
    switch (TEST_CASE)
    {
    case DRAW_TEXTURE:
        DrawTexture();
        //DrawPoints();
        break;
    case DRAW_POINTS:
        DrawPoints();
        break;
    case DRAW_LINES:
        DrawLines();
        break;
    case DRAW_FILL_RECTANGLES:
        DrawFillRectangles();
        break;
    case DRAW_EMPTY_RECTANGLES:
        DrawEmptyRectangles();
        break;
    default:
        break;
    }

#if USE_BACK_TEXTURE
    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, back_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, back_texture);
    SDL_RenderClear(renderer);
#else
    SDL_RenderPresent(renderer);
#endif

}

void DrawTexture()
{
    for (int i = 0; i < BATCH_SIZE; ++i)
    {
        SDL_SetTextureColorMod(texture->GetTexture(), COLORS[i].r, COLORS[i].g, COLORS[i].b);
        SDL_SetTextureAlphaMod(texture->GetTexture(), COLORS[i].a);
        texture->DrawTexture(&DESTINATIONS[i], 0, 0, ANGLES[i]);
    }
}

void DrawPoints()
{
    if (DRAW_BATCH)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawPoints(renderer, POINTS, BATCH_SIZE);
    }
    else
    {
        for (int i = 0; i < BATCH_SIZE; ++i)
        {
            SDL_SetRenderDrawColor(renderer, COLORS[i].r, COLORS[i].g, COLORS[i].b, COLORS[i].a);
            SDL_RenderDrawPoint(renderer, POINTS[i].x, POINTS[i].y);
        }
    }
}

void DrawLines()
{
    if (DRAW_BATCH)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLines(renderer, POINTS, BATCH_SIZE);
    }
    else
    {
        for (int i = 0; i < BATCH_SIZE - 1; ++i)
        {
            SDL_SetRenderDrawColor(renderer, COLORS[i].r, COLORS[i].g, COLORS[i].b, COLORS[i].a);
            SDL_RenderDrawLine(renderer, POINTS[i].x, POINTS[i].y, POINTS[i+1].x, POINTS[i+1].y);
        }
    }
}

void DrawFillRectangles()
{
    if (DRAW_BATCH)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRects(renderer, DESTINATIONS, BATCH_SIZE);
    }
    else
    {
        for (int i = 0; i < BATCH_SIZE; ++i)
        {
            SDL_SetRenderDrawColor(renderer, COLORS[i].r, COLORS[i].g, COLORS[i].b, COLORS[i].a);
            SDL_RenderFillRect(renderer, &DESTINATIONS[i]);
        }
    }
}

void DrawEmptyRectangles()
{
    if (DRAW_BATCH)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRects(renderer, DESTINATIONS, BATCH_SIZE);
    }
    else
    {
        for (int i = 0; i < BATCH_SIZE; ++i)
        {
            SDL_SetRenderDrawColor(renderer, COLORS[i].r, COLORS[i].g, COLORS[i].b, COLORS[i].a);
            SDL_RenderDrawRect(renderer, &DESTINATIONS[i]);
        }
    }
}

void ProgramDie(const char* msg)
{
    auto sdl_msg = SDL_GetError();
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL: %s", sdl_msg);
    if (msg)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "USER: %s", msg);
    }
    CleanUp();
    system("pause");
    exit(-1);
}