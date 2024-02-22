/** @file
    LED-Strip DDP Simulator.

    Copyright (C) 2024 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This code is prepared to compile for PLATFORM_DRM and PLATFORM_DESKTOP.

    This program has been created using raylib 1.3 (www.raylib.com)
    raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details).
*/

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <sys/times.h>
#define DDP_BUFSIZE 1500

#define VERSION "1.0"

/// Global Variables Definition.
typedef struct args
{
    /// Verbosity of diagnostic messages.
    int verbose;
    /// Screen/window width to use (-s).
    int screenWidth;
    /// Screen/window height to use (-s).
    int screenHeight;
    /// horizontal pixel count (-p).
    int pixelsX;
    /// vertical pixel count, automatic if `pixelCount` ist set (-p).
    int pixelsY;
    /// total pixel count, automatic if `pixelY` ist set (-n).
    int pixelCount;
    /// horizontal gap between pixels (-g).
    int gutterX;
    /// vertical gap between pixels (-g).
    int gutterY;
    /// Computed width of a pixel.
    int pixelW;
    /// Computed height of a pixel.
    int pixelH;

    /// Enable snake layout (-S).
    int snake;
    /// Enable mirror layout (-M).
    int mirror;
    /// Enable flip layout (-F).
    int flip;
    /// Enable tilt layout (-T).
    int tilt;
    /// Enable circle drawing (-C).
    int circle;
    /// Enable text overlay (-O).
    int overlay;

    /// Target FPS (-f).
    int fps;
    /// Hold N seconds before blanking (0 is forever, default 0) (-H).
    int hold;
    /// Exit after being idle N seconds (0 is never, default 0) (-E).
    int idleexit;
    /// Report rate (-r).
    int report_rate;
    /// Dump every n'th-packet (-d).
    int dump_nth;
} args_t;

typedef struct stats
{
    int draw_count;
    int packet_count;
    int packet_errors;
    // TODO: needs moving stats not just global stats
    struct timespec start_time;
    struct timespec ddp_time;
    struct timespec last_report;
    double elapsed;
} stats_t;

static args_t args = {
    .screenWidth = 800,
    .screenHeight = 600,
    .pixelsX = 20,
    .gutterX = 15,
    .fps = 60,
};

static stats_t stats = {0};

