// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Voscillator.h for the primary calling header

#ifndef VERILATED_VOSCILLATOR___024ROOT_H_
#define VERILATED_VOSCILLATOR___024ROOT_H_  // guard

#include "verilated.h"


class Voscillator__Syms;

class alignas(VL_CACHE_LINE_BYTES) Voscillator___024root final {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst,0,0);
    VL_IN8(sample_en,0,0);
    VL_OUT8(valid,0,0);
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
    VL_IN16(step,15,0);
    VL_OUT16(wavetable_addr,15,0);
    SData/*15:0*/ oscillator__DOT__phase_acc;
    IData/*31:0*/ __VactIterCount;
    VlUnpacked<QData/*63:0*/, 1> __VstlTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VactTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Voscillator__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Voscillator___024root(Voscillator__Syms* symsp, const char* namep);
    ~Voscillator___024root();
    VL_UNCOPYABLE(Voscillator___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
