// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Voscillator__pch.h"
#include "verilated_vcd_c.h"

//============================================================
// Constructors

Voscillator::Voscillator(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Voscillator__Syms(contextp(), _vcname__, this)}
    , clk{vlSymsp->TOP.clk}
    , rst{vlSymsp->TOP.rst}
    , sample_en{vlSymsp->TOP.sample_en}
    , valid{vlSymsp->TOP.valid}
    , step{vlSymsp->TOP.step}
    , wavetable_addr{vlSymsp->TOP.wavetable_addr}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
    contextp()->traceBaseModelCbAdd(
        [this](VerilatedTraceBaseC* tfp, int levels, int options) { traceBaseModel(tfp, levels, options); });
}

Voscillator::Voscillator(const char* _vcname__)
    : Voscillator(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Voscillator::~Voscillator() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Voscillator___024root___eval_debug_assertions(Voscillator___024root* vlSelf);
#endif  // VL_DEBUG
void Voscillator___024root___eval_static(Voscillator___024root* vlSelf);
void Voscillator___024root___eval_initial(Voscillator___024root* vlSelf);
void Voscillator___024root___eval_settle(Voscillator___024root* vlSelf);
void Voscillator___024root___eval(Voscillator___024root* vlSelf);

void Voscillator::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Voscillator::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Voscillator___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_activity = true;
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        vlSymsp->__Vm_didInit = true;
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Voscillator___024root___eval_static(&(vlSymsp->TOP));
        Voscillator___024root___eval_initial(&(vlSymsp->TOP));
        Voscillator___024root___eval_settle(&(vlSymsp->TOP));
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Voscillator___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Voscillator::eventsPending() { return false; }

uint64_t Voscillator::nextTimeSlot() {
    VL_FATAL_MT(__FILE__, __LINE__, "", "No delays in the design");
    return 0;
}

//============================================================
// Utilities

const char* Voscillator::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Voscillator___024root___eval_final(Voscillator___024root* vlSelf);

VL_ATTR_COLD void Voscillator::final() {
    Voscillator___024root___eval_final(&(vlSymsp->TOP));
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Voscillator::hierName() const { return vlSymsp->name(); }
const char* Voscillator::modelName() const { return "Voscillator"; }
unsigned Voscillator::threads() const { return 1; }
void Voscillator::prepareClone() const { contextp()->prepareClone(); }
void Voscillator::atClone() const {
    contextp()->threadPoolpOnClone();
}
std::unique_ptr<VerilatedTraceConfig> Voscillator::traceConfig() const {
    return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{false, false, false}};
};

//============================================================
// Trace configuration

void Voscillator___024root__trace_decl_types(VerilatedVcd* tracep);

void Voscillator___024root__trace_init_top(Voscillator___024root* vlSelf, VerilatedVcd* tracep);

VL_ATTR_COLD static void trace_init(void* voidSelf, VerilatedVcd* tracep, uint32_t code) {
    // Callback from tracep->open()
    Voscillator___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Voscillator___024root*>(voidSelf);
    Voscillator__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {
        VL_FATAL_MT(__FILE__, __LINE__, __FILE__,
            "Turning on wave traces requires Verilated::traceEverOn(true) call before time 0.");
    }
    vlSymsp->__Vm_baseCode = code;
    tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);
    Voscillator___024root__trace_decl_types(tracep);
    Voscillator___024root__trace_init_top(vlSelf, tracep);
    tracep->popPrefix();
}

VL_ATTR_COLD void Voscillator___024root__trace_register(Voscillator___024root* vlSelf, VerilatedVcd* tracep);

VL_ATTR_COLD void Voscillator::traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options) {
    (void)levels; (void)options;
    VerilatedVcdC* const stfp = dynamic_cast<VerilatedVcdC*>(tfp);
    if (VL_UNLIKELY(!stfp)) {
        vl_fatal(__FILE__, __LINE__, __FILE__,"'Voscillator::trace()' called on non-VerilatedVcdC object;"
            " use --trace-fst with VerilatedFst object, and --trace-vcd with VerilatedVcd object");
    }
    stfp->spTrace()->addModel(this);
    stfp->spTrace()->addInitCb(&trace_init, &(vlSymsp->TOP));
    Voscillator___024root__trace_register(&(vlSymsp->TOP), stfp->spTrace());
}