/// Creates non-blocking UDP listening socket for DDP.
static int DDPCreateListener(void)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    struct sockaddr_in serveraddr;

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(4048);

    if (bind(sockfd, (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
    {
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("ERROR on setting non-blocking mode");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

char ddp_buf[DDP_BUFSIZE];

/// Gets next DDP UDP packet if available.
static int DDPGetPacket(int sockfd, uint8_t *pixels, unsigned pixelCount)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    struct timeval tv = {0};
    //tv.tv_sec = 0;
    //tv.tv_usec = 0;

    int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if (ret != 1)
    {
        return -1;
    }

    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int datalen = recvfrom(sockfd, ddp_buf, DDP_BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
    if (datalen < 0)
    {
        perror("ERROR in recvfrom");
        exit(EXIT_FAILURE);
    }
    int rgbCount = datalen > 10 ? datalen - 10 : 0;
    if (rgbCount > pixelCount * 3)
    {
        rgbCount = pixelCount * 3;
    }

    memcpy(pixels, ddp_buf + 10, rgbCount);
    return rgbCount;
}

/// Draws a complete frame.
static void UpdateDrawFrame(uint8_t *pixels, int ddp)
{
    // Update
    stats.draw_count += 1;

    // Draw
    BeginDrawing();

        ClearBackground(BLACK);

        // Draw pixels
        for (int i = 0; i < args.pixelCount; i++)
        {
            // Find x and y positions
            int x = i % args.pixelsX;
            int y = i / args.pixelsX;
            if ((args.snake && y % 2) ? !args.mirror : args.mirror) {
                x = args.pixelsX - 1 - x;
            }
            if (args.flip) {
                y = args.pixelsY - 1 - y;
            }
            if (args.tilt) {
                int tmp = x;
                x = y;
                y = tmp;
            }
            // Draw at x, y position
            int sx = args.gutterX + (args.gutterX + args.pixelW) * x;
            int sy = args.gutterY + (args.gutterY + args.pixelH) * y;
            Color c = {pixels[i * 3], pixels[i * 3 + 1], pixels[i * 3 + 2], 255};
            if (args.circle)
            {
                DrawEllipse(sx + args.pixelW / 2, sy + args.pixelH / 2, args.pixelW / 2, args.pixelH / 2, c);
            }
            else
            {
                DrawRectangle(sx, sy, args.pixelW, args.pixelH, c);
            }
            if (args.overlay) {
                char text[8] = {0};
                snprintf(text, sizeof(text) - 1, "%i", i);
                DrawText(text, sx, sy, 20, GRAY); // Draw text (using default font)
            }
        }
        if (args.overlay) {
            if (ddp > 0) {
                DrawText("DDP", 0, 0, 20, GRAY); // Draw text (using default font)
            }
            char text[32] = {0};
            snprintf(text, sizeof(text) - 1, "%.1f s", stats.elapsed);
            DrawText(text, 60, 0, 20, GRAY);
            snprintf(text, sizeof(text) - 1, "%.1f pkt/s  %i pkt",
                     stats.packet_count / stats.elapsed, stats.packet_count);
            DrawText(text, 150, 0, 20, GRAY);
            snprintf(text, sizeof(text) - 1, "%.1f fps",
                     stats.draw_count / stats.elapsed);
            DrawText(text, 400, 0, 20, GRAY);
        }

    EndDrawing();
}

/// Prints a report.
static void ReportStats(void)
{
    printf("DDP stats: ");
    printf("runtime %.1f s, ", stats.elapsed);
    printf("%.1f pkt/s %i pkt, ",
            stats.packet_count / stats.elapsed, stats.packet_count);
    printf("%.1f fps %i frames\n",
            stats.draw_count / stats.elapsed, stats.draw_count);
}

/// Parse int arg, otherwise print error and exit.
static int parse_int(char const *arg, char const *error_hint)
{
    if (!arg)
    {
        printf("Missing parameter for %s\n", error_hint);
        exit(EXIT_FAILURE);
    }
    char *endptr;
    int val = strtol(arg, &endptr, 10);
    if (arg == endptr)
    {
        printf("Bad parameter (%s) for %s\n", arg, error_hint);
        exit(EXIT_FAILURE);
    }
    return val;
}

/// Parse int tuple arg, otherwise print error and exit.
static int parse_tuple(char const *arg, int *x, int *y, char const *error_hint)
{
    if (!arg)
    {
        printf("Missing parameter for %s\n", error_hint);
        exit(EXIT_FAILURE);
    }
    char *endptr;
    int val = strtol(arg, &endptr, 10);
    if (arg == endptr)
    {
        printf("Bad parameter (%s) for %s\n", arg, error_hint);
        exit(EXIT_FAILURE);
    }
    *x = val;

    if (*endptr != '\0')
    {
        return 1;
    }
    if (*endptr != 'x')
    {
        printf("Bad parameter (%s) for %s\n", arg, error_hint);
        exit(EXIT_FAILURE);
    }

    char const *p = endptr + 1;
    val = strtol(p, &endptr, 10);
    if (arg == endptr)
    {
        printf("Bad parameter (%s) for %s\n", arg, error_hint);
        exit(EXIT_FAILURE);
    }
    *y = val;
    return 2;
}

/// Prints the program usage help.
static void print_help()
{
    printf(
        "\nUsage:\n"
        "\t\t= General options =\n"
        "  [-V] Output the version string and exit\n"
        "  [-v] Increase verbosity (can be used multiple times).\n"
        "       -v : verbose, -vv : debug, -vvv : trace.\n"
        "  [-h] Output this usage help and exit\n"
        "\t\t= Geometry parameters =\n"
        "  [-s NxM] Screen/window width to use (default 800x600).\n"
        "  [-p N | NxM] horizontal pixel count (default 20x10).\n"
        "  [-n N] total pixel count, automatic if `pixelY` ist set.\n"
        "  [-g N | NxM] horizontal gap between pixels (default 15x15).\n"
        "\t\t= Geometry options =\n"
        "  [-S] Enable snake layout, alternates direction of rows.\n"
        "  [-M] Enable mirror layout, mirros horizontally.\n"
        "  [-F] Enable flip layout, flips vertically.\n"
        "  [-T] Enable tilt layout, transforms diagonally.\n"
        "  [-R] Rotate layout right.\n"
        "  [-L] Rotate layout left.\n"
        "  [-C] Enable circle drawing.\n"
        "  [-O] Enable text overlay.\n"
        "\t\t= Statistics options =\n"
        "  [-f N] Target FPS (default 60).\n"
        "  [-H N] Hold N seconds before blanking (0 is forever, default 0).\n"
        "  [-E N] Exit after being idle N seconds (0 is never, default 0).\n"
        "  [-r N] Report rate.\n"
        "  [-d N] Dump every n'th packet.\n");
}

/// Prints the program version.
static void print_version()
{
    printf("LED-Strip DDP Simulator version " VERSION "\n");
}

/// Main Entry Point.
int main(int argc, char *argv[])
{
    print_version();
    int opt;
    while ((opt = getopt(argc, argv, "hVvs:p:n:g:SMFTRLCOf:H:E:r:d:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
            /* NOTREACHED */
            break;
        case 'V':
            exit(EXIT_SUCCESS);
            /* NOTREACHED */
            break;
        case 'v':
            args.verbose += 1;
            break;

        case 's':
            parse_tuple(optarg, &args.screenWidth, &args.screenHeight, "-s");
            break;

        case 'p':
            parse_tuple(optarg, &args.pixelsX, &args.pixelsY, "-p");
            break;
        case 'n':
            args.pixelCount = parse_int(optarg, "-n");
            break;

        case 'g':
            parse_tuple(optarg, &args.gutterX, &args.gutterY, "-g");
            break;

        case 'S':
            args.snake = !args.snake;
            break;
        case 'M':
            args.mirror = !args.mirror;
            break;
        case 'F':
            args.flip = !args.flip;
            break;
        case 'T':
            args.tilt = !args.tilt;
            break;
        case 'R':
            if (args.tilt) {
                args.tilt = 0;
                args.flip = !args.flip;
            } else {
                args.tilt = 1;
                args.mirror = !args.mirror;
            }
            break;
        case 'L':
            if (args.tilt) {
                args.tilt = 0;
                args.mirror = !args.mirror;
            } else {
                args.tilt = 1;
                args.flip = !args.flip;
            }
            break;
        case 'C':
            args.circle += 1;
            break;
        case 'O':
            args.overlay += 1;
            break;

        case 'f':
            args.fps = parse_int(optarg, "-f");
            break;
        case 'H':
            args.hold = parse_int(optarg, "-H");
            break;
        case 'E':
            args.idleexit = parse_int(optarg, "-E");
            break;
        case 'r': // TODO: implement
            args.report_rate = parse_int(optarg, "-r");
            break;
        case 'd':
            args.dump_nth = parse_int(optarg, "-d");
            break;
        case '?':
            printf("Unknown option `-%c'.\n", optopt);
            print_help();
            exit(EXIT_FAILURE);
            /* NOTREACHED */
            break;
        }
    }

    // Use defaults if unset:
    if (!args.pixelsY && !args.pixelCount)
    {
        args.pixelsY = 10;
    }
    if (!args.pixelsY)
    {
        args.pixelsY = (args.pixelCount + args.pixelsX - 1) / args.pixelsX;
    }
    if (!args.pixelCount)
    {
        args.pixelCount = args.pixelsX * args.pixelsY;
    }

    if (!args.gutterY)
    {
        args.gutterY = args.gutterX;
    }

    // Initialization
    InitWindow(args.screenWidth, args.screenHeight, "LED-Strip DDP Simulator");

    SetTargetFPS(args.fps);   // Set our display to target at N frames-per-second

    uint8_t pixels[256 * 3] = {0};

    int sockfd = DDPCreateListener();

    clock_gettime(CLOCK_MONOTONIC, &stats.start_time);

    // Main display loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
#if defined(PLATFORM_DESKTOP)
        // Input
        if (IsKeyPressed(KEY_F)) ToggleFullscreen();

        if (IsKeyPressed(KEY_R))
        {
            if (IsWindowState(FLAG_WINDOW_RESIZABLE)) ClearWindowState(FLAG_WINDOW_RESIZABLE);
            else SetWindowState(FLAG_WINDOW_RESIZABLE);
        }

        if (IsKeyPressed(KEY_D))
        {
            if (IsWindowState(FLAG_WINDOW_UNDECORATED)) ClearWindowState(FLAG_WINDOW_UNDECORATED);
            else SetWindowState(FLAG_WINDOW_UNDECORATED);
        }

        if (IsKeyPressed(KEY_M))
        {
            // NOTE: Requires FLAG_WINDOW_RESIZABLE enabled!
            if (IsWindowState(FLAG_WINDOW_MAXIMIZED)) RestoreWindow();
            else MaximizeWindow();
        }

        if (IsKeyPressed(KEY_T))
        {
            if (IsWindowState(FLAG_WINDOW_TOPMOST)) ClearWindowState(FLAG_WINDOW_TOPMOST);
            else SetWindowState(FLAG_WINDOW_TOPMOST);
        }

        if (IsKeyPressed(KEY_V))
        {
            if (IsWindowState(FLAG_VSYNC_HINT)) ClearWindowState(FLAG_VSYNC_HINT);
            else SetWindowState(FLAG_VSYNC_HINT);
        }
#endif

        // Update state
        int ddp_len = DDPGetPacket(sockfd, pixels, args.pixelCount);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        stats.elapsed = (now.tv_sec - stats.start_time.tv_sec);
        stats.elapsed += (now.tv_nsec - stats.start_time.tv_nsec) / 1000000000.0;

        if (ddp_len < 0)
        {
            stats.packet_errors += 1;
        }
        if (ddp_len > 0)
        {
            stats.packet_count += 1;
            stats.ddp_time = now;

            if (args.dump_nth && stats.packet_count % args.dump_nth == 0)
            {
                char *b = ddp_buf;
                printf("DDP: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x %02x%02x%02x ... (len: %d, %d pixel)\n",
                        b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12],
                        ddp_len + 10, ddp_len / 3);
            }
        }
        if (args.report_rate && now.tv_sec - stats.last_report.tv_sec > args.report_rate)
        {
            stats.last_report = now;
            ReportStats();
        }
        if (args.hold
                && now.tv_sec - stats.start_time.tv_sec > args.hold
                && now.tv_sec - stats.ddp_time.tv_sec > args.hold)
        {
            // Blank if no DDP packet for more than `args.hold` seconds.
            memset(pixels, 0, sizeof(pixels));
        }
        if (args.idleexit
                && now.tv_sec - stats.start_time.tv_sec > args.idleexit
                && now.tv_sec - stats.ddp_time.tv_sec > args.idleexit)
        {
            // Exit if no DDP packet for more than `args.idleexit` seconds.
            break;
        }

        // Drawing
        args.screenWidth = GetScreenWidth();
        args.screenHeight = GetScreenHeight();
        args.pixelW = (args.screenWidth - (args.pixelsX + 1) * args.gutterX) / args.pixelsX;
        args.pixelH = (args.screenHeight - (args.pixelsY + 1) * args.gutterY) / args.pixelsY;

        UpdateDrawFrame(pixels, ddp_len);
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
