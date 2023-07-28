#pragma once

#include "pch.h"
#include "tictoc.h"

LONGLONG Tic() {
    LARGE_INTEGER counter;
    if (QueryPerformanceCounter(&counter)) {
        return counter.QuadPart;
    }
    return 0;
}

float Toc(LONGLONG ticTime) {
    LARGE_INTEGER freq;
    LONGLONG freqQuadPart = 0;
    if (QueryPerformanceFrequency(&freq)) {
        freqQuadPart = freq.QuadPart;
    }

    LARGE_INTEGER counter;
    if (ticTime && freqQuadPart && QueryPerformanceCounter(&counter)) {
        return float((counter.QuadPart - ticTime) * 1000.0 / freqQuadPart);
    }
    return 0;
}