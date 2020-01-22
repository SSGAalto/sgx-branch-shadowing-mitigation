//===-X86BranchConversion.cpp-Convert conditional branches to unconditional-===//
//
//                     The LLVM Compiler Infrastructure
//
// Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
// Author: Hans Liljestrand, Shohreh Hosseinzadeh
// Technical report available at: https://arxiv.org/abs/1808.06478
//===----------------------------------------------------------------------===//

#include "X86.h"
#include "X86InstrBuilder.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include <list>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

//#define DISABLE_BRANCH_CONVERSION

#define LANE_DEBUG(x) do {              \
        errs() << "\t" << KCYN;         \
        x;                              \
        errs() << KNRM;                 \
} while(0)
#define BC_DEBUG(x) do {                \
        x;                              \
} while(0)
#undef LANE_DEBUG
#define LANE_DEBUG(x)
#undef BC_DEBUG
#define BC_DEBUG(x)

using namespace llvm;

#define DEBUG_TYPE "x86-branch-conversion"

namespace {
struct blockLane {
  MachineBasicBlock *currentMBB;
  MachineBasicBlock *DestMBB;
  bool taken;

  blockLane(MachineBasicBlock *a, MachineBasicBlock *b, bool c) {
    currentMBB = a;
    DestMBB = b;
    taken = c;
  }
};

static cl::opt<bool> EnableBBDummyInstr("x86-bc-dummy-instr",
                                        cl::desc("Use dummy instruction in skip-trampolines."),
                                        cl::init(false), cl::Hidden);

class X86BranchConversion : public MachineFunctionPass {
private:
  static unsigned int getCorrespondingMovOpcode(MachineInstr &MI);

  bool replaceUnconditionalJump(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineInstrBundleIterator <MachineInstr> &iter,
                                MachineBasicBlock *fallThrough,
                                struct blockLane *takenLane);

  bool replaceConditionalBranch(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineInstrBundleIterator<MachineInstr, false> iter,
                                MachineBasicBlock *fallThrough,
                                struct blockLane *takenLane);

  bool replaceIndirectJump(MachineFunction &MF, MachineBasicBlock &MBB,
                           MachineInstrBundleIterator <MachineInstr> &iter,
                           MachineBasicBlock *fallThrough,
                           struct blockLane *takenLane);

  bool replaceNoBranchBlock(MachineFunction &MF, MachineBasicBlock &MBB,
                            MachineInstrBundleIterator<MachineInstr, false> iter,
                            MachineBasicBlock *fallThrough,
                            struct blockLane *takenLane);

  void addDummyInstructions(MachineBasicBlock *realBlock, MachineBasicBlock *trampolineBlock);

  MachineBasicBlock *CreateNewBBonTrampoline(MachineBasicBlock &MBB, MachineFunction &MF,
                                             MachineBasicBlock *destOnCode);

  struct blockLane *addBBtoBlockLane(MachineBasicBlock *MBB, MachineBasicBlock *DestMBB, bool taken);

  inline void updateLane(struct blockLane *lane, MachineBasicBlock *a, MachineBasicBlock *b, bool c);

  // Debug Functions
  static std::string getOperandType(MachineOperand &op);

  inline void dump_function(const MachineFunction &MF);

  inline void dump_MBB(const MachineBasicBlock &MBB);

  inline void dump_MI_with_operands(const std::string &name, const MachineInstr *MI);

  inline void dump_target(const std::string &name, const MachineBasicBlock *MBB);

public:
  static char ID;

  X86BranchConversion() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return "X86 Branch Conversion"; }

  bool doInitialization(Module &M) override;

  bool runOnMachineFunction(MachineFunction &F) override;

private:
  static constexpr const unsigned targetRegOpcode = X86::R15;
  static constexpr const unsigned tmpReg = X86::R13; // temporary register used by conditional move
  static constexpr const unsigned tmpReg8 = X86::R13B; // temporary register used by conditional move

  const TargetMachine *TM;
  const X86Subtarget *STI;
  const X86InstrInfo *TII;

