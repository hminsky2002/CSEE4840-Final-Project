// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Voscillator.h for the primary calling header

#include "Voscillator__pch.h"

void Voscillator___024root___ctor_var_reset(Voscillator___024root* vlSelf);

Voscillator___024root::Voscillator___024root(Voscillator__Syms* symsp, const char* namep)
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Voscillator___024root___ctor_var_reset(this);
}

void Voscillator___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Voscillator___024root::~Voscillator___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
