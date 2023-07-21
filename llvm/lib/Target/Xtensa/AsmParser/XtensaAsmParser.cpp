//===- XtensaAsmParser.cpp - Parse Xtensa assembly to MCInst instructions -===//
//
//                     The LLVM Compiler Infrastructure
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/XtensaMCExpr.h"
#include "MCTargetDesc/XtensaMCTargetDesc.h"
#include "MCTargetDesc/XtensaTargetStreamer.h"
#include "TargetInfo/XtensaTargetInfo.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

#define DEBUG_TYPE "xtensa-asm-parser"

struct XtensaOperand;

class XtensaAsmParser : public MCTargetAsmParser {
  // Xtensa GNU assembler supports region definitions using
  // .begin and .end directives. Currently only .literal_prefix regions are
  // supported.
  struct RegionInfo {
  public:
    SMLoc Loc;
    StringRef RegionDirectiveName;
    StringRef LiteralPrefixName;
    RegionInfo() = default;
    RegionInfo( SMLoc L, StringRef DirectiveName, StringRef Name = "")
     : Loc(L), RegionDirectiveName(DirectiveName), LiteralPrefixName(Name) {}
  };

  // Stack of active region definitions.
  SmallVector<RegionInfo, 1> RegionInProgress;

  SMLoc getLoc() const { return getParser().getTok().getLoc(); }
 
  XtensaTargetStreamer &getTargetStreamer() {
    MCTargetStreamer &TS = *getParser().getStreamer().getTargetStreamer();
    return static_cast<XtensaTargetStreamer &>(TS);
  }

  // Override MCTargetAsmParser.
  bool ParseDirective(AsmToken DirectiveID) override;
  bool parseRegister(MCRegister &RegNo,
                     SMLoc &StartLoc, SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op,
                                      unsigned Kind) override;

  bool processInstruction(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out,
                          const MCSubtargetInfo *STI);

// Auto-generated instruction matching functions
#define GET_ASSEMBLER_HEADER
#include "XtensaGenAsmMatcher.inc"

  OperandMatchResultTy parseImmediate(OperandVector &Operands);
  OperandMatchResultTy parseRegister(OperandVector &Operands,
                                     StringRef Mnemonic,
                                     bool AllowParens = false, bool SR = false,
                                     bool UR = false);
  OperandMatchResultTy parseOperandWithModifier(OperandVector &Operands);
  bool parseOperand(OperandVector &Operands, StringRef Mnemonic,
                    bool SR = false, bool UR = false);
  bool ParseInstructionWithSR(ParseInstructionInfo &Info, StringRef Name,
                              SMLoc NameLoc, OperandVector &Operands);
  OperandMatchResultTy tryParseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;
  OperandMatchResultTy parsePCRelTarget(OperandVector &Operands);
  bool checkRegister(StringRef Mnemonic, StringRef RegName, MCRegister RegNo);
  bool parseLiteralDirective(SMLoc L);
  bool parseBeginDirective(SMLoc L);
  bool parseEndDirective(SMLoc L);
  void onEndOfFile() override {
    if (!RegionInProgress.empty()) {
      Error(RegionInProgress.back().Loc, ".end of region is not found");
    }
  }

public:
  enum XtensaMatchResultTy {
    Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "XtensaGenAsmMatcher.inc"
#undef GET_OPERAND_DIAGNOSTIC_TYPES
  };

  XtensaAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                  const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {
    Parser.addAliasForDirective(".half", ".2byte");
    Parser.addAliasForDirective(".hword", ".2byte");
    Parser.addAliasForDirective(".word", ".4byte");
    Parser.addAliasForDirective(".dword", ".8byte");
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  bool hasWindowed() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureWindowed];
  };

  bool hasSingleFloat() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureSingleFloat];
  };

  bool hasLoop() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureLoop];
  };

  bool hasMAC16() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureMAC16];
  };

  bool hasBoolean() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureBoolean];
  };

  bool hasDFPAccel() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureDFPAccel];
  };

  bool hasS32C1I() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureS32C1I];
  };

  bool hasTHREADPTR() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureTHREADPTR];
  };

  bool hasExtendedL32R() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureExtendedL32R];
  }

  bool hasATOMCTL() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureATOMCTL];
  }

  bool hasMEMCTL() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureMEMCTL];
  }

  bool hasDebug() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureDebug];
  }

  bool hasException() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureException];
  }

  bool hasHighPriInterrupts() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureHighPriInterrupts];
  }

  bool hasCoprocessor() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureCoprocessor];
  }

  bool hasInterrupt() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureInterrupt];
  }

  bool hasRelocatableVector() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureRelocatableVector];
  }

  bool hasTimerInt() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureTimerInt];
  }

  bool hasPRID() const {
    return getSTI().getFeatureBits()[Xtensa::FeaturePRID];
  }

  bool hasMiscSR() const {
    return getSTI().getFeatureBits()[Xtensa::FeatureMiscSR];
  }
};

// Return true if Expr is in the range [MinValue, MaxValue].
static bool inRange(const MCExpr *Expr, int64_t MinValue, int64_t MaxValue) {
  if (auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
    int64_t Value = CE->getValue();
    return Value >= MinValue && Value <= MaxValue;
  }
  return false;
}