  std::list<struct blockLane *> *takenBlocks;
  std::set<MachineBasicBlock *> *processedMBBs;
};

} // end anonymous namespace

FunctionPass *llvm::createX86BranchConversionPass() {
  return new X86BranchConversion();
}

char X86BranchConversion::ID = 0;

bool X86BranchConversion::doInitialization(Module &M) {
  return false;
}

bool X86BranchConversion::runOnMachineFunction(MachineFunction &MF) {
  DEBUG(dbgs() << getPassName() << '\n');

  TM = &MF.getTarget();
  STI = &MF.getSubtarget<X86Subtarget>();
  TII = STI->getInstrInfo();

  takenBlocks = new std::list<struct blockLane *>();
  processedMBBs = new std::set<MachineBasicBlock *>();

  auto &entry = MF.front(); // TMP: store this so we can jump over trampolines

  BC_DEBUG(dump_function(MF));

  // Inset a default startup blockLane
  addBBtoBlockLane(nullptr, nullptr, true);

  // Use manual iterator to better control iteration while inserting new stuff
  auto iMBB = MF.begin();
  while (iMBB != MF.end()) {
    auto &MBB = *iMBB;
    ++iMBB; // Move iterator forward

    BC_DEBUG(errs() << KRED << "#---------------------------------------------------------------------\n" << KNRM);
    BC_DEBUG(dump_MBB(MBB));

#ifdef DISABLE_BRANCH_CONVERSION
    continue;
#endif

    // This block might jump to itself, so immediately mark it as processed to handle this properly
    processedMBBs->insert(&MBB);

    MachineBasicBlock *originalFallThrough = MBB.getFallThrough();

    // Insert new block, unless iterator is at tend
    MachineBasicBlock *fakeBlock = nullptr;
    if (!(iMBB == MF.end() && MBB.isReturnBlock())) {
      // Only if this is the last block AND has a return can we omit the jump-block
      fakeBlock = MF.CreateMachineBasicBlock();
      MF.insert(iMBB, fakeBlock); // Insert before next element (between MBB and iMBB)
      BuildMI(fakeBlock, DebugLoc(), TII->get(X86::JMP64r), targetRegOpcode);
    }

    // We only need one lane for taking, so we can terminate any extra ones
    struct blockLane *takenLane = nullptr;

    // We also always need to update incoming blocks, so lets do that here also
    LANE_DEBUG(errs() << "!!! updating " << takenBlocks->size() << " lanes\n");
    for (auto laneIterator = takenBlocks->begin(); laneIterator != takenBlocks->end(); laneIterator++) {
      auto lane = *laneIterator;

      if (lane->taken || lane->DestMBB == &MBB) {
        if (lane->currentMBB != nullptr) {
          // Put in the jump from the previous trampoline to this MBB (e.g., zBlockN -> BlockN);
          LANE_DEBUG(errs() << "!!! updating lane to ");
          LANE_DEBUG(errs().write_escaped(MBB.getName()) << "\n");
          BuildMI(lane->currentMBB, DebugLoc(), TII->get(X86::JMP_4)).addMBB(&MBB);
          lane->currentMBB->addSuccessor(&MBB);
        }

        if (takenLane == nullptr) {
          LANE_DEBUG(errs() << "!!! storing one taken lane\n");
          takenLane = lane;
        } else {
          // We can remove this lane if we already have one that is going to update the taken path
          laneIterator = takenBlocks->erase(laneIterator); // Moves iterator forward!
          laneIterator--;
          LANE_DEBUG(errs() << "!!! removing duplicate lane\n");
        }
      } else if (lane->currentMBB != nullptr) {
        LANE_DEBUG(errs() << "!!! updating skip lane\n");
        assert(fakeBlock != nullptr && "assuming this will never happen for last block");

        // FIXME: we could remove duplicate lanes with the same destination, but should work as is.

        if (fakeBlock != nullptr) {
          auto nextZBlock = CreateNewBBonTrampoline(MBB, MF, nullptr);

          if (EnableBBDummyInstr)
            addDummyInstructions(&MBB, lane->currentMBB);

          //BuildMI(lane->currentMBB, DebugLoc(), TII->get(X86::MOV64ri), targetRegOpcode).addMBB(nextZBlock);
          BuildMI(lane->currentMBB, DebugLoc(), TII->get(X86::LEA64r), targetRegOpcode)
              .addReg(X86::RIP)
              .addImm(0)
              .addReg(0)
              .addMBB(nextZBlock)
              .addReg(0);
          BuildMI(lane->currentMBB, DebugLoc(), TII->get(X86::JMP_4)).addMBB(fakeBlock);
          lane->currentMBB->addSuccessor(fakeBlock);

          // Update our lane to point to the zSkipMBB
          lane->currentMBB = nextZBlock;
        }
      } else {
        // This lane is dead, let's remove it
        laneIterator = takenBlocks->erase(laneIterator); // Moves iterator forward!
        laneIterator--;
        LANE_DEBUG(errs() << "!!! removing dead lane\n");
      }
    }
    LANE_DEBUG(errs() << "!!! " << takenBlocks->size() << " lanes left\n");

    auto pos = --(MBB.end()); // get iterator to last instruction

    if (takenLane == nullptr) {
      // We have no taken lane, this could be due to an indirect jump that lands directly into this
      // block. So lets just create a empty lane to go forward.
      LANE_DEBUG(errs() << "!!! found a non-taken lane, adding new lane\n");
      takenLane = addBBtoBlockLane(nullptr, nullptr, true);
    }

    if (pos == MBB.end()) {
      errs() << "\t\t\t!!!!!!!!!!!!!!!!!!!!!!WARNING: We seem to have an empty block: ";
      continue;
    }

    if (pos->isUnconditionalBranch()) {
      auto tmp_iter = pos;

      // Check if we have a switch-type thing with je + jmp
      if (pos != MBB.begin() && (--tmp_iter)->isConditionalBranch()) { // if so, split it into to MBBs
        BC_DEBUG(errs() << KBLU << "\t\tsplitting je+jmp into two blocks\n" << KNRM);
        BC_DEBUG(pos->dump());

        // Create new MBB and put jmp in there
        auto newBlock = MF.CreateMachineBasicBlock();
        MF.insert(iMBB, newBlock); // Insert before next element (between MBB and iMBB)
        BuildMI(newBlock, DebugLoc(), TII->get(pos->getOpcode())).addMBB(pos->getOperand(0).getMBB());
        MBB.addSuccessor(newBlock);
        // Move iMBB back so we proces the new block
        iMBB--;
        // remove jmp from old block and restore pos
        pos->removeFromParent();
        pos = tmp_iter;
        // then process original block as conditional
        replaceConditionalBranch(MF, MBB, pos, originalFallThrough, takenLane);
      } else {
        // Uncondtional branches
        replaceUnconditionalJump(MF, MBB, pos, originalFallThrough, takenLane);
      }
    } else if (pos->isIndirectBranch()) {
      // Indirect branches
      replaceIndirectJump(MF, MBB, pos, originalFallThrough, takenLane);
    } else if (pos->isConditionalBranch()) {
      // Conditional branches
      replaceConditionalBranch(MF, MBB, pos, originalFallThrough, takenLane);
    } else {
      // Not a branch instruction
      assert((originalFallThrough != NULL || pos->isReturn() || pos->isCall()) &&
             "either we have a fallthrough, or we return");
      replaceNoBranchBlock(MF, MBB, pos, originalFallThrough, takenLane);
    }
  }

  MachineBasicBlock *newBlock = MF.CreateMachineBasicBlock();
  MF.push_front(newBlock);
  BuildMI(newBlock, DebugLoc(), TII->get(X86::JMP_4)).addMBB(&entry);
  newBlock->addSuccessor(&entry);

  // Cleanup
  for (auto child : *takenBlocks) {
    delete child;
  }
  delete takenBlocks;
  delete processedMBBs;

  return true;
}

