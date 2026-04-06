// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Voscillator.h for the primary calling header

#include "Voscillator__pch.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Voscillator___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

void Voscillator___024root___eval_triggers__act(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_triggers__act\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VactTriggered[0U] = (QData)((IData)(
                                                    ((IData)(vlSelfRef.clk) 
                                                     & (~ (IData)(vlSelfRef.__Vtrigprevexpr___TOP__clk__0)))));
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Voscillator___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
    }
#endif
}

bool Voscillator___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___trigger_anySet__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        if (in[n]) {
            return (1U);
        }
        n = ((IData)(1U) + n);
    } while ((1U > n));
    return (0U);
}

void Voscillator___024root___nba_sequent__TOP__0(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___nba_sequent__TOP__0\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    SData/*15:0*/ __Vdly__oscillator__DOT__phase_acc;
    __Vdly__oscillator__DOT__phase_acc = 0;
    // Body
    __Vdly__oscillator__DOT__phase_acc = vlSelfRef.oscillator__DOT__phase_acc;
    if (vlSelfRef.rst) {
        __Vdly__oscillator__DOT__phase_acc = 0U;
        vlSelfRef.valid = 0U;
    } else {
        vlSelfRef.valid = 0U;
        if (vlSelfRef.sample_en) {
            __Vdly__oscillator__DOT__phase_acc = (0x0000ffffU 
                                                  & (((IData)(vlSelfRef.oscillator__DOT__phase_acc) 
                                                      >= 
                                                      ((IData)(0x0000bb80U) 
                                                       - (IData)(vlSelfRef.step)))
                                                      ? 
                                                     ((IData)(vlSelfRef.oscillator__DOT__phase_acc) 
                                                      - 
                                                      ((IData)(0xbb80U) 
                                                       - (IData)(vlSelfRef.step)))
                                                      : 
                                                     ((IData)(vlSelfRef.oscillator__DOT__phase_acc) 
                                                      + (IData)(vlSelfRef.step))));
            vlSelfRef.valid = 1U;
        }
    }
    vlSelfRef.oscillator__DOT__phase_acc = __Vdly__oscillator__DOT__phase_acc;
    vlSelfRef.wavetable_addr = vlSelfRef.oscillator__DOT__phase_acc;
}

void Voscillator___024root___eval_nba(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_nba\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((1ULL & vlSelfRef.__VnbaTriggered[0U])) {
        Voscillator___024root___nba_sequent__TOP__0(vlSelf);
    }
}

void Voscillator___024root___trigger_orInto__act(VlUnpacked<QData/*63:0*/, 1> &out, const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___trigger_orInto__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = (out[n] | in[n]);
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Voscillator___024root___eval_phase__act(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_phase__act\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Voscillator___024root___eval_triggers__act(vlSelf);
    Voscillator___024root___trigger_orInto__act(vlSelfRef.__VnbaTriggered, vlSelfRef.__VactTriggered);
    return (0U);
}

void Voscillator___024root___trigger_clear__act(VlUnpacked<QData/*63:0*/, 1> &out) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___trigger_clear__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = 0ULL;
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Voscillator___024root___eval_phase__nba(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_phase__nba\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = Voscillator___024root___trigger_anySet__act(vlSelfRef.__VnbaTriggered);
    if (__VnbaExecute) {
        Voscillator___024root___eval_nba(vlSelf);
        Voscillator___024root___trigger_clear__act(vlSelfRef.__VnbaTriggered);
    }
    return (__VnbaExecute);
}

void Voscillator___024root___eval(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VnbaIterCount;
    // Body
    __VnbaIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VnbaIterCount)))) {
#ifdef VL_DEBUG
            Voscillator___024root___dump_triggers__act(vlSelfRef.__VnbaTriggered, "nba"s);
#endif
            VL_FATAL_MT("oscillator.sv", 1, "", "NBA region did not converge after 100 tries");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        vlSelfRef.__VactIterCount = 0U;
        do {
            if (VL_UNLIKELY(((0x00000064U < vlSelfRef.__VactIterCount)))) {
#ifdef VL_DEBUG
                Voscillator___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
#endif
                VL_FATAL_MT("oscillator.sv", 1, "", "Active region did not converge after 100 tries");
            }
            vlSelfRef.__VactIterCount = ((IData)(1U) 
                                         + vlSelfRef.__VactIterCount);
        } while (Voscillator___024root___eval_phase__act(vlSelf));
    } while (Voscillator___024root___eval_phase__nba(vlSelf));
}

#ifdef VL_DEBUG
void Voscillator___024root___eval_debug_assertions(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_debug_assertions\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if (VL_UNLIKELY(((vlSelfRef.clk & 0xfeU)))) {
        Verilated::overWidthError("clk");
    }
    if (VL_UNLIKELY(((vlSelfRef.rst & 0xfeU)))) {
        Verilated::overWidthError("rst");
    }
    if (VL_UNLIKELY(((vlSelfRef.sample_en & 0xfeU)))) {
        Verilated::overWidthError("sample_en");
    }
}
#endif  // VL_DEBUG
