/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Code debugging tools. See the header file for more information.
 */

#include <allegro5/allegro.h>

#include "code_debug.h"


double code_debug_benchmark_measure_start;
double code_debug_benchmark_sum;
unsigned int code_debug_benchmark_iterations;


#ifdef CODE_DEBUG_NEW

map<void*, string> code_debug_new_allocs;
bool code_debug_new_recording;

/**
 * @brief Overrides operator delete.
 *
 * @param ptr Pointer to memory to deallocate.
 */
void operator delete(void* ptr) noexcept {
    if(code_debug_new_recording) {
        map<void*, string>::iterator it = code_debug_new_allocs.find(ptr);
        if(it != code_debug_new_allocs.end()) {
            code_debug_new_allocs.erase(it);
        }
    }
    return free(ptr);
}


/**
 * @brief Overrides operator delete[].
 *
 * @param ptr Pointer to memory to deallocate.
 */
void operator delete[](void* ptr) noexcept {
    if(code_debug_new_recording) {
        map<void*, string>::iterator it = code_debug_new_allocs.find(ptr);
        if(it != code_debug_new_allocs.end()) {
            code_debug_new_allocs.erase(it);
        }
    }
    return free(ptr);
}


/**
 * @brief Overrides operator new.
 *
 * @param size Size of memory to allocate.
 * @param file Name of the code file that requested this allocation.
 * @param line Line in the code file that requested this allocation.
 */
void* operator new(size_t size, char* file, int line) {
    void* ptr = malloc(size);
    if(code_debug_new_recording) {
        code_debug_new_allocs[ptr] =
            string(file) + ":" + to_string((long long) line);
    }
    return ptr;
}


/**
 * @brief Overrides operator new[].
 *
 * @param size Size of memory to allocate.
 * @param file Name of the code file that requested this allocation.
 * @param line Line in the code file that requested this allocation.
 */
void* operator new[](size_t size, char* file, int line) {
    void* ptr = malloc(size);
    if(code_debug_new_recording) {
        code_debug_new_allocs[ptr] =
            string(file) + ":" + to_string((long long) line);
    }
    return ptr;
}


#endif //ifndef CODE_DEBUG_NEW



/**
 * @brief Starts a time measurement for benchmarking.
 */
void codeDebugBenchmarkStartMeasuring() {
    code_debug_benchmark_measure_start = al_get_time();
}


/**
 * @brief Finishes a time measurement for benchmarking. Stores and returns the
 * time difference.
 *
 * @return The time difference.
 */
double codeDebugBenchmarkEndMeasuring() {
    double duration = al_get_time() - code_debug_benchmark_measure_start;
    code_debug_benchmark_sum += duration;
    code_debug_benchmark_iterations++;
    return duration;
}


/**
 * @brief Returns the average duration of all measurements taken so far.
 *
 * @return The average duration.
 */
double codeDebugBenchmarkGetAvgDuration() {
    if(code_debug_benchmark_iterations == 0) return 0.0f;
    return
        code_debug_benchmark_sum / (double) code_debug_benchmark_iterations;
}
