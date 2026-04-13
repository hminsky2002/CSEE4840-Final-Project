// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Voscillator.h for the primary calling header

#include "Voscillator__pch.h"

VL_ATTR_COLD void Voscillator___024root___eval_static(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_static\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
}

VL_ATTR_COLD void Voscillator___024root___eval_initial(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_initial\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

VL_ATTR_COLD void Voscillator___024root___eval_final(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_final\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Voscillator___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Voscillator___024root___eval_phase__stl(Voscillator___024root* vlSelf);

VL_ATTR_COLD void Voscillator___024root___eval_settle(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_settle\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VstlIterCount;
    // Body
    __VstlIterCount = 0U;
    vlSelfRef.__VstlFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VstlIterCount)))) {
#ifdef VL_DEBUG
            Voscillator___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
#endif
            VL_FATAL_MT("oscillator.sv", 1, "", "Settle region did not converge after 100 tries");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
    } while (Voscillator___024root___eval_phase__stl(vlSelf));
}

VL_ATTR_COLD void Voscillator___024root___eval_triggers__stl(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_triggers__stl\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VstlTriggered[0U] = ((0xfffffffffffffffeULL 
                                      & vlSelfRef.__VstlTriggered
                                      [0U]) | (IData)((IData)(vlSelfRef.__VstlFirstIteration)));
    vlSelfRef.__VstlFirstIteration = 0U;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Voscillator___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
    }
#endif
}

VL_ATTR_COLD bool Voscillator___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Voscillator___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(Voscillator___024root___trigger_anySet__stl(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD bool Voscillator___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___trigger_anySet__stl\n"); );
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

VL_ATTR_COLD void Voscillator___024root___stl_sequent__TOP__0(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___stl_sequent__TOP__0\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.wavetable_addr = vlSelfRef.oscillator__DOT__phase_acc;
}

VL_ATTR_COLD void Voscillator___024root___eval_stl(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_stl\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((1ULL & vlSelfRef.__VstlTriggered[0U])) {
        Voscillator___024root___stl_sequent__TOP__0(vlSelf);
    }
}

VL_ATTR_COLD bool Voscillator___024root___eval_phase__stl(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___eval_phase__stl\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VstlExecute;
    // Body
    Voscillator___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = Voscillator___024root___trigger_anySet__stl(vlSelfRef.__VstlTriggered);
    if (__VstlExecute) {
        Voscillator___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

bool Voscillator___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Voscillator___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(Voscillator___024root___trigger_anySet__act(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Voscillator___024root___ctor_var_reset(Voscillator___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Voscillator___024root___ctor_var_reset\n"); );
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    const uint64_t __VscopeHash = VL_MURMUR64_HASH(vlSelf->vlNamep);
    vlSelf->clk = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 16707436170211756652ull);
    vlSelf->rst = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 18209466448985614591ull);
    vlSelf->step = VL_SCOPED_RAND_RESET_I(16, __VscopeHash, 6399494957627430472ull);
    vlSelf->sample_en = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 4392723383910105695ull);
    vlSelf->wavetable_addr = VL_SCOPED_RAND_RESET_I(16, __VscopeHash, 4765399908845357098ull);
    vlSelf->valid = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 4944192500720994163ull);
    vlSelf->oscillator__DOT__phase_acc = VL_SCOPED_RAND_RESET_I(16, __VscopeHash, 15908119024948310932ull);
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VstlTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VactTriggered[__Vi0] = 0;
    }
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VnbaTriggered[__Vi0] = 0;
    }
}