bool X86BranchConversion::replaceNoBranchBlock(MachineFunction &MF, MachineBasicBlock &MBB,
                                               MachineInstrBundleIterator<MachineInstr, false> iter,
                                               MachineBasicBlock *fallThrough,
                                               struct blockLane *takenLane) {
  BC_DEBUG(dump_MI_with_operands("non-branch", nullptr));
  BC_DEBUG(dump_target("fallthrough", fallThrough));

  if (fallThrough != nullptr) {
    auto zbN_p1 = CreateNewBBonTrampoline(MBB, MF, nullptr);
    updateLane(takenLane, zbN_p1, nullptr, true);

    BuildMI(MBB, ++iter, DebugLoc(), TII->get(X86::LEA64r), targetRegOpcode)
        .addReg(X86::RIP)
        .addImm(0)
        .addReg(0)
        .addMBB(zbN_p1)
        .addReg(0);


  } else {
    updateLane(takenLane, nullptr, nullptr, false);
    LANE_DEBUG(errs() << "!!! invalidating lane\n");
  }

  return true;
}


bool X86BranchConversion::replaceIndirectJump(MachineFunction &MF, MachineBasicBlock &MBB,
                                              MachineInstrBundleIterator <MachineInstr> &iter,
                                              MachineBasicBlock *fallThrough,
                                              struct blockLane *takenLane) {
  auto &MI = *iter;
  BC_DEBUG(dump_MI_with_operands("indirect branch", &MI));

  // get the first operand of MachineInstr
  auto operand = MI.getOperand(0);
  assert(operand.isReg() && "support for non register operands is unimplemented");

  // FIXME: This just disables handling of indirect jumps...

#ifdef DONT_DISABLE_INDIRECT_JUMP_INSTRUMENTATION
  auto zbN_p1 = CreateNewBBonTrampoline(MBB, MF, nullptr);

  // Unclear how to handle this since we do not know the target of the jump before run-time
  BuildMI(zbN_p1, DebugLoc(), TII->get(X86::JMP64r)).addReg(operand.getReg());
  // FIXME: we just assume the register is 64bits, guess it could also be 32?

  BuildMI(MBB, iter, DebugLoc(), TII->get(X86::LEA64r), targetRegOpcode)
                .addReg(X86::RIP)
                .addImm(0)
                .addReg(0)
                .addMBB(zbN_p1)
                .addReg(0);

  iter->eraseFromParent();
#endif

  // Invalidate this lane, we have no idea where its going...
  updateLane(takenLane, nullptr, nullptr, false);

  return true;
}


