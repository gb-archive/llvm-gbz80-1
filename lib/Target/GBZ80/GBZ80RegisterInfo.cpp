//===-- GBZ80RegisterInfo.cpp - GBZ80 Register Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the GBZ80 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "GBZ80RegisterInfo.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

#include "GBZ80.h"
#include "GBZ80InstrInfo.h"
#include "GBZ80TargetMachine.h"
#include "MCTargetDesc/GBZ80MCTargetDesc.h"

#define GET_REGINFO_TARGET_DESC
#include "GBZ80GenRegisterInfo.inc"

namespace llvm {

GBZ80RegisterInfo::GBZ80RegisterInfo() : GBZ80GenRegisterInfo(0) {}

const uint16_t *
GBZ80RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  // TODO: Interrupt functions save all registers.
  // TODO: The number of parameters for this should probably be stored
  // in a MachineFunctionInfo instance or something.
  if (MF->getFunction().getFunctionType()->getNumParams() <= 1)
    return CSR_0_1_SaveList;
  else
    return CSR_2_SaveList;
}

const uint32_t *
GBZ80RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                      CallingConv::ID CC) const {
  if (MF.getFunction().getFunctionType()->getNumParams() <= 1)
    return CSR_0_1_RegMask;
  else
    return CSR_2_RegMask;
}

BitVector GBZ80RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  // FIXME: Should we reserve A?

  //  Reserve the stack pointer.
  Reserved.set(GB::SP);

  // Reserve the IME.
  Reserved.set(GB::IME);

  return Reserved;
}

const TargetRegisterClass *
GBZ80RegisterInfo::getLargestLegalSuperClass(const TargetRegisterClass *RC,
                                           const MachineFunction &MF) const {
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  if (RC == &GB::R8_GPRRegClass || RC == &GB::R8RegClass) {
    return &GB::R8RegClass;
  }

  if (RC == &GB::R16_HLRegClass || RC == &GB::R16_BCDERegClass ||
      RC == &GB::R16RegClass) {
    return &GB::R16RegClass;
  }

  return RC;
}

/// Fold a frame offset shared between two add instructions into a single one.
static void foldFrameOffset(MachineBasicBlock::iterator &II, int &Offset, unsigned DstReg) {
#if 0
  MachineInstr &MI = *II;
  int Opcode = MI.getOpcode();

  // Don't bother trying if the next instruction is not an add or a sub.
  if ((Opcode != GB::SUBIWRdK) && (Opcode != GB::ADIWRdK)) {
    return;
  }

  // Check that DstReg matches with next instruction, otherwise the instruction
  // is not related to stack address manipulation.
  if (DstReg != MI.getOperand(0).getReg()) {
    return;
  }

  // Add the offset in the next instruction to our offset.
  switch (Opcode) {
  case GB::SUBIWRdK:
    Offset += -MI.getOperand(2).getImm();
    break;
  case GB::ADIWRdK:
    Offset += MI.getOperand(2).getImm();
    break;
  }

  // Finally remove the instruction.
  II++;
  MI.eraseFromParent();
#endif
}

void GBZ80RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                          int SPAdj, unsigned FIOperandNum,
                                          RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected SPAdj value");

  MachineInstr &MI = *II;
  DebugLoc dl = MI.getDebugLoc();
  MachineBasicBlock &MBB = *MI.getParent();
  const MachineFunction &MF = *MBB.getParent();
  const GBZ80TargetMachine &TM = (const GBZ80TargetMachine &)MF.getTarget();
  const TargetInstrInfo &TII = *TM.getSubtargetImpl()->getInstrInfo();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetFrameLowering *TFI = TM.getSubtargetImpl()->getFrameLowering();
  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  // Offset is the offset from the stack pointer on function entry.
  int Offset = MFI.getObjectOffset(FrameIndex);

  // No extra offset here since SP points to the top of the stack.
  Offset += MFI.getStackSize() - TFI->getOffsetOfLocalArea();

  // Don't fold the offset here. We'll do this later after PEI.

  assert(MI.getOpcode() == GB::FRMIDX || MI.getOpcode() == GB::LD8_FI ||
         MI.getOpcode() == GB::ST8_FI);
  MI.getOperand(FIOperandNum).ChangeToImmediate(Offset);
}

unsigned GBZ80RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  // The stack pointer is always the frame register.
  return GB::SP;
}

const TargetRegisterClass *
GBZ80RegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                    unsigned Kind) const {
  return &GB::R16RegClass;
}

const TargetRegisterClass *
GBZ80RegisterInfo::getCrossCopyRegClass(const TargetRegisterClass *RC) const {
  if (RC == &GB::R8_GPRRegClass)
    return &GB::R8RegClass;
  else if (RC == &GB::R16_HLRegClass || RC == &GB::R16_BCDERegClass)
    return &GB::R16RegClass;
  return RC;
}

bool GBZ80RegisterInfo::shouldCoalesce(MachineInstr *MI,
                                       const TargetRegisterClass *SrcRC,
                                       unsigned SubReg,
                                       const TargetRegisterClass *DstRC,
                                       unsigned DstSubReg,
                                       const TargetRegisterClass *NewRC,
                                       LiveIntervals &LIS) const {
  return true;
}

void GBZ80RegisterInfo::splitReg(unsigned Reg,
                               unsigned &LoReg,
                               unsigned &HiReg) const {
    assert(GB::R16RegClass.contains(Reg) && "can only split 16-bit registers");

    LoReg = getSubReg(Reg, GB::sub_lo);
    HiReg = getSubReg(Reg, GB::sub_hi);
}

} // end of namespace llvm
