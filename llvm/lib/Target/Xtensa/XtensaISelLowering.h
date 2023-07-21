//===- XtensaISelLowering.h - Xtensa DAG Lowering Interface -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Xtensa uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H
#define LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
namespace XtensaISD {
enum {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  BR_T,
  BR_F,

  // Conditional branch with FP operands
  BR_CC_FP,

  BR_JT,

  // Calls a function.  Operand 0 is the chain operand and operand 1
  // is the target address.  The arguments start at operand 2.
  // There is an optional glue operand at the end.
  CALL,
  // WinABI Call version
  CALLW,

  // Floating point unordered compare conditions
  CMPUEQ,
  CMPULE,
  CMPULT,
  CMPUO,
  // Floating point compare conditions
  CMPOEQ,
  CMPOLE,
  CMPOLT,

  // Branch at the end of the loop, uses result of the LOOPDEC
  LOOPBR,
  // Decrement loop counter
  LOOPDEC,
  LOOPEND,

  // FP multipy-add/sub
  MADD,
  MSUB,
  // FP move
  MOVS,

  MEMW,

  MOVSP,

  // Wraps a TargetGlobalAddress that should be loaded using PC-relative
  // accesses.  Operand 0 is the address.
  PCREL_WRAPPER,

  // Return with a flag operand.  Operand 0 is the chain operand.
  RET_FLAG,
  // WinABI Return
  RETW_FLAG,

  RUR,

  // Selects between operand 0 and operand 1.  Operand 2 is the
  // mask of condition-code values for which operand 0 should be
  // chosen over operand 1; it has the same form as BR_CCMASK.
  // Operand 3 is the flag operand.
  SELECT,
  SELECT_CC,
  SELECT_CC_FP,

  // Shift
  SHL,
  SRA,
  SRL,
  SRC,
  SSL,
  SSR
};
}

class XtensaSubtarget;

class XtensaTargetLowering : public TargetLowering {
public:
  explicit XtensaTargetLowering(const TargetMachine &TM,
                                const XtensaSubtarget &STI);

  MVT getScalarShiftAmountTy(const DataLayout &, EVT LHSTy) const override {
    return LHSTy.getSizeInBits() <= 32 ? MVT::i32 : MVT::i64;
  }

  /// Return the register type for a given MVT
  MVT getRegisterTypeForCallingConv(LLVMContext &Context, CallingConv::ID CC,
                                    EVT VT) const override;

  EVT getSetCCResultType(const DataLayout &, LLVMContext &,
                         EVT VT) const override {
    if (!VT.isVector())
      return MVT::i32;
    return VT.changeVectorElementTypeToInteger();
  }

  bool isFMAFasterThanFMulAndFAdd(const MachineFunction &MF,
                                  EVT VT) const override;

  bool isFNegFree(EVT VT) const override;

  /// If a physical register, this returns the register that receives the
  /// exception address on entry to an EH pad.
  Register
  getExceptionPointerRegister(const Constant *PersonalityFn) const override;
  /// If a physical register, this returns the register that receives the
  /// exception typeid on entry to a landing pad.
  Register
  getExceptionSelectorRegister(const Constant *PersonalityFn) const override;

  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;
  bool isFPImmLegal(const APFloat &Imm, EVT VT,
                    bool ForCodeSize) const override;
  const char *getTargetNodeName(unsigned Opcode) const override;

  /// Returns the size of the platform's va_list object.
  unsigned getVaListSizeInBits(const DataLayout &DL) const override;

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;
  TargetLowering::ConstraintType
  getConstraintType(StringRef Constraint) const override;
  TargetLowering::ConstraintWeight
  getSingleConstraintMatchWeight(AsmOperandInfo &info,
                                 const char *constraint) const override;

  void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;

  bool shouldInsertFencesForAtomic(const Instruction *I) const override {
    return true;
  }

  bool shouldReduceLoadWidth(SDNode *Load, ISD::LoadExtType ExtTy,
                             EVT NewVT) const override {
    return false;
  }

  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *BB) const override;

  MachineBasicBlock *EmitDSPInstrWithCustomInserter(
      MachineInstr &MI, MachineBasicBlock *MBB, const TargetInstrInfo &TII,
      MachineFunction *MF, MachineRegisterInfo &MRI, DebugLoc DL) const;

private:
  const XtensaSubtarget &Subtarget;

  SDValue LowerBR_JT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerImmediate(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerImmediateFP(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalTLSAddress(GlobalAddressSDNode *Node,
                                SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(BlockAddressSDNode *Node, SelectionDAG &DAG) const;
  SDValue LowerJumpTable(JumpTableSDNode *JT, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(ConstantPoolSDNode *CP, SelectionDAG &DAG) const;

  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVACOPY(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSTACKSAVE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSTACKRESTORE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerShiftLeftParts(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerShiftRightParts(SDValue Op, SelectionDAG &DAG, bool IsSRA) const;
  SDValue LowerFunnelShift(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerATOMIC_FENCE(SDValue Op, SelectionDAG &DAG) const;

  SDValue getAddrPCRel(SDValue Op, SelectionDAG &DAG) const;

  CCAssignFn *CCAssignFnForCall(CallingConv::ID CC, bool IsVarArg) const;

  // Implement EmitInstrWithCustomInserter for individual operation types.
  MachineBasicBlock *emitSelectCC(MachineInstr &MI,
                                  MachineBasicBlock *BB) const;
  MachineBasicBlock *emitAtomicSwap(MachineInstr &MI, MachineBasicBlock *BB,
                                    int isByteOperand) const;
  MachineBasicBlock *emitAtomicCmpSwap(MachineInstr &MI, MachineBasicBlock *BB,
                                       int isByteOperand) const;
  MachineBasicBlock *emitAtomicSwap(MachineInstr &MI,
                                    MachineBasicBlock *BB) const;
  MachineBasicBlock *emitAtomicRMW(MachineInstr &MI, MachineBasicBlock *BB,
                                   bool isByteOperand, unsigned Opcode,
                                   bool inv, bool minmax) const;
  MachineBasicBlock *emitAtomicRMW(MachineInstr &MI, MachineBasicBlock *BB,
                                   unsigned Opcode, bool inv,
                                   bool minmax) const;

  unsigned getInlineAsmMemConstraint(StringRef ConstraintCode) const override {
    if (ConstraintCode == "R")
      return InlineAsm::Constraint_R;
    else if (ConstraintCode == "ZC")
      return InlineAsm::Constraint_ZC;
    return TargetLowering::getInlineAsmMemConstraint(ConstraintCode);
  }
};

} // end namespace llvm

#endif /* LLVM_LIB_TARGET_XTENSA_XTENSAISELLOWERING_H */
