#include "game.h"
#include <math.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PSP_MODULE_INFO("Dustbound", 0, 0, 2);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12 * 1024);
static volatile int running = 1;
static int exit_callback(int a, int b, void *c) {
    (void)a;
    (void)b;
    (void)c;
    running = 0;
    return 0;
}
static int callback_thread(SceSize n, void *p) {
    (void)n;
    (void)p;
    int cb = sceKernelCreateCallback("Exit", exit_callback, 0);
    sceKernelRegisterExitCallback(cb);
    sceKernelSleepThreadCB();
    return 0;
}
static int buttons(unsigned b) {
    int k = 0;
    if (b & PSP_CTRL_UP)
        k |= KEY_UP;
    if (b & PSP_CTRL_DOWN)
        k |= KEY_DOWN;
    if (b & PSP_CTRL_LEFT)
        k |= KEY_LEFT;
    if (b & PSP_CTRL_RIGHT)
        k |= KEY_RIGHT;
    if (b & PSP_CTRL_CROSS)
        k |= KEY_CROSS;
    if (b & PSP_CTRL_CIRCLE)
        k |= KEY_CIRCLE;
    if (b & PSP_CTRL_SQUARE)
        k |= KEY_SQUARE;
    if (b & PSP_CTRL_TRIANGLE)
        k |= KEY_TRIANGLE;
    if (b & PSP_CTRL_LTRIGGER)
        k |= KEY_L;
    if (b & PSP_CTRL_RTRIGGER)
        k |= KEY_R;
    if (b & PSP_CTRL_START)
        k |= KEY_START;
    if (b & PSP_CTRL_SELECT)
        k |= KEY_SELECT;
    return k;
}
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    int th = sceKernelCreateThread("Callbacks", callback_thread, 0x11, 0x1000, 0, 0);
    if (th >= 0)
        sceKernelStartThread(th, 0, 0);
    scePowerSetClockFrequency(333, 333, 166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    sceDisplaySetMode(0, 480, 272);
    /* Two 512-stride 8888 framebuffers in uncached VRAM. Never display a buffer while writing it. */
    uint32_t *buffers[2] = {(uint32_t *)0x44000000, (uint32_t *)(0x44000000 + 512 * 272 * 4)};
    memset(buffers[0], 0, 512 * 272 * 4 * 2);
    sceDisplaySetFrameBuf((void *)0x04000000, 512, PSP_DISPLAY_PIXEL_FORMAT_8888,
                          PSP_DISPLAY_SETBUF_IMMEDIATE);
    int draw = 1, old = 0, frames = 0;
    float repeat = 0;
    audio_init();
    game_new(73819, 0);
    ui.screen = SCREEN_TITLE;
    FILE *q = fopen("DUST_QA", "r");
    int trace = q != NULL;
    if (q)
        fclose(q);
    FILE *recordFlag = fopen("DUST_RECORD", "r");
    int record = recordFlag != NULL, recorded = 0;
    if (recordFlag)
        fclose(recordFlag);
    SceCtrlData pad;
    uint64_t last, now;
    sceRtcGetCurrentTick(&last);
    unsigned resolution = sceRtcGetTickResolution();
    while (running) {
        sceRtcGetCurrentTick(&now);
        float dt = (now - last) / (float)resolution;
        last = now;
        if (dt <= 0)
            dt = 1.0f / 30;
        if (dt > .1f)
            dt = .1f;
        sceCtrlPeekBufferPositive(&pad, 1);
        int held = buttons(pad.Buttons), press = held & ~old;
        old = held;
        if (ui.screen != SCREEN_WORLD && (held & (KEY_UP | KEY_DOWN))) {
            repeat += dt;
            if (repeat > .34f) {
                press |= held & (KEY_UP | KEY_DOWN);
                repeat = .25f;
            }
        } else
            repeat = 0;
        float ax = (pad.Lx - 128) / 127.0f, ay = (pad.Ly - 128) / 127.0f;
        if (fabsf(ax) < .22f)
            ax = 0;
        if (fabsf(ay) < .22f)
            ay = 0;
        if (record) dt = 1.0f / 30; /* Deterministic QA pacing despite capture I/O. */
        game_tick(dt, (Input){held, press, ax, ay});
        render_game(buffers[draw], 512);
        /* Optional QA readback of frames produced by this PSP executable.
           Marker absent in release packages; no recording cost in normal play. */
        if (record && ui.screen == SCREEN_WORLD && recorded < 240) {
            char path[64];
            snprintf(path, sizeof(path), "QA_FRAME_%03d.rgba", recorded);
            FILE *out = fopen(path, "wb");
            if (out) {
                fwrite(buffers[draw], 512 * 272 * 4, 1, out);
                fclose(out);
            }
            out = fopen("QA_FRAMES.csv", recorded ? "a" : "w");
            if (out) {
                if (!recorded)
                    fprintf(out, "frame,time,attack_phase\n");
                fprintf(out, "%d,%.4f,%d\n", recorded, g.time, g.actors[g.selected].attackPhase);
                fclose(out);
            }
            recorded++;
        }
        sceDisplayWaitVblankStart();
        sceDisplaySetFrameBuf((void *)(0x04000000 + draw * 512 * 272 * 4), 512, PSP_DISPLAY_PIXEL_FORMAT_8888,
                              PSP_DISPLAY_SETBUF_NEXTFRAME);
        draw ^= 1;
        sceDisplayWaitVblankStart();
        display_buffer = 0x04000000 + (1 - draw) * 512 * 272 * 4;
        rendered_frames++;
        if (trace && ++frames % 30 == 0)
            debug_snapshot("DUST_STATE.json");
    }
    if (ui.screen != SCREEN_TITLE)
        save_slot(0);
    audio_shutdown();
    sceKernelExitGame();
    return 0;
}
