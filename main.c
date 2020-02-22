#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#define DIST 200
const size_t TEST_DIST = DIST * DIST;
int TEST_STEPS = 512;

uint8_t* red = NULL;
uint8_t* green = NULL;
uint8_t* blue = NULL;

#define MIN_X -2.1
#define MAX_X 0.67
#define MIN_Y (-MAX_Y)
#define MAX_Y (((MAX_X - MIN_X) * ((double)HEIGHT / WIDTH)) / 2)

#define MOVE_PRECISION 10

//#define FULLSCREEN

#define WIDTH 1000
#define HEIGHT 600

SDL_Window* gWindow;
SDL_Renderer* gRenderer;
SDL_Texture* gScreen;
SDL_Texture* gTemp;

struct DoubleSelection {
    double minX;
    double minY;
    double maxX;
    double maxY;
};

struct IntSelection {
    int minX;
    int minY;
    int maxX;
    int maxY;
};

typedef struct {
    double re;
    double im;
} Complex;

enum Response { RESP_QUIT, RESP_UP, RESP_DOWN, RESP_LEFT, RESP_RIGHT, RESP_ZOOM_IN, 
                RESP_ZOOM_OUT, RESP_RESET, RESP_NONE, RESP_EVOLVE, RESP_DEGENERATE,
                RESP_JUMP_UP, RESP_JUMP_DOWN };

void init_colors() {
    free(red);
    free(green);
    free(blue);
    red = (uint8_t*)malloc(TEST_STEPS);
    green = (uint8_t*)malloc(TEST_STEPS);
    blue = (uint8_t*)malloc(TEST_STEPS);
    for(size_t i = 0; i < TEST_STEPS; i++) {
        double angle = M_PI * 2 / TEST_STEPS * i + 3.7;
        red[i] = sin(M_PI_2 * (sin(angle) + 1) / 2) * 0xFF;
        green[i] = sin(M_PI_2 * (sin(angle + M_PI_2) + 1) / 2) * 0xFF;
        blue[i] = sin(M_PI_2 * (sin(angle + M_PI) + 1) / 2) * 0xFF;
    }
}

void init() {
    SDL_Init(SDL_INIT_VIDEO);
    gWindow = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
#ifdef FULLSCREEN
    SDL_SetWindowFullscreen(gWindow, SDL_WINDOW_FULLSCREEN);
#endif
    gRenderer= SDL_CreateRenderer(gWindow, -1, 0);
    gScreen = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 
        WIDTH, HEIGHT);
    gTemp = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 
        WIDTH, HEIGHT);
    init_colors();
}

void quit() {
    SDL_DestroyTexture(gScreen);
    SDL_DestroyTexture(gTemp);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
}

void square(const Complex* comp, Complex* res) {
    res->re = (comp->re - comp->im) * (comp->re + comp->im);
    res->im = comp->re * comp->im;
    res->im += res->im;
}

void add(Complex* res, const Complex* added) {
    res->re += added->re;
    res->im += added->im;
}

double find_distance(const Complex* comp2) {
    return comp2->re * comp2->re + comp2->im * comp2->im;
}

int count_steps(const Complex* comp) {
    size_t res = 0;
    Complex temp1, temp2;
    temp1.re = temp1.im = 0;
    do {
        square(&temp1, &temp2);
        add(&temp2, comp);
        temp1 = temp2;
        res++;
    } while(res < TEST_STEPS && find_distance(&temp2) < TEST_DIST);
    return res;
}

void present() {
    SDL_SetRenderTarget(gRenderer, gScreen);
    SDL_RenderCopy(gRenderer, gTemp, NULL, NULL);
    SDL_SetRenderTarget(gRenderer, NULL);
    SDL_RenderCopy(gRenderer, gTemp, NULL, NULL);
    SDL_RenderPresent(gRenderer);
    SDL_SetRenderTarget(gRenderer, gTemp);
}

void draw(const struct DoubleSelection* ds, const struct IntSelection* is, const bool pres) {
    int width = is->maxX - is->minX;
    int height = is->maxY - is->minY;
    double hUnit = (ds->maxX - ds->minX) / (WIDTH / (is->maxX - is->minX)) / width;
    double vUnit = (ds->maxY - ds->minY) / (HEIGHT / (is->maxY - is->minY)) / height;
    Complex comp;
    for(unsigned i = is->minY; i < is->maxY; i++) {
        comp.im = ds->maxY - i * vUnit;
        for(unsigned j = is->minX; j < is->maxX; j++) {
            comp.re = ds->minX + j * hUnit;
            size_t steps = count_steps(&comp);
            if(steps != TEST_STEPS)
                SDL_SetRenderDrawColor(gRenderer, red[steps], green[steps], blue[steps], 0xFF);
            else
                SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);
            SDL_RenderDrawPoint(gRenderer, j, i);
            
        }
        if(pres) 
            present();
    }
}