bool X86BranchConversion::replaceUnconditionalJump(MachineFunction &MF, MachineBasicBlock &MBB,
                                                   MachineInstrBundleIterator <MachineInstr> &iter,
                                                   MachineBasicBlock *fallThrough,
                                                   struct blockLane *takenLane) {
  auto &MI = *iter;
  BC_DEBUG(dump_MI_with_operands("unconditional branch", &MI));

  // get the first operand of MachineInstr
  auto operand = MI.getOperand(0);
  assert(operand.isMBB() && "support for non MBB operands is unimplemented");

  // Retrieve the target MachineBasicBlock
  auto dstMBB = operand.getMBB();
  BC_DEBUG(dump_target("target", dstMBB));
  MachineBasicBlock *zbN_p1;
  if (processedMBBs->find(dstMBB) != processedMBBs->end()) {
    zbN_p1 = CreateNewBBonTrampoline(MBB, MF, dstMBB);
    updateLane(takenLane, nullptr, nullptr, false);
  } else {
    zbN_p1 = CreateNewBBonTrampoline(MBB, MF, nullptr);
    updateLane(takenLane, zbN_p1, dstMBB, false);
  }

  BuildMI(MBB, iter, DebugLoc(), TII->get(X86::LEA64r), targetRegOpcode)
      .addReg(X86::RIP)
      .addImm(0)
      .addReg(0)
      .addMBB(zbN_p1)
      .addReg(0);

  iter->eraseFromParent();

  return true;
}

