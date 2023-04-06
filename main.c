#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <time.h>
#include <string.h>

#include <unistd.h>
#include <X11/Xlib.h>
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <X11/extensions/Xdbe.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define WIDTH 800
#define HEIGHT 800

#define abs(value) (value<0?value*-1:value)

double xrot = 0;
double yrot = 0;
double zrot = 0;

uint32_t decodeRGB(int r, int g, int b)
{
    return (r << 16) + (g << 8) + b;
}

void put_pixel(char *memory, int x, int y, uint32_t color)
{
    if(!(y >= HEIGHT || y < 0 || x >= WIDTH || x < 0)) {
        uint32_t *pixel = (uint32_t *) memory;
        *(pixel + y * WIDTH + x) = color;
    }
}

void drawLine(char *memory, int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    double Xinc = dx / (float) steps;
    double Yinc = dy / (float) steps;

    double X = x0;
    double Y = y0;
    for(int i = 0; i <= steps; ++i) {
        put_pixel(memory, (int) X, (int) Y, color);
        X += Xinc;
        Y += Yinc;
    }
}

void fillOval(char *memory, int cx, int cy, int a, int b, uint32_t color)
{
    for(int y = -b; y <= b; ++y) {
        for(int x = -a; x <= a; ++x) {
            int L = x*x*b*b + y*y*a*a;
            int R = b*b*a*a;
            if(L <= R) {
                put_pixel(memory, cx + x, cy + y, color);
            }
        }
    }
}

void fillMemory(char *memory, uint32_t color)
{
    for(int y = 0; y < HEIGHT; ++y) {
        for(int x = 0; x < HEIGHT; ++x) {
            put_pixel(memory, x, y, color);
        }
    }
}

double calculateX(int i, int j, int k, double A, double B, double C) {
    return j*sin(A)*sin(B)*cos(C) - k*cos(A)*sin(B)*cos(C) + j*cos(A)*sin(C) + k*sin(A)*sin(C) + i*cos(B)*cos(C);
}

double calculateY(int i, int j, int k, double A, double B, double C) {
    return j*cos(A)*cos(C) + k*sin(A)*cos(C) - j*sin(A)*sin(B)*sin(C) + k*cos(A)*sin(B)*sin(C) - i*cos(B)*sin(C);
}

double calculateZ(int i, int j, int k, double A, double B) {
    return k*cos(A)*cos(B) - j*sin(A)*cos(B) + i*sin(B);
}

int ppoints[8][2] = {};

double cubepoints[8][3][1] = {
    {{-1}, {-1}, { 1}},
    {{ 1}, {-1}, { 1}},
    {{ 1}, { 1}, { 1}},
    {{-1}, { 1}, { 1}},
    {{-1}, {-1}, {-1}},
    {{ 1}, {-1}, {-1}},
    {{ 1}, { 1}, {-1}},
    {{-1}, { 1}, {-1}}
};

void render_cube(char *memory, int cx, int cy, int scale)
{
    for(int i = 0; i < 8; ++i) {
        double result[3][1] = {
            {calculateX(cubepoints[i][0][0], cubepoints[i][1][0], cubepoints[i][2][0], xrot, yrot, zrot)},
            {calculateY(cubepoints[i][0][0], cubepoints[i][1][0], cubepoints[i][2][0], xrot, yrot, zrot)},
            {calculateZ(cubepoints[i][0][0], cubepoints[i][1][0], cubepoints[i][2][0], xrot, yrot)}
        };

        double distance = 5;
        double z = 1.0/(distance - result[2][0]);

        int px = (int) (z * result[0][0] * scale + cx);
        int py = (int) (z * result[1][0] * scale + cy);

        ppoints[i][0] = px;
        ppoints[i][1] = py;
    }

    for(int m = 0; m < 4; ++m) {
        drawLine(memory, ppoints[m][0], ppoints[m][1], ppoints[(m+1)%4][0], ppoints[(m+1)%4][1], decodeRGB(255, 255, 255));
        drawLine(memory, ppoints[m+4][0], ppoints[m+4][1], ppoints[(m+1)%4 + 4][0], ppoints[(m+1)%4 + 4][1], decodeRGB(255, 255, 255));
        drawLine(memory, ppoints[m][0], ppoints[m][1], ppoints[m+4][0], ppoints[m+4][1], decodeRGB(255, 255, 255));
    }
}

int main(void)
{
    Display *display = XOpenDisplay(NULL);

    int screen = DefaultScreen(display);

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0,
        WIDTH, HEIGHT,
        0, 0,
        0x00000000
    );
    
    XWindowAttributes win_attr = {0};
    XGetWindowAttributes(display, window, &win_attr);

    
    char *memory = (char *) malloc(WIDTH*HEIGHT*4);

    if((void *) memory == NULL) exit(-1);
    

    XImage *image = XCreateImage(
        display,
        win_attr.visual,
        win_attr.depth,
        ZPixmap,
        0,
        memory,
        WIDTH,
        HEIGHT,
        32,
        WIDTH*4
    );

    GC graphics = XCreateGC(display, window, 0, NULL);

    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delete_window, 1);

    XSelectInput(display, window, KeyPressMask);

    XMapWindow(display, window);

    XSync(display, False);

    int exit = 0;
    XEvent event;

    while(!exit) {
        while(XPending(display) > 0) {
            XNextEvent(display, &event);
            if(event.type == ClientMessage) {
                if((Atom) event.xclient.data.l[0] == delete_window) {
                    exit = 1;   
                }
            }
        }

        fillMemory(memory, decodeRGB(0, 0, 0));
        render_cube(memory, WIDTH/2, HEIGHT/2, 1000);
        
        XPutImage(
            display,
            window,
            graphics,
            image,
            0, 0,
            0, 0,
            WIDTH, HEIGHT
        );

        XSync(display, False);

        xrot += 0.0002;
        yrot += 0.0002;
        zrot += 0.0002;
        if(xrot > M_PI*2) xrot = 0;
        if(yrot > M_PI*2) yrot = 0;
        if(zrot > M_PI*2) zrot = 0;
    }

    XCloseDisplay(display);
    free(memory);

    return 0;
}