void set_ds(struct DoubleSelection* ds, const double minX, const double minY, const double maxX, 
        const double maxY) {
    ds->minX = minX;
    ds->minY = minY;
    ds->maxX = maxX;
    ds->maxY = maxY;
}

void set_is(struct IntSelection* is, const int minX, const int minY, const int maxX,
        const int maxY) {
    is->minX = minX;
    is->minY = minY;
    is->maxX = maxX;
    is->maxY = maxY;
}

enum Response handle_input() {
    SDL_Event e;
    SDL_WaitEvent(&e);
    static bool input = true;
    switch(e.type) {
    case SDL_QUIT: return RESP_QUIT;
    case SDL_KEYDOWN:
        if(input) {
            input = false;
            switch(e.key.keysym.sym) {
                case SDLK_UP: return RESP_UP;
                case SDLK_DOWN: return RESP_DOWN;
                case SDLK_LEFT: return RESP_LEFT;
                case SDLK_RIGHT: return RESP_RIGHT;
                case SDLK_PAGEUP:
                case SDLK_z:
                    return RESP_ZOOM_IN;
                case SDLK_PAGEDOWN:
                case SDLK_x:
                    return RESP_ZOOM_OUT;
                case SDLK_SPACE: return RESP_RESET;
                case SDLK_q: return RESP_DEGENERATE;
                case SDLK_w: return RESP_EVOLVE;
                case SDLK_a: return RESP_JUMP_DOWN;
                case SDLK_s: return RESP_JUMP_UP;
                case SDLK_ESCAPE:
                    return RESP_QUIT;
                default: return RESP_NONE;
            }
        }
        break;
        case SDL_KEYUP:
            input = true;
            return RESP_NONE;
    }
    return RESP_NONE;
}

void move_up(struct DoubleSelection* ds, struct IntSelection* is, double dStep, int iStep) {
    SDL_SetRenderTarget(gRenderer, gTemp);
    SDL_Rect dst = {0, iStep, WIDTH, HEIGHT};
    SDL_RenderCopy(gRenderer, gScreen, NULL, &dst);
    ds->minY += dStep;
    ds->maxY += dStep;
    set_is(is, 0, 0, WIDTH, iStep);
    draw(ds, is, false);
}

void move_down(struct DoubleSelection* ds, struct IntSelection* is, double dStep, int iStep) {
    SDL_SetRenderTarget(gRenderer, gTemp);
    SDL_Rect dst = {0, -iStep, WIDTH, HEIGHT};
    SDL_RenderCopy(gRenderer, gScreen, NULL, &dst);
    ds->minY -= dStep;
    ds->maxY -= dStep;
    set_is(is, 0, HEIGHT - iStep, WIDTH, HEIGHT);
    draw(ds, is, false);
}

void move_left(struct DoubleSelection* ds, struct IntSelection* is, double dStep, int iStep) {
    SDL_SetRenderTarget(gRenderer, gTemp);
    SDL_Rect dst = {iStep, 0, WIDTH, HEIGHT};
    SDL_RenderCopy(gRenderer, gScreen, NULL, &dst);
    ds->minX -= dStep;
    ds->maxX -= dStep;
    set_is(is, 0, 0, iStep, HEIGHT);
    draw(ds, is, false);
}

void move_right(struct DoubleSelection* ds, struct IntSelection* is, double dStep, int iStep) {
    SDL_SetRenderTarget(gRenderer, gTemp);
    SDL_Rect dst = {-iStep, 0, WIDTH, HEIGHT};
    SDL_RenderCopy(gRenderer, gScreen, NULL, &dst);
    ds->minX += dStep;
    ds->maxX += dStep;
    set_is(is, WIDTH - iStep, 0, WIDTH, HEIGHT);
    draw(ds, is, false);
}

void zoom_in(struct DoubleSelection* ds, struct IntSelection* is, double* dhStep, double* dvStep,
        int* ihStep, int* ivStep) {
    SDL_SetRenderTarget(gRenderer, gTemp);
    ds->minX += *dhStep;
    ds->minY += *dvStep;
    ds->maxX -= *dhStep;
    ds->maxY -= *dvStep;
    set_is(is, 0, 0, WIDTH, HEIGHT);
    draw(ds, is, true);
    *dhStep = (ds->maxX - ds->minX) / MOVE_PRECISION;
    *dvStep = (ds->maxY - ds->minY) / MOVE_PRECISION;
    *ihStep = WIDTH / MOVE_PRECISION;
    *ivStep = HEIGHT / MOVE_PRECISION;
}

void zoom_out(struct DoubleSelection* ds, struct IntSelection* is, double* dhStep, double* dvStep,
        int* ihStep, int* ivStep) {
    SDL_SetRenderTarget(gRenderer, gTemp);
    ds->minX -= *dhStep;
    ds->minY -= *dvStep;
    ds->maxX += *dhStep;
    ds->maxY += *dvStep;
    set_is(is, 0, 0, WIDTH, HEIGHT);
    draw(ds, is, true);
    *dhStep = (ds->maxX - ds->minX) / MOVE_PRECISION;
    *dvStep = (ds->maxY - ds->minY) / MOVE_PRECISION;
    *ihStep = WIDTH / MOVE_PRECISION;
    *ivStep = HEIGHT / MOVE_PRECISION;
}

void redraw(struct DoubleSelection* ds, struct IntSelection* is) {
    SDL_SetRenderTarget(gRenderer, gTemp);
    if(!is) {
        struct IntSelection is1;
        set_is(&is1, 0, 0, WIDTH, HEIGHT);
        draw(ds, &is1, true);
    } else
        draw(ds, is, true);
    present();
}

void reset(struct DoubleSelection* ds, struct IntSelection* is, double* dhStep, double* dvStep,
        int* ihStep, int* ivStep) {
    set_ds(ds, MIN_X, MIN_Y, MAX_X, MAX_Y);
    set_is(is, 0, 0, WIDTH, HEIGHT);
    SDL_SetRenderTarget(gRenderer, gTemp);
    draw(ds, is, false);
    present();
    *dhStep = (ds->maxX - ds->minX) / MOVE_PRECISION;
    *dvStep = (ds->maxY - ds->minY) / MOVE_PRECISION;
    *ihStep = WIDTH / MOVE_PRECISION;
    *ivStep = HEIGHT / MOVE_PRECISION;
}

void loop(struct DoubleSelection* ds, struct IntSelection* is, double* dhStep, double* dvStep, 
        int* ihStep, int* ivStep) {
    while(true) {
        switch(handle_input()) {
            case RESP_UP: 
                move_up(ds, is, *dvStep, *ivStep);
                break;
            case RESP_DOWN:
                move_down(ds, is, *dvStep, *ivStep);
                break;
            case RESP_LEFT:
                move_left(ds, is, *dhStep, *ihStep);
                break;
            case RESP_RIGHT:
                move_right(ds, is, *dhStep, *ihStep);
                break;
            case RESP_ZOOM_IN:
                zoom_in(ds, is, dhStep, dvStep, ihStep, ivStep);
                break;
            case RESP_ZOOM_OUT:
                zoom_out(ds, is, dhStep, dvStep, ihStep, ivStep);
                break;
            case RESP_JUMP_UP:
                TEST_STEPS *= 2;
                init_colors();
                redraw(ds, NULL);
                break;
            case RESP_JUMP_DOWN:
                if(TEST_STEPS) 
                    TEST_STEPS /= 2;
                init_colors();
                redraw(ds, NULL);
                break;
            case RESP_EVOLVE:
                TEST_STEPS++;
                init_colors();
                redraw(ds, NULL);
                break;
            case RESP_DEGENERATE:
                if(TEST_STEPS) 
                    TEST_STEPS--;
                init_colors();
                redraw(ds, NULL);
                break;
            case RESP_RESET:
                reset(ds, is, dhStep, dvStep, ihStep, ivStep);
                break;
            case RESP_QUIT: return;
            case RESP_NONE: break;
        }
        present();
    }
}

void proceed() {
    struct DoubleSelection ds;
    struct IntSelection is;
    double dhStep, dvStep;
    int ihStep, ivStep;
    reset(&ds, &is, &dhStep, &dvStep, &ihStep, &ivStep);
    loop(&ds, &is, &dhStep, &dvStep, &ihStep, &ivStep);
}

int main() {
    init();
    proceed();
    quit();
    return 0;
}