struct XtensaOperand : public MCParsedAsmOperand {

  enum KindTy {
    Token,
    Register,
    Immediate,
  } Kind;

  struct RegOp {
    unsigned RegNum;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  SMLoc StartLoc, EndLoc;
  union {
    StringRef Tok;
    RegOp Reg;
    ImmOp Imm;
  };

  XtensaOperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

public:
  XtensaOperand(const XtensaOperand &o) : MCParsedAsmOperand() {
    Kind = o.Kind;
    StartLoc = o.StartLoc;
    EndLoc = o.EndLoc;
    switch (Kind) {
    case Register:
      Reg = o.Reg;
      break;
    case Immediate:
      Imm = o.Imm;
      break;
    case Token:
      Tok = o.Tok;
      break;
    }
  }

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return false; }

  bool isImm(int64_t MinValue, int64_t MaxValue) const {
    return Kind == Immediate && inRange(getImm(), MinValue, MaxValue);
  }

  bool isImm8() const {
    //The addi instruction maybe expaned to addmi and addi.
    return isImm((-32768 - 128), (32512 + 127));
  }

  bool isImm8_sh8() const {
    return isImm(-32768, 32512) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0xFF) == 0);
  }

  bool isImm12() const { return isImm(-2048, 2047); }

  // Convert MOVI to literal load, when immediate is not in range (-2048, 2047)
  bool isImm12m() const {
    if (Kind == Immediate)
       return true;

    return false;
  }

  bool isOffset4m32() const {
    return isImm(0, 60) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x3) == 0);
  }

  bool isOffset8m8() const { return isImm(0, 255); }

  bool isOffset8m16() const {
    return isImm(0, 510) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x1) == 0);
  }

  bool isOffset8m32() const {
    return isImm(0, 1020) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x3) == 0);
  }

  bool isentry_imm12() const { return isImm(0, 32760); }

  bool isUimm4() const { return isImm(0, 15); }

  bool isUimm5() const { return isImm(0, 31); }

  bool isImm8n_7() const { return isImm(-8, 7); }

  bool isShimm1_31() const { return isImm(1, 31); }

  bool isImm16_31() const { return isImm(16, 31); }

  bool isImm1_16() const { return isImm(1, 16); }

  bool isImm1n_15() const { return (isImm(1, 15) || isImm(-1, -1)); }

  bool isImm32n_95() const { return isImm(-32, 95); }

  bool isImm64n_4n() const {
    return isImm(-64, -4) &&
           ((dyn_cast<MCConstantExpr>(getImm())->getValue() & 0x3) == 0);
  }

  bool isB4const() const {
    if (Kind != Immediate)
      return false;
    if (auto *CE = dyn_cast<MCConstantExpr>(getImm())) {
      int64_t Value = CE->getValue();
      switch (Value) {
      case -1:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 10:
      case 12:
      case 16:
      case 32:
      case 64:
      case 128:
      case 256:
        return true;
      default:
        return false;
      }
    }
    return false;
  }

  bool isB4constu() const {
    if (Kind != Immediate)
      return false;
    if (auto *CE = dyn_cast<MCConstantExpr>(getImm())) {
      int64_t Value = CE->getValue();
      switch (Value) {
      case 32768:
      case 65536:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 10:
      case 12:
      case 16:
      case 32:
      case 64:
      case 128:
      case 256:
        return true;
      default:
        return false;
      }
    }
    return false;
  }

  bool isimm7_22() const { return isImm(7, 22); }

  bool isSelect_2() const { return isImm(0, 1); }

  bool isSelect_4() const { return isImm(0, 3); }

  bool isSelect_8() const { return isImm(0, 7); }

  bool isSelect_16() const { return isImm(0, 16); }

  bool isSelect_256() const { return isImm(0, 255); }

  bool isOffset_16_16() const {
    return isImm(-128, 112) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0xf) == 0);
  }

  bool isOffset_256_8() const {
    return isImm(-1024, 1016) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x7) == 0);
  }

  bool isOffset_256_16() const {
    return isImm(-2048, 2032) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0xf) == 0);
  }

  bool isOffset_256_4() const {
    return isImm(-512, 508) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x3) == 0);
  }

  bool isOffset_128_2() const {
    return isImm(0, 254) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0x1) == 0);
  }

  bool isOffset_128_1() const { return isImm(0, 127); }

  bool isOffset_64_16() const {
    return isImm(-512, 496) &&
           ((cast<MCConstantExpr>(getImm())->getValue() & 0xf) == 0);
  }

  /// getStartLoc - Gets location of the first token of this operand
  SMLoc getStartLoc() const override { return StartLoc; }
  /// getEndLoc - Gets location of the last token of this operand
  SMLoc getEndLoc() const override { return EndLoc; }

  unsigned getReg() const override {
    assert(Kind == Register && "Invalid type access!");
    return Reg.RegNum;
  }

  const MCExpr *getImm() const {
    assert(Kind == Immediate && "Invalid type access!");
    return Imm.Val;
  }

  StringRef getToken() const {
    assert(Kind == Token && "Invalid type access!");
    return Tok;
  }

  void print(raw_ostream &OS) const override {
    switch (Kind) {
    case Immediate:
      OS << *getImm();
      break;
    case Register:
      OS << "<register x";
      OS << getReg() << ">";
      break;
    case Token:
      OS << "'" << getToken() << "'";
      break;
    }
  }

  static std::unique_ptr<XtensaOperand> createToken(StringRef Str, SMLoc S) {
    auto Op = std::make_unique<XtensaOperand>(Token);
    Op->Tok = Str;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static std::unique_ptr<XtensaOperand> createReg(unsigned RegNo, SMLoc S,
                                                  SMLoc E) {
    auto Op = std::make_unique<XtensaOperand>(Register);
    Op->Reg.RegNum = RegNo;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<XtensaOperand> createImm(const MCExpr *Val, SMLoc S,
                                                  SMLoc E) {
    auto Op = std::make_unique<XtensaOperand>(Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const {
    assert(Expr && "Expr shouldn't be null!");
    int64_t Imm = 0;
    bool IsConstant = false;

    if (auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
      IsConstant = true;
      Imm = CE->getValue();
    }

    if (IsConstant)
      Inst.addOperand(MCOperand::createImm(Imm));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  // Used by the TableGen Code
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    addExpr(Inst, getImm());
  }
};

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "XtensaGenAsmMatcher.inc"

unsigned XtensaAsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp,
                                                     unsigned Kind) {
  return Match_InvalidOperand;
}

static SMLoc RefineErrorLoc(const SMLoc Loc, const OperandVector &Operands,
                            uint64_t ErrorInfo) {
  if (ErrorInfo != ~0ULL && ErrorInfo < Operands.size()) {
    SMLoc ErrorLoc = Operands[ErrorInfo]->getStartLoc();
    if (ErrorLoc == SMLoc())
      return Loc;
    return ErrorLoc;
  }
  return Loc;
}

bool XtensaAsmParser::processInstruction(MCInst &Inst, SMLoc IDLoc,
                                         MCStreamer &Out,
                                         const MCSubtargetInfo *STI) {
  Inst.setLoc(IDLoc);
  const unsigned Opcode = Inst.getOpcode();

  switch (Opcode) {
  case Xtensa::ADDI: {
    int64_t Imm = Inst.getOperand(2).getImm();
    // Expand 16-bit immediate in ADDI instruction:
    // ADDI rd, rs, imm - > ADMI rd, rs, (imm & 0xff00); ADDI rd, rd, (imm & 0xff)
    if ((Imm < -128) || (Imm > 127)) {
      unsigned DReg = Inst.getOperand(0).getReg();
      unsigned SReg = Inst.getOperand(1).getReg();
      MCInst ADDMIInst;
      MCInst ADDIInst;
      int64_t ImmHi = Imm & (~((uint64_t)0xff));
      int64_t ImmLo = Imm & 0xff;

      if (ImmLo > 127) {
        ImmHi += 0x100;
        ImmLo = ImmLo - 0x100;
      }

      ADDMIInst.setOpcode(Xtensa::ADDMI);
      ADDMIInst.addOperand(MCOperand::createReg(DReg));
      ADDMIInst.addOperand(MCOperand::createReg(SReg));
      ADDMIInst.addOperand(MCOperand::createImm(ImmHi));
      ADDMIInst.setLoc(IDLoc);

      Out.emitInstruction(ADDMIInst, *STI);

      ADDIInst.setOpcode(Xtensa::ADDI);
      ADDIInst.addOperand(MCOperand::createReg(DReg));
      ADDIInst.addOperand(MCOperand::createReg(DReg));
      ADDIInst.addOperand(MCOperand::createImm(ImmLo));
      ADDIInst.setLoc(IDLoc);

      Inst = ADDIInst;
    }
  } break;
  case Xtensa::L32R: {
    const MCSymbolRefExpr *OpExpr =
        (const MCSymbolRefExpr *)Inst.getOperand(1).getExpr();
    XtensaMCExpr::VariantKind Kind = XtensaMCExpr::VK_Xtensa_None;
    const MCExpr *NewOpExpr = XtensaMCExpr::create(OpExpr, Kind, getContext());
    Inst.getOperand(1).setExpr(NewOpExpr);
  } break;
  case Xtensa::MOVI: {
    XtensaTargetStreamer &TS = this->getTargetStreamer();

    //In the case of asm output, simply pass the representation of
    //the MOVI instruction as is
    if (TS.getStreamer().hasRawTextSupport())
      break;

    //Expand MOVI operand
    if (!Inst.getOperand(1).isExpr()) {
      uint64_t ImmOp64 = Inst.getOperand(1).getImm();
      int32_t Imm = ImmOp64;
      if ((Imm < -2048) || (Imm > 2047)) {
        XtensaTargetStreamer &TS = this->getTargetStreamer();
        MCInst TmpInst;
        TmpInst.setLoc(IDLoc);
        TmpInst.setOpcode(Xtensa::L32R);
        const MCExpr *Value = MCConstantExpr::create(ImmOp64, getContext());
        MCSymbol *Sym = getContext().createTempSymbol();
        const MCExpr *Expr = MCSymbolRefExpr::create(
            Sym, MCSymbolRefExpr::VK_None, getContext());
        const MCExpr *OpExpr = XtensaMCExpr::create(
            Expr, XtensaMCExpr::VK_Xtensa_None, getContext());
        TmpInst.addOperand(Inst.getOperand(0));
        MCOperand Op1 = MCOperand::createExpr(OpExpr);
        TmpInst.addOperand(Op1);
        TS.emitLiteralLabel(Sym, IDLoc);
        TS.emitLiteral(Value, IDLoc);
        Inst = TmpInst;
      }
    } else {
      MCInst TmpInst;
      TmpInst.setLoc(IDLoc);
      TmpInst.setOpcode(Xtensa::L32R);
      const MCExpr *Value = Inst.getOperand(1).getExpr();
      MCSymbol *Sym = getContext().createTempSymbol();
      const MCExpr *Expr =
          MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, getContext());
      const MCExpr *OpExpr = XtensaMCExpr::create(
          Expr, XtensaMCExpr::VK_Xtensa_None, getContext());
      TmpInst.addOperand(Inst.getOperand(0));
      MCOperand Op1 = MCOperand::createExpr(OpExpr);
      TmpInst.addOperand(Op1);
      Inst = TmpInst;
      TS.emitLiteralLabel(Sym, IDLoc);
      TS.emitLiteral(Value, IDLoc);
    }
  } break;
  case Xtensa::SRLI: {
    uint32_t ImmOp32 = static_cast<uint32_t>(Inst.getOperand(2).getImm());
    int64_t Imm = ImmOp32;
    if (Imm >= 16 && Imm <= 31) {
      MCInst TmpInst;
      TmpInst.setLoc(IDLoc);
      TmpInst.setOpcode(Xtensa::EXTUI);
      TmpInst.addOperand(Inst.getOperand(0));
      TmpInst.addOperand(Inst.getOperand(1));
      TmpInst.addOperand(MCOperand::createImm(Imm));
      TmpInst.addOperand(MCOperand::createImm(32 - Imm));
      Inst = TmpInst;
    }
  } break;
  default:
    break;
  }
  return true;
}

bool XtensaAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                              OperandVector &Operands,
                                              MCStreamer &Out,
                                              uint64_t &ErrorInfo,
                                              bool MatchingInlineAsm) {
  MCInst Inst;
  auto Result =
      MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm);

  switch (Result) {
  default:
    break;
  case Match_Success:
    processInstruction(Inst, IDLoc, Out, STI);
    Inst.setLoc(IDLoc);
    Out.emitInstruction(Inst, getSTI());
    return false;
  case Match_MissingFeature:
    return Error(IDLoc, "instruction use requires an option to be enabled");
  case Match_MnemonicFail:
    return Error(IDLoc, "unrecognized instruction mnemonic");
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(ErrorLoc, "too few operands for instruction");

      ErrorLoc = ((XtensaOperand &)*Operands[ErrorInfo]).getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = IDLoc;
    }
    return Error(ErrorLoc, "invalid operand for instruction");
  }
  case Match_InvalidImm8:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-32896, 32639]");
  case Match_InvalidImm8_sh8:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-32768, 32512], first 8 bits "
                 "should be zero");
  case Match_InvalidB4const:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected b4const immediate");
  case Match_InvalidB4constu:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected b4constu immediate");
  case Match_InvalidImm12:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-2048, 2047]");
  case Match_InvalidImm1_16:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [1, 16]");
  case Match_InvalidImm1n_15:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-1, 15] except 0");
  case Match_InvalidImm32n_95:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-32, 95] except 0");
  case Match_InvalidImm64n_4n:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-64, -4]");
  case Match_InvalidImm8n_7:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-8, 7]");
  case Match_InvalidShimm1_31:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [1, 31]");
  case Match_InvalidUimm4:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 15]");
  case Match_InvalidUimm5:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 31]");
  case Match_InvalidOffset8m8:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 255]");
  case Match_InvalidOffset8m16:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 510], first bit "
                 "should be zero");
  case Match_InvalidOffset8m32:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 1020], first 2 bits "
                 "should be zero");
  case Match_InvalidOffset4m32:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 60], first 2 bits "
                 "should be zero");
  case Match_Invalidentry_imm12:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 32760]");
  case Match_Invalidimm7_22:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [7, 22]");
  case Match_InvalidSelect_2:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 1]");
  case Match_InvalidSelect_4:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 3]");
  case Match_InvalidSelect_8:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 7]");
  case Match_InvalidSelect_16:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 15]");
  case Match_InvalidSelect_256:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 255]");
  case Match_InvalidOffset_16_16:
    return Error(
        RefineErrorLoc(IDLoc, Operands, ErrorInfo),
        "expected immediate in range [-128, 112], first 4 bits should be zero");
  case Match_InvalidOffset_256_8:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-1024, 1016], first 3 bits "
                 "should be zero");
  case Match_InvalidOffset_256_16:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [-2048, 2032], first 4 bits "
                 "should be zero");
  case Match_InvalidOffset_256_4:
    return Error(
        RefineErrorLoc(IDLoc, Operands, ErrorInfo),
        "expected immediate in range [-512, 508], first 2 bits should be zero");
  case Match_InvalidOffset_128_2:
    return Error(
        RefineErrorLoc(IDLoc, Operands, ErrorInfo),
        "expected immediate in range [0, 254], first bit should be zero");
  case Match_InvalidOffset_128_1:
    return Error(RefineErrorLoc(IDLoc, Operands, ErrorInfo),
                 "expected immediate in range [0, 127]");
  case Match_InvalidOffset_64_16:
    return Error(
        RefineErrorLoc(IDLoc, Operands, ErrorInfo),
        "expected immediate in range [-512, 496], first 4 bits should be zero");
  }

  report_fatal_error("Unknown match type detected!");
}