bool X86BranchConversion::replaceConditionalBranch(MachineFunction &MF, MachineBasicBlock &MBB,
                                                   MachineInstrBundleIterator<MachineInstr, false> iter,
                                                   MachineBasicBlock *fallThrough,
                                                   struct blockLane *takenLane) {
  auto &MI = *iter;
  BC_DEBUG(dump_MI_with_operands("conditional branch", &MI));

  // Make sure our operand is as expected
  auto dstOperand = MI.getOperand(0);

  if (!dstOperand.isMBB()) // TODO: currently just crashing if we cannot handle this
    llvm_unreachable("non MBB operands not supported");

  // Get the target MBB for taken jump
  auto dstMBB = dstOperand.getMBB();

  BC_DEBUG(dump_target("target", dstMBB));
  BC_DEBUG(dump_target("fallthrough", fallThrough));

  // Determine what kind of test we are using
  auto cmovOpcode = getCorrespondingMovOpcode(MI);

  // Create the trampoline blocks we're going to need when no skipping
  auto zbN_p1_F = CreateNewBBonTrampoline(MBB, MF, nullptr);

  MachineBasicBlock *zbN_p1;

  if (processedMBBs->find(dstMBB) != processedMBBs->end()) {
    LANE_DEBUG(errs() << "!!! jumping backwards, skipping new lane creation for conditional");
    zbN_p1 = CreateNewBBonTrampoline(MBB, MF, dstMBB);
  } else {
    zbN_p1 = CreateNewBBonTrampoline(MBB, MF, nullptr);
    // Add new lane for the taken jump
    addBBtoBlockLane(
        zbN_p1,   // Goes via fake zb(N+1)F
        dstMBB,     // We should eventually reach dstMBB
        false       // , but not take the next MBB
    );
  }

  // Insert the default, fallthrough move
  BC_DEBUG(errs() << "\t\t\t" << "just trying hasAddressTaken for MBB: " << MBB.hasAddressTaken() << "\n");

  BuildMI(MBB, iter, DebugLoc(), TII->get(X86::LEA64r), targetRegOpcode)
      .addReg(X86::RIP)
      .addImm(0)
      .addReg(0)
      .addMBB(zbN_p1_F)
      .addReg(0);

  BuildMI(MBB, iter, DebugLoc(), TII->get(X86::LEA64r), tmpReg)
      .addReg(X86::RIP)
      .addImm(0)
      .addReg(0)
      .addMBB(zbN_p1)
      .addReg(0);

  BuildMI(MBB, iter, DebugLoc(), TII->get(cmovOpcode), X86::R11)
      // FIXME: should probably figure out what op0 (X86:R11) does
      .addReg(targetRegOpcode) // dst register
      .addReg(tmpReg);  // src register

  // and finally, remove the original conditional jump
  iter->eraseFromParent();

  // Treat the takenLane as the fallthrough
  updateLane(takenLane, zbN_p1_F, nullptr, true);

  LANE_DEBUG(errs() << "!!! splitting lane (lanes " << takenBlocks->size() << ")\n");

  return true;
}

void X86BranchConversion::addDummyInstructions(MachineBasicBlock *realBlock, MachineBasicBlock *trampolineBlock) {

  int instructionCount = 0;

  for (const auto &I : *realBlock) {
    (void) I;
    instructionCount++;
  }

  //const Constant *C = ConstantInt::get(Type::getInt8Ty(realBlock->getBasicBlock()->getContext()), 0);
  for (int i = 2; i < instructionCount; i++) {
    BuildMI(trampolineBlock, DebugLoc(), TII->get(X86::ADD8ri), tmpReg8).addReg(tmpReg8).addImm(0);
  }
}

