#include <SDL2/SDL.h>
#include <iostream>
#include <complex>
#include <tuple>
#include <thread>
#include <vector>
#include <math.h>
#include <mutex>

typedef struct {
    double r;       // ∈ [0, 1]
    double g;       // ∈ [0, 1]
    double b;       // ∈ [0, 1]
} rgb;

typedef struct {
    double h;       // ∈ [0, 360]
    double s;       // ∈ [0, 1]
    double v;       // ∈ [0, 1]
} hsv;

rgb hsv2rgb(double H, double S, double V)
{
    rgb RGB;
    double  P, Q, T,
            fract;

    (H == 360.)?(H = 0.):(H /= 60.);
    fract = H - floor(H);

    P = V*(1. - S);
    Q = V*(1. - S*fract);
    T = V*(1. - S*(1. - fract));

    if      (0. <= H && H < 1.)
        RGB = (rgb){.r = V, .g = T, .b = P};
    else if (1. <= H && H < 2.)
        RGB = (rgb){.r = Q, .g = V, .b = P};
    else if (2. <= H && H < 3.)
        RGB = (rgb){.r = P, .g = V, .b = T};
    else if (3. <= H && H < 4.)
        RGB = (rgb){.r = P, .g = Q, .b = V};
    else if (4. <= H && H < 5.)
        RGB = (rgb){.r = T, .g = P, .b = V};
    else if (5. <= H && H < 6.)
        RGB = (rgb){.r = V, .g = P, .b = Q};
    else
        RGB = (rgb){.r = 0., .g = 0., .b = 0.};

    return RGB;
}

SDL_Window* gWindow;
SDL_Renderer* gRenderer;
float RE_START = -2;
float RE_END = 1;
float IM_START = -1;
float IM_END = 1;

float zoomPosScalar = 1/1.02;
float zoomNegScalar = 1.02;

float moveAmount = 0.03;

int threads = 8;


constexpr int WIDTH = 1000;
constexpr int HEIGHT = 1000;

constexpr int iter = 100;
bool quit = false;
bool refresh = true;

struct coordinateData
{
    long double x;
    long double y;
    SDL_Color color;
};


void colorPixel(int x, int y, SDL_Color c) {
    SDL_Rect rect = SDL_Rect();
    rect.h = 1;
    rect.w = 1;
    rect.x = x;
    rect.y = y;
    SDL_SetRenderDrawColor(gRenderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(gRenderer, &rect);
}

std::tuple<long double, long double> translate(long double x, long double y) {
    x = RE_START + (x / WIDTH) * (RE_END - RE_START);
    y = IM_START + (y / HEIGHT) * (IM_END - IM_START);
    return std::tuple<long double, long double>(x, y);
}

SDL_Color iterate(long double x, long double y) {
    auto c_coords = translate(x, y);
    
    auto z = std::complex<long double>(0,0);
    auto c = std::complex<long double>(std::get<0>(c_coords), std::get<1>(c_coords));

    int n = 0;

    while(abs(z) <= 2 && n < iter) {
        z = (z*z)+c;
        n++;
    }

    auto col = SDL_Color();

    int hue = 360 * n / iter;
    int saturation = 1;
    int value = n < iter ? 1 : 0;
    auto rgb = hsv2rgb(hue, saturation, value);
    col.r = rgb.r * 255;
    col.b = rgb.b * 255;
    col.g = rgb.g * 255;
    col.a = 255;

    //    col.r = 0*n/iter;
    //    col.g = 255*n/iter;
    //    col.b = 255*n/iter;
    //    col.a = 255;
    return col;
}
int sectionWidth = WIDTH / threads;
std::vector<coordinateData> colors;
std::mutex vecLock;

void getColors(int from, int to) {
    for(long double x = from; x < to; x++) {
        for(long double y = 0; y < HEIGHT; y++) {
            auto color = iterate(x, y);
            vecLock.lock();
            colors.push_back(coordinateData{.x=x, .y=y, .color=color});
            vecLock.unlock();
        }
    }
}

void draw() {
    colors.clear();
    std::vector<std::thread*> threadContainer;

    for(int i = 0; i<threads; i++) {
        int start = sectionWidth * i;
        int end = sectionWidth * (i+1);
        auto t = new std::thread(getColors, start, end);
        threadContainer.push_back(t);
    }
    for(int i = 0; i<threads; i++) {
        threadContainer[i]->join();
        delete threadContainer[i];
    }
    for (auto color: colors) {
        colorPixel(color.x, color.y, color.color);
    }
}
void input() {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) {
            quit = true;
            return;
        }

        if(e.type == SDL_MOUSEBUTTONDOWN) {
            switch(e.button.button) {
                case SDL_BUTTON_LEFT:
                    RE_START *= zoomPosScalar;
                    IM_START *= zoomPosScalar;
                    RE_END *= zoomPosScalar;
                    IM_END *= zoomPosScalar;
                    refresh = true;
                    break;
                case SDL_BUTTON_RIGHT:
                    RE_START *= zoomNegScalar;
                    IM_START *= zoomNegScalar;
                    RE_END *= zoomNegScalar;
                    IM_END *= zoomNegScalar;
                    refresh = true;
                    break;
            }
        } else if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_w:
                    IM_START -= moveAmount;
                    IM_END -= moveAmount;
                    refresh = true;
                    break;
                case SDLK_s:
                    IM_START += moveAmount;
                    IM_END += moveAmount;
                    refresh = true;
                    break;
                case SDLK_d:
                    RE_START += moveAmount;
                    RE_END += moveAmount;
                    refresh = true;
                    break;
                case SDLK_a:
                    RE_START -= moveAmount;
                    RE_END -= moveAmount;
                    refresh = true;
                    break;
                case SDLK_q:
                    quit = true;
                    break;
                case SDLK_ESCAPE:
                    quit = true;
                    break;
            }
        }

    };
    
}
int main() {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {        
        std::cout << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_SHOWN, &gWindow, &gRenderer);

    while(!quit) {
        if(refresh) {
            draw();
            SDL_RenderPresent(gRenderer);
            refresh = false;
        }
        input();
    }
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
}  