OperandMatchResultTy
XtensaAsmParser::parsePCRelTarget(OperandVector &Operands) {
  MCAsmParser &Parser = getParser();
  LLVM_DEBUG(dbgs() << "parsePCRelTarget\n");

  SMLoc S = getLexer().getLoc();

  // Expressions are acceptable
  const MCExpr *Expr = nullptr;
  if (Parser.parseExpression(Expr)) {
    // We have no way of knowing if a symbol was consumed so we must ParseFail
    return MatchOperand_ParseFail;
  }

  // Currently not support constants
  if (Expr->getKind() == MCExpr::ExprKind::Constant) {
    Error(getLoc(), "unknown operand");
    return MatchOperand_ParseFail;
  }

  Operands.push_back(XtensaOperand::createImm(Expr, S, getLexer().getLoc()));
  return MatchOperand_Success;
}

// Attempts to match Name as a register (either using the default name or
// alternative ABI names), setting RegNo to the matching register. Upon
// failure, returns true and sets RegNo to 0
static bool matchRegisterNameHelper(MCRegister &RegNo, StringRef Name) {
  RegNo = MatchRegisterName(Name);

  if (RegNo == Xtensa::NoRegister)
    RegNo = MatchRegisterAltName(Name.lower());

  if (RegNo == Xtensa::NoRegister)
    RegNo = MatchRegisterAltName(Name.upper());

  return RegNo == Xtensa::NoRegister;
}

