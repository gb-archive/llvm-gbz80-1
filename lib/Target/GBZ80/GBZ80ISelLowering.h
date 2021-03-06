//===-- GBZ80ISelLowering.h - GBZ80 DAG Lowering Interface ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that GBZ80 uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_GBZ80_ISEL_LOWERING_H
#define LLVM_GBZ80_ISEL_LOWERING_H

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

namespace GBISD {

/// GBZ80 Specific DAG Nodes
enum NodeType {
  /// Start the numbering where the builtin ops leave off.
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  /// Return from subroutine.
  RET_FLAG,
  /// Return from ISR.
  RETI_FLAG,
  /// Represents an abstract call instruction,
  /// which includes a bunch of information.
  CALL,
  /// A wrapper node for TargetConstantPool,
  /// TargetExternalSymbol, and TargetGlobalAddress.
  WRAPPER,
  /// A wrapper for FrameIndex.
  FI,
  LSL,     ///< Logical shift left.
  LSR,     ///< Logical shift right.
  ASR,     ///< Arithmetic shift right.
  ROR,     ///< Bit rotate right.
  ROL,     ///< Bit rotate left.
  LSLLOOP, ///< A loop of single logical shift left instructions.
  LSRLOOP, ///< A loop of single logical shift right instructions.
  ROLLOOP, ///< A loop of single left bit rotate instructions.
  RORLOOP, ///< A loop of single right bit rotate instructions.
  ASRLOOP, ///< A loop of single arithmetic shift right instructions.
  /// GBZ80 conditional branches. Operand 0 is the chain operand, operand 1
  /// is the block to branch if condition is true, operand 2 is the
  /// condition code, and operand 3 is the flag operand produced by a CMP
  /// or TEST instruction.
  BRCOND,
  /// Compare instruction.
  CMP,
  /// Compare with carry instruction.
  CMPC,
  /// Test for zero or minus instruction.
  TST,
  /// Operand 0 and operand 1 are selection variable, operand 2
  /// is condition code and operand 3 is flag operand.
  SELECT_CC,
  /// Operand 0 and operand 1 are selection variable, operand 2
  /// is condition code and operand 3 is flag operand.
  SELECT_BR,
  BR16
};

} // end of namespace GBISD

class GBZ80TargetMachine;

/// Performs target lowering for the GBZ80.
class GBZ80TargetLowering : public TargetLowering {
public:
  explicit GBZ80TargetLowering(GBZ80TargetMachine &TM);

public:
  MVT getScalarShiftAmountTy(const DataLayout &, EVT LHSTy) const override {
    return MVT::i8;
  }

  bool isSelectSupported(SelectSupportKind) const override {
    return false;
  }

  MVT::SimpleValueType getCmpLibcallReturnType() const override {
    return MVT::i8;
  }

  const char *getTargetNodeName(unsigned Opcode) const override;

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;

  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM, Type *Ty,
                             unsigned AS, Instruction *I = nullptr) const override;
#if 0
  bool getPreIndexedAddressParts(SDNode *N, SDValue &Base, SDValue &Offset,
                                 ISD::MemIndexedMode &AM,
                                 SelectionDAG &DAG) const override;
#endif
  bool getPostIndexedAddressParts(SDNode *N, SDNode *Op, SDValue &Base,
                                  SDValue &Offset, ISD::MemIndexedMode &AM,
                                  SelectionDAG &DAG) const override;

  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;

  EVT getSetCCResultType(const DataLayout &DL, LLVMContext &Context,
                         EVT VT) const override;

  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *MBB) const override;
  /*
  ConstraintType getConstraintType(StringRef Constraint) const override;

  ConstraintWeight
  getSingleConstraintMatchWeight(AsmOperandInfo &info,
                                 const char *constraint) const override;

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;

  unsigned getInlineAsmMemConstraint(StringRef ConstraintCode) const override;

  void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

  unsigned getRegisterByName(const char* RegName, EVT VT,
                             SelectionDAG &DAG) const override;
*/
private:
  SDValue getGBZ80Cmp(SDValue LHS, SDValue RHS, ISD::CondCode CC, SDValue &GBCC,
                    SelectionDAG &DAG, SDLoc dl) const;
  SDValue LowerShifts(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerDivRem(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFrameIndex(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerINLINEASM(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerLOAD(SDValue Op, SelectionDAG &DAG) const;

  CCAssignFn *CCAssignFnForReturn(CallingConv::ID CC) const;

  bool CanLowerReturn(CallingConv::ID CallConv,
                      MachineFunction &MF, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
                      SelectionDAG &DAG) const override;
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool isVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          const SDLoc &dl, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  SDValue getPair(SDValue Lo, SDValue Hi, SelectionDAG &DAG) const;

private:
  MachineBasicBlock *insertShift(MachineInstr &MI, MachineBasicBlock *BB) const;
  MachineBasicBlock *insertMul(MachineInstr &MI, MachineBasicBlock *BB) const;
};

} // end namespace llvm

#endif // LLVM_GBZ80_ISEL_LOWERING_H