MachineBasicBlock *X86BranchConversion::CreateNewBBonTrampoline(MachineBasicBlock &MBB, MachineFunction &MF,
                                                                MachineBasicBlock *destOnCode) {
  MachineBasicBlock *newBlock = MF.CreateMachineBasicBlock();
  MF.push_front(newBlock);
  if (destOnCode != nullptr) {
    BC_DEBUG(errs() << "\t\t\t" << "we create a block on Trampoline, that is a jump from: ");
    BC_DEBUG(errs() << newBlock << " to: " << destOnCode << "\n");
    LANE_DEBUG(errs() << "!!! updating lane to ");
    LANE_DEBUG(errs().write_escaped(destOnCode->getName()) << "\n");
    BuildMI(newBlock, DebugLoc(), TII->get(X86::JMP_4)).addMBB(destOnCode);
    newBlock->addSuccessor(destOnCode);
  }

  //MBB.setHasAddressTaken();
  MBB.addSuccessor(newBlock);
  return newBlock;
}

struct blockLane *X86BranchConversion::addBBtoBlockLane(MachineBasicBlock *MBB,
                                                        MachineBasicBlock *DestMBB,
                                                        bool taken) {
  auto newLane = new blockLane(MBB, DestMBB, taken);
  takenBlocks->push_back(newLane);
  return newLane;
}

inline void
X86BranchConversion::updateLane(struct blockLane *lane, MachineBasicBlock *a, MachineBasicBlock *b, bool c) {
  if (lane != nullptr) {
    lane->currentMBB = a;
    lane->DestMBB = b;
    lane->taken = c;
  }
}

unsigned int X86BranchConversion::getCorrespondingMovOpcode(MachineInstr &MI) {
  X86::CondCode CC = X86::getCondFromBranchOpc(MI.getOpcode());
  return X86::getCMovFromCond(CC, 8, false);
}

inline void X86BranchConversion::dump_function(const MachineFunction &MF) {
  errs() << "function ";
  errs().write_escaped(MF.getName()) << "\n";
}

inline void X86BranchConversion::dump_MBB(const MachineBasicBlock &MBB) {
  errs() << KWHT << "\t<BLOCK ";
  errs().write_escaped(MBB.getName()) << ">\n";
  for (auto &MI : MBB) {
    errs() << "\t\t";
    MI.dump();
  }
  errs() << "\t</BLOCK ";
  errs().write_escaped(MBB.getName()) << ">\n" << KNRM;
}

inline void X86BranchConversion::dump_MI_with_operands(const std::string &name, const MachineInstr *MI) {
  errs() << KGRN;
  if (MI == nullptr) {
    errs() << "\t\tfound a " << name << "\n";
  } else {
    errs() << "\t\tfound a " << name << " with "
           << MI->getNumOperands() << " operands: ";
    MI->dump();
  }
  errs() << KNRM;
}

inline void X86BranchConversion::dump_target(const std::string &name, const MachineBasicBlock *MBB) {
  errs() << "\t\t\t" << name << " is ";
  if (MBB == nullptr)
    errs() << "NULL\n";
  else
    errs().write_escaped(MBB->getName()) << "\n";
}

/**
 * @brief get the name for an operand type
 *
 * @param op The machine operand to inpsect
 * @return a string representing the name of the operand type
 */
std::string X86BranchConversion::getOperandType(MachineOperand &op) {
  std::string r = "";

  if (op.isReg())
    r += "Reg";
  if (op.isImm())
    r += "Imm";
  if (op.isCImm())
    r += "CImm";
  if (op.isFPImm())
    r += "FPImm";
  if (op.isMBB())
    r += "MBB";
  if (op.isFI())
    r += "FI";
  if (op.isCPI())
    r += "CPI";
  if (op.isTargetIndex())
    r += "TargetIndex";
  if (op.isJTI())
    r += "JTI";
  if (op.isGlobal())
    r += "isGlobal";
  if (op.isSymbol())
    r += "Symbol";
  if (op.isBlockAddress())
    r += "BlockAddress";
  if (op.isRegMask())
    r += "RegMask";
  if (op.isRegLiveOut())
    r += "RegLiveOut";
  if (op.isMetadata())
    r += "Metadata";
  if (op.isMCSymbol())
    r += "MCSymbol";
  if (op.isCFIIndex())
    r += "CFIIndex";
  if (op.isIntrinsicID())
    r += "IntrinsicID";
  if (op.isPredicate())
    r += "Predicate";

  return r;
}