OperandMatchResultTy XtensaAsmParser::tryParseRegister(MCRegister &RegNo,
                                                       SMLoc &StartLoc,
                                                       SMLoc &EndLoc) {
  if (parseRegister(RegNo, StartLoc, EndLoc))
    return MatchOperand_NoMatch;
  return MatchOperand_Success;
}

bool XtensaAsmParser::parseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                                    SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  StartLoc = Tok.getLoc();
  EndLoc = Tok.getEndLoc();
  RegNo = 0;
  StringRef Name = getLexer().getTok().getIdentifier();

  if (!MatchRegisterName(Name) && !MatchRegisterAltName(Name)) {
    getParser().Lex(); // Eat identifier token.
    return false;
  }

  return Error(StartLoc, "invalid register name");
}

OperandMatchResultTy XtensaAsmParser::parseRegister(OperandVector &Operands,
                                                    StringRef Mnemonic,
                                                    bool AllowParens, bool SR,
                                                    bool UR) {
  SMLoc FirstS = getLoc();
  bool HadParens = false;
  AsmToken Buf[2];
  std::string RegName = "";
  MCRegister RegNo = 0;
  int64_t Num;
  bool IsIdentifier = false;

  // If this a parenthesised register name is allowed, parse it atomically
  if (AllowParens && getLexer().is(AsmToken::LParen)) {
    size_t ReadCount = getLexer().peekTokens(Buf);
    if (ReadCount == 2 && Buf[1].getKind() == AsmToken::RParen) {
      if ((Buf[0].getKind() == AsmToken::Integer) && (!SR) && (!UR))
        return MatchOperand_NoMatch;
      HadParens = true;
      getParser().Lex(); // Eat '('
    }
  }

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::Integer:
  case AsmToken::LParen:
    if ((!SR) && (!UR))
      return MatchOperand_NoMatch;
    const MCExpr *Res;

    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;

    if (!Res->evaluateAsAbsolute(Num))
      return MatchOperand_NoMatch;

    // Parse case when we expect UR operand as special case,
    // because SR and UR registers may have the same number
    // and such situation may lead to confilct
    if (UR) {
      if (Num == 0)
        RegName = "GPIO_OUT";
      if (Num == 230)
        RegName = "EXPSTATE";
      if (Num == 231)
        RegName = "THREADPTR";
      if (Num == 232)
        RegName = "FCR";
      if (Num == 233)
        RegName = "FSR";
      if (Num == 234)
        RegName = "F64R_LO";
      if (Num == 235)
        RegName = "F64R_HI";
      if (Num == 236)
        RegName = "F64S";
    } else
      RegName = std::to_string(Num);

    matchRegisterNameHelper(RegNo, RegName);
    break;
  case AsmToken::Identifier:
    IsIdentifier = true;
    RegName = getLexer().getTok().getIdentifier().str();
    matchRegisterNameHelper(RegNo, RegName);
    break;
  }

  if (RegNo == 0) {
    if (HadParens)
      getLexer().UnLex(Buf[0]);
    return MatchOperand_NoMatch;
  }

  if (!checkRegister(Mnemonic.lower(), RegName, RegNo))
    return MatchOperand_NoMatch;

  if (HadParens)
    Operands.push_back(XtensaOperand::createToken("(", FirstS));
  SMLoc S = getLoc();
  SMLoc E = getParser().getTok().getEndLoc();

  if (IsIdentifier)
    getLexer().Lex();

  Operands.push_back(XtensaOperand::createReg(RegNo, S, E));

  if (HadParens) {
    getParser().Lex(); // Eat ')'
    Operands.push_back(XtensaOperand::createToken(")", getLoc()));
  }

  return MatchOperand_Success;
}

