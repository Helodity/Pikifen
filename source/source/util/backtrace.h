/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the backtrace functions.
 */

#pragma once

#include <string>
#include <vector>


using std::size_t;
using std::string;
using std::vector;


namespace BACKTRACE {
const size_t MAX_FRAMES = 30;
const size_t MAX_SYMBOL_LENGTH = 512;
}


#if defined(__linux__) || (__APPLE__)
//Linux and Mac OS.

#include <execinfo.h>
#include <cxxabi.h>


/**
 * @brief Demangles a mangled debugging symbol.
 * 
 * @param symbol The symbol to demangle.
 * @return The demangled symbol.
 */
string demangeSymbol(const string &symbol) {
    //Special thanks: https://oroboro.com/stack-trace-on-crash/
    size_t module_size = 0;
    size_t name_size = 0;
    size_t offset_start = string::npos;
    size_t offset_size = 0;
    string ret;
    
#ifdef __APPLE__ //Mac OS.
    size_t name_start = symbol.find(" _");
    if(name_start != string::npos) {
        module_size = name_start;
        name_start = name_start + 1;
        size_t space_pos = symbol.find(" ", name_start + 1);
        if(space_pos != string::npos) {
            name_size = space_pos - name_start;
            offset_start = name_start + name_size + 1;
            if(offset_start != string::npos) {
                offset_size = symbol.size();
            }
        }
    }
#else //Linux.
    size_t name_start = symbol.find("(");
    if(name_start != string::npos) {
        module_size = name_start;
        name_start = name_start + 1;
        size_t plus_pos = symbol.find("+", name_start + 1);
        if(plus_pos != string::npos) {
            name_size = plus_pos - name_start;
            offset_start = name_start + name_size + 1;
            if(offset_start != string::npos) {
                offset_size = symbol.find(")", offset_start) - offset_start;
            }
        }
    }
#endif
    
    if(name_start != string::npos && offset_start != string::npos) {
        string module_str = symbol.substr(0, module_size);
        string mangled_name = symbol.substr(name_start, name_size);
        string offset_str = symbol.substr(offset_start, offset_size);
        
        int demangle_status;
        char* demangled_name =
            abi::__cxa_demangle(
                mangled_name.c_str(), nullptr, nullptr, &demangle_status
            );
            
        if(demangle_status == 0) {
            ret =
                module_str + " " + demangled_name + " + " + offset_str;
        } else {
            ret =
                module_str + " " + mangled_name + " + " + offset_str;
        }
    } else {
        ret = symbol;
    }
    return ret;
}


/**
 * @brief Returns the backtrace of the current stack.
 * 
 * @return The backtrace.
 */
vector<string> getBacktrace() {
    vector<string> result;
    void* stack[BACKTRACE::MAX_FRAMES];
    
    size_t n_symbols = backtrace(stack, BACKTRACE::MAX_FRAMES);
    char** symbols = backtrace_symbols(stack, n_symbols);
    
    for(size_t s = 0; s < n_symbols; s++) {
        result.push_back(demangeSymbol(symbols[s]));
    }
    
    free(symbols);
    
    if(result.empty()) {
        result.push_back("(Could not obtain)");
    }
    return result;
}



#elif defined(_WIN32)
//Windows.

#include <Windows.h>
#include <DbgHelp.h>
#include <sstream>
#include <WinBase.h>

/**
 * @brief Returns the backtrace of the current stack.
 * 
 * @return The backtrace.
 */
vector<string> getBacktrace() {
    vector<string> result;
    void* stack[BACKTRACE::MAX_FRAMES];
    
    HANDLE process = GetCurrentProcess();
    SYMBOL_INFO* symbol =
        (SYMBOL_INFO*) malloc(
            sizeof(SYMBOL_INFO) +
            (BACKTRACE::MAX_SYMBOL_LENGTH - 1) * sizeof(TCHAR)
        );
    symbol->MaxNameLen = BACKTRACE::MAX_SYMBOL_LENGTH;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    DWORD dummy_displacement;
    
    IMAGEHLP_LINE64* line = (IMAGEHLP_LINE64*) malloc(sizeof(IMAGEHLP_LINE64));
    line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    
    SymInitialize(process, nullptr, TRUE);
    size_t n_symbols =
        CaptureStackBackTrace(0, BACKTRACE::MAX_FRAMES, stack, nullptr);
        
    for(size_t s = 0; s < n_symbols; s++) {
        SymFromAddr(process, (DWORD64) stack[s], nullptr, symbol);
        
        std::stringstream str;
        if(
            SymGetLineFromAddr64(
                process, (DWORD64) stack[s],
                &dummy_displacement, line
            )
        ) {
            str <<
                symbol->Name << " in " <<
                line->FileName << ":" << line->LineNumber <<
                " [" << symbol->Address << "]";
        } else {
            str << symbol->Name << " [" << symbol->Address << "]";
        }
        
        result.push_back(str.str());
    }
    
    free(line);
    free(symbol);
    
    if(result.empty()) {
        result.push_back("(Could not obtain)");
    }
    return result;
}



#else
//Not supported.

/**
 * @brief Returns the backtrace of the current stack.
 * 
 * @return The backtrace.
 */
vector<string> getBacktrace() {
    vector<string> v;
    v.push_back("(Not supported)");
    return v;
}


#endif