OperandMatchResultTy XtensaAsmParser::parseImmediate(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E;
  const MCExpr *Res;

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::LParen:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Tilde:
  case AsmToken::Integer:
  case AsmToken::String:
    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;
    break;
  case AsmToken::Identifier: {
    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;
    break;
  }
  case AsmToken::Percent:
    return parseOperandWithModifier(Operands);
  }

  E = SMLoc::getFromPointer(S.getPointer() - 1);
  Operands.push_back(XtensaOperand::createImm(Res, S, E));
  return MatchOperand_Success;
}

OperandMatchResultTy
XtensaAsmParser::parseOperandWithModifier(OperandVector &Operands) {
  return MatchOperand_ParseFail;
}

/// Looks at a token type and creates the relevant operand
/// from this information, adding to Operands.
/// If operand was parsed, returns false, else true.
bool XtensaAsmParser::parseOperand(OperandVector &Operands, StringRef Mnemonic,
                                   bool SR, bool UR) {
  // Check if the current operand has a custom associated parser, if so, try to
  // custom parse the operand, or fallback to the general approach.
  OperandMatchResultTy ResTy = MatchOperandParserImpl(Operands, Mnemonic);
  if (ResTy == MatchOperand_Success)
    return false;

  // If there wasn't a custom match, try the generic matcher below. Otherwise,
  // there was a match, but an error occurred, in which case, just return that
  // the operand parsing failed.
  if (ResTy == MatchOperand_ParseFail)
    return true;

  // Attempt to parse token as register
  if (parseRegister(Operands, Mnemonic, true, SR, UR) == MatchOperand_Success)
    return false;

  // Attempt to parse token as an immediate
  if (parseImmediate(Operands) == MatchOperand_Success) {
    return false;
  }

  // Finally we have exhausted all options and must declare defeat.
  Error(getLoc(), "unknown operand");
  return true;
}

bool XtensaAsmParser::ParseInstructionWithSR(ParseInstructionInfo &Info,
                                             StringRef Name, SMLoc NameLoc,
                                             OperandVector &Operands) {
  bool IsSR = Name.startswith("wsr") || Name.startswith("rsr") ||
              Name.startswith("xsr");
  bool IsUR = Name.startswith("wur") || Name.startswith("rur");

  if ((Name.startswith("wsr.") || Name.startswith("rsr.") ||
       Name.startswith("xsr.") || Name.startswith("rur.") ||
       Name.startswith("wur.")) &&
      (Name.size() > 4)) {
    // Parse case when instruction name is concatenated with SR register
    // name, like "wsr.sar a1"

    // First operand is token for instruction
    Operands.push_back(XtensaOperand::createToken(Name.take_front(3), NameLoc));

    StringRef RegName = Name.drop_front(4);
    MCRegister RegNo = 0;

    matchRegisterNameHelper(RegNo, RegName);

    if (!checkRegister(Name.lower(), RegName, RegNo)) {
      Error(NameLoc, "invalid register name");
      return true;
    }

    // Parse operand
    if (parseOperand(Operands, Name))
      return true;

    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    Operands.push_back(XtensaOperand::createReg(RegNo, S, E));
  } else {
    // First operand is token for instruction
    Operands.push_back(XtensaOperand::createToken(Name, NameLoc));

    // Parse first operand
    if (parseOperand(Operands, Name))
      return true;

    if (!getLexer().is(AsmToken::Comma)) {
      SMLoc Loc = getLexer().getLoc();
      getParser().eatToEndOfStatement();
      return Error(Loc, "unexpected token");
    }

    getLexer().Lex();

    // Parse second operand
    if (parseOperand(Operands, Name, IsSR, IsUR))
      return true;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }

  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

bool XtensaAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                       StringRef Name, SMLoc NameLoc,
                                       OperandVector &Operands) {
  if (Name.startswith("wsr") || Name.startswith("rsr") ||
      Name.startswith("xsr") || Name.startswith("rur") ||
      Name.startswith("wur")) {
    return ParseInstructionWithSR(Info, Name, NameLoc, Operands);
  }

  // First operand is token for instruction
  Operands.push_back(XtensaOperand::createToken(Name, NameLoc));

  // If there are no more operands, then finish
  if (getLexer().is(AsmToken::EndOfStatement))
    return false;

  // Parse first operand
  if (parseOperand(Operands, Name))
    return true;

  // Parse until end of statement, consuming commas between operands
  while (getLexer().is(AsmToken::Comma)) {
    // Consume comma token
    getLexer().Lex();

    // Parse next operand
    if (parseOperand(Operands, Name))
      return true;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }

  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

bool XtensaAsmParser::parseLiteralDirective(SMLoc L) {
  MCAsmParser &Parser = getParser();
  MCSymbol *Sym;
  const MCExpr *Value;
  SMLoc LiteralLoc = getLexer().getLoc();
  XtensaTargetStreamer &TS = this->getTargetStreamer();

  if (Parser.parseExpression(Value))
    return true;

  const MCSymbolRefExpr *SE = dyn_cast<MCSymbolRefExpr>(Value);
  if (!SE)
    return Error(LiteralLoc, "literal label must be a symbol");
  else {
    Sym = getContext().getOrCreateSymbol(SE->getSymbol().getName());
  }

  if (Parser.parseToken(AsmToken::Comma, "expected comma"))
    return true;

  SMLoc OpcodeLoc = getLexer().getLoc();
  if (parseOptionalToken(AsmToken::EndOfStatement))
    return Error(OpcodeLoc, "expected value");

  if (Parser.parseExpression(Value))
    return true;

  TS.emitLiteralLabel(Sym, LiteralLoc);
  TS.emitLiteral(Value, LiteralLoc);

  return false;
}

bool XtensaAsmParser::parseBeginDirective(SMLoc L) {
  MCAsmParser &Parser = getParser();
  const MCExpr *Value;
  SMLoc BeginLoc = getLexer().getLoc();
  XtensaTargetStreamer &TS = this->getTargetStreamer();

  if (Parser.parseExpression(Value))
    return true;

  const MCSymbolRefExpr *SE = dyn_cast<MCSymbolRefExpr>(Value);
  if (!SE)
    return Error(BeginLoc, "region option must be a symbol");

  StringRef RegionDirectiveName = SE->getSymbol().getName();

  if (RegionDirectiveName == "literal_prefix") {

    SMLoc OpcodeLoc = getLexer().getLoc();
    if (parseOptionalToken(AsmToken::EndOfStatement))
      return Error(OpcodeLoc, "expected literal section name");

    if (Parser.parseExpression(Value))
      return true;

    OpcodeLoc = getLexer().getLoc();
    SE = dyn_cast<MCSymbolRefExpr>(Value);
    if (!SE)
      return Error(OpcodeLoc, "literal_prefix name must be a symbol");

    StringRef LiteralPrefixName = SE->getSymbol().getName();
    TS.setLiteralSectionPrefix(LiteralPrefixName);
    RegionInProgress.emplace_back(BeginLoc, RegionDirectiveName, LiteralPrefixName);
  } else {
    return Error(BeginLoc, "unsupported region directive");
  }

  return false;
}

bool XtensaAsmParser::parseEndDirective(SMLoc L) {
  MCAsmParser &Parser = getParser();
  const MCExpr *Value;
  SMLoc EndLoc = getLexer().getLoc();
  XtensaTargetStreamer &TS = this->getTargetStreamer();

  if (Parser.parseExpression(Value))
    return true;

  const MCSymbolRefExpr *SE = dyn_cast<MCSymbolRefExpr>(Value);
  if (!SE)
    return Error(EndLoc, "region option must be a symbol");

  StringRef RegionDirectiveName = SE->getSymbol().getName();

  if (RegionInProgress.empty())
    return Error(EndLoc, ".end of the region without .begin");
  else {
    RegionInfo Region = RegionInProgress.pop_back_val();
   
    if (RegionInProgress.empty())
      TS.setLiteralSectionPrefix("");
    else
      TS.setLiteralSectionPrefix(Region.LiteralPrefixName);
   
    if (RegionDirectiveName != Region.RegionDirectiveName) {
      return Error(EndLoc, ".end directive differs from .begin directive");
    }
  }

  // Error: does not match begin literal_prefix
  return false;
}

bool XtensaAsmParser::ParseDirective(AsmToken DirectiveID) {
  StringRef IDVal = DirectiveID.getString();
  SMLoc Loc = getLexer().getLoc();

  if (IDVal == ".literal_position") {
    // We currently push literals in literal section which name depends on name
    // of the current section.
    // So, assume that we may skip this directive.
    return false;
  }

  if (IDVal == ".literal") {
    parseLiteralDirective(Loc);
    return false;
  }

  if (IDVal == ".begin") {
    parseBeginDirective(Loc);
    return false;
  }

  if (IDVal == ".end") {
    parseEndDirective(Loc);
    return false;
  }

  return true;
}
  
// Verify SR and UR
bool XtensaAsmParser::checkRegister(StringRef Mnemonic, StringRef RegName,
                                    MCRegister RegNo) {
  StringRef CPU = getSTI().getCPU();
  unsigned NumIntLevels = 0;
  unsigned NumTimers = 0;
  unsigned NumMiscSR = 0;
  bool IsESP32 = false;
  bool IsESP32S2 = false;
  bool IsESP32S3 = false;
  bool Res = true;
  bool IsWSR = Mnemonic.startswith("wsr");
  bool IsRSR = Mnemonic.startswith("rsr");

  // Assume that CPU is esp32 by default
  if ((CPU == "esp32") || (CPU == "")) {
    NumIntLevels = 6;
    NumTimers = 3;
    NumMiscSR = 4;
    IsESP32 = true;
  } else if (CPU == "esp32s2") {
    NumIntLevels = 6;
    NumTimers = 3;
    NumMiscSR = 4;
    IsESP32S2 = true;
  } else if (CPU == "esp32s3") {
    NumIntLevels = 6;
    NumTimers = 3;
    NumMiscSR = 4;
    IsESP32S3 = true;
  } else if (CPU == "esp8266") {
    NumIntLevels = 2;
    NumTimers = 1;
  }

  switch (RegNo) {
  case Xtensa::LBEG:
  case Xtensa::LEND:
  case Xtensa::LCOUNT:
    Res = hasLoop();
    break;
  case Xtensa::BREG:
    Res = hasBoolean();
    break;
  case Xtensa::LITBASE:
    Res = hasExtendedL32R();
    break;
  case Xtensa::SCOMPARE1:
    Res = hasS32C1I();
    break;
  case Xtensa::ACCLO:
  case Xtensa::ACCHI:
  case Xtensa::M0:
  case Xtensa::M1:
  case Xtensa::M2:
  case Xtensa::M3:
    Res = hasMAC16();
    break;
  case Xtensa::WINDOWBASE:
  case Xtensa::WINDOWSTART:
    Res = hasWindowed();
    break;
  case Xtensa::IBREAKENABLE:
  case Xtensa::IBREAKA0:
  case Xtensa::IBREAKA1:
  case Xtensa::DBREAKA0:
  case Xtensa::DBREAKA1:
  case Xtensa::DBREAKC0:
  case Xtensa::DBREAKC1:
  case Xtensa::ICOUNT:
  case Xtensa::ICOUNTLEVEL:
    Res = hasDebug();
    break;
  case Xtensa::DEBUGCAUSE:
    Res = hasDebug();
    Res = Res & IsRSR;
    break;
  case Xtensa::ATOMCTL:
    Res = hasATOMCTL();
    break;
  case Xtensa::MEMCTL:
    Res = hasMEMCTL();
    break;
  case Xtensa::EPC1:
    Res = hasException();
    break;
  case Xtensa::EPC2:
  case Xtensa::EPC3:
  case Xtensa::EPC4:
  case Xtensa::EPC5:
  case Xtensa::EPC6:
  case Xtensa::EPC7:
    Res = hasHighPriInterrupts();
    Res = Res & (NumIntLevels >= (RegNo - Xtensa::EPC1));
    break;
  case Xtensa::EPS2:
  case Xtensa::EPS3:
  case Xtensa::EPS4:
  case Xtensa::EPS5:
  case Xtensa::EPS6:
  case Xtensa::EPS7:
    Res = hasHighPriInterrupts();
    Res = Res & (NumIntLevels > (RegNo - Xtensa::EPS2));
    break;
  case Xtensa::EXCSAVE1:
    Res = hasException();
    break;
  case Xtensa::EXCSAVE2:
  case Xtensa::EXCSAVE3:
  case Xtensa::EXCSAVE4:
  case Xtensa::EXCSAVE5:
  case Xtensa::EXCSAVE6:
  case Xtensa::EXCSAVE7:
    Res = hasHighPriInterrupts();
    Res = Res & (NumIntLevels >= (RegNo - Xtensa::EXCSAVE1));
    break;
  case Xtensa::DEPC:
  case Xtensa::EXCCAUSE:
  case Xtensa::EXCVADDR:
    Res = hasException();
    break;
  case Xtensa::CPENABLE:
    Res = hasCoprocessor();
    break;
  case Xtensa::VECBASE:
    Res = hasRelocatableVector();
    break;
  case Xtensa::CCOUNT:
    Res = hasTimerInt();
    Res &= (NumTimers > 0);
    break;
  case Xtensa::CCOMPARE0:
  case Xtensa::CCOMPARE1:
  case Xtensa::CCOMPARE2:
    Res = hasTimerInt();
    Res &= (NumTimers > (RegNo - Xtensa::CCOMPARE0));
    break;
  case Xtensa::PRID:
    Res = hasPRID();
    Res = Res & IsRSR;
    break;
  case Xtensa::INTERRUPT:
    // INTSET mnemonic is wrtite-only
    // INTERRUPT mnemonic is read-only
    if (RegName.startswith("intset")) {
      if (!IsWSR)
        Res = false;
    } else if (!IsRSR) {
      Res = false;
    }
    Res = Res & hasInterrupt();
    break;
  case Xtensa::INTCLEAR:
    Res = hasInterrupt();
    Res = Res & IsWSR;
    break;
  case Xtensa::INTENABLE:
    Res = hasInterrupt();
    break;
  case Xtensa::MISC0:
  case Xtensa::MISC1:
  case Xtensa::MISC2:
  case Xtensa::MISC3:
    Res = hasMiscSR();
    Res &= (NumMiscSR > (RegNo - Xtensa::MISC0));
    break;
  case Xtensa::THREADPTR:
    Res = hasTHREADPTR();
    break;
  case Xtensa::GPIO_OUT:
    Res = IsESP32S2 || IsESP32S3;
    break;
  case Xtensa::EXPSTATE:
    Res = IsESP32;
    break;
  case Xtensa::FCR:
  case Xtensa::FSR:
    Res = hasSingleFloat();
    break;
  case Xtensa::F64R_LO:
  case Xtensa::F64R_HI:
  case Xtensa::F64S:
    Res = hasDFPAccel();
    break;
  case Xtensa::NoRegister:
    Res = false;
  }

  return Res;
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeXtensaAsmParser() {
  RegisterMCAsmParser<XtensaAsmParser> X(getTheXtensaTarget());
}
