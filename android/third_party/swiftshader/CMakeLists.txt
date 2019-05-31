cmake_minimum_required(VERSION 2.8)
project(SwiftShader C CXX)

###########################################################
# Options
###########################################################

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The type of build: Debug Release MinSizeRel RelWithDebInfo." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)

option(BUILD_EGL "Build the EGL library" 1)
option(BUILD_GLESv2 "Build the OpenGL ES 2 library" 1)
option(BUILD_GLES_CM "Build the OpenGL ES 1.1 library" 1)
option(BUILD_VULKAN "Build the Vulkan library" 1)

option(USE_GROUP_SOURCES "Group the source files in a folder tree for Visual Studio" 1)

option(BUILD_SAMPLES "Build sample programs" 1)
option(BUILD_TESTS "Build test programs" 1)

option(WARNINGS_AS_ERRORS "Treat all warnings as errors" 1)
option(DCHECK_ALWAYS_ON "Check validation macros even in release builds" 0)

set(DEFAULT_REACTOR_BACKEND "LLVM")

set(REACTOR_BACKEND ${DEFAULT_REACTOR_BACKEND} CACHE STRING "JIT compiler back-end used by Reactor")
set_property(CACHE REACTOR_BACKEND PROPERTY STRINGS LLVM Subzero)

# LLVM disallows calling cmake . from the main LLVM dir, the reason is that
# it builds header files that could overwrite the orignal ones. Here we
# want to include LLVM as a subdirectory and even though it wouldn't cause
# the problem, if cmake . is called from the main dir, the condition that
# LLVM checkes, "CMAKE_CURRENT_SOURCE_DIR == CMAKE_CURRENT_BINARY_DIR" will be true. So we
# disallow it ourselves too to. In addition if there are remining CMakeFiles
# and CMakeCache in the directory, cmake .. from a subdirectory will still
# try to build from the main directory so we instruct users to delete these
# files when they get the error.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In source builds are not allowed by LLVM, please create a build/ directory and build from there. You may have to delete the CMakeCache.txt file and CMakeFiles directory that are next to the CMakeLists.txt.")
endif()


# Takes target library and a directory where the export map is and add the linker options so that only the API symbols
# are exported.
macro(set_shared_library_export_map TARGET DIR)
  if(MSVC)
    set_property(TARGET ${TARGET} APPEND_STRING PROPERTY LINK_FLAGS " /DEF:\"${DIR}/${TARGET}.def\"")
  elseif(LINUX_X86_64)
    # The version script only exports the API functions and hides all the others.
    set_property(TARGET ${TARGET} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--version-script=${DIR}/${TARGET}.lds")
    set_property(TARGET ${TARGET} APPEND_STRING PROPERTY LINK_DEPENDS "${DIR}/${TARGET}.lds;")

    # Don't allow symbols to be overridden by another module.
    set_property(TARGET ${TARGET} APPEND_STRING PROPERTY COMPILE_FLAGS " -fvisibility=protected")

    # Both hash-style are needed, because we want both gold and GNU ld to be able to read our libraries.
    set_property(TARGET ${TARGET} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--hash-style=both")

    # Gc sections is used in combination with each functions being in its own section, to reduce the binary size.
    set_property(TARGET ${TARGET} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")

    # Don't allow undefined symbols, unless it's a Sanitizer build.
    if(NOT OPTION_ASAN)
      set_property(TARGET ${TARGET} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--no-undefined")
    endif()
  endif()
endmacro()

#
# Directories
#

get_filename_component(SWIFTSHADER_ROOT ${ANDROID_QEMU2_TOP_DIR}/../swiftshader ABSOLUTE)
set(SOURCE_DIR ${SWIFTSHADER_ROOT}/src)
set(OPENGL_DIR ${SOURCE_DIR}/OpenGL)
set(OPENGL_COMPILER_DIR ${OPENGL_DIR}/compiler)
set(VULKAN_DIR ${SOURCE_DIR}/Vulkan)
if(REACTOR_LLVM_VERSION EQUAL 3)
  set(LLVM_DIR ${SWIFTSHADER_ROOT}/third_party/LLVM)
else()
  set(LLVM_DIR ${SWIFTSHADER_ROOT}/third_party/llvm-7.0/llvm)
  set(LLVM_CONFIG_DIR ${SWIFTSHADER_ROOT}/third_party/llvm-7.0/configs)
endif()
set(SUBZERO_DIR ${SWIFTSHADER_ROOT}/third_party/subzero)
set(SUBZERO_LLVM_DIR ${SWIFTSHADER_ROOT}/third_party/llvm-subzero)
set(TESTS_DIR ${SWIFTSHADER_ROOT}/tests)

###########################################################
# Compile flags
###########################################################

# Flags for project code (non 3rd party)
set(SWIFTSHADER_COMPILE_OPTIONS "")

macro(set_cpp_flag FLAG)
  set(SWIFTSHADER_CXX_FLAGS "${SWIFTSHADER_CXX_FLAGS};${FLAG}")
endmacro()

macro(set_def_flag FLAG)
  set(SWIFTSHADER_DEFS "${SWIFTSHADER_DEFS};${FLAG}")
endmacro()

if(WINDOWS_MSVC_X86_64)
  set_cpp_flag("/MP")
  set_def_flag(-D_CRT_SECURE_NO_WARNINGS)
  set_def_flag(-D_SBCS) # Single Byte Character Set (ASCII)

    list(APPEND MSVC_SWIFTSHADER_COMPILE_OPTIONS
        "/wd4005" # 'identifier' : macro redefinition
        "/wd4018" # 'expression' : signed/unsigned mismatch
        "/wd4065" # switch statement contains 'default' but no 'case' labels
        "/wd4141" # 'modifier' : used more than once
        "/wd4146" # unary minus operator applied to unsigned type, result still unsigned
        "/wd4244" # 'conversion' conversion from 'type1' to 'type2', possible loss of data
        "/wd4267" # 'var' : conversion from 'size_t' to 'type', possible loss of data
        "/wd4291" # 'void X new(size_t,unsigned int,unsigned int)': no matching operator delete found; memory will not be freed if initialization throws an exception
        "/wd4309" # 'conversion' : truncation of constant value
        "/wd4624" # 'derived class' : destructor was implicitly defined as deleted because a base class destructor is inaccessible or deleted
        "/wd4800" # 'type' : forcing value to bool 'true' or 'false' (performance warning)
        "/wd4838" # conversion from 'type_1' to 'type_2' requires a narrowing conversion
        "/wd5030" # attribute 'attribute' is not recognized
        "/wd5038" # data member 'member1' will be initialized after data member 'member2' data member 'member' will be initialized after base class 'base_class'
    )

    # Treat specific warnings as errors
    list(APPEND MSVC_SWIFTSHADER_COMPILE_OPTIONS
        "/we4018" # 'expression' : signed/unsigned mismatch
        "/we4471" # 'enumeration': a forward declaration of an unscoped enumeration must have an underlying type (int assumed)
        "/we4838" # conversion from 'type_1' to 'type_2' requires a narrowing conversion
        "/we5038" # data member 'member1' will be initialized after data member 'member2' data member 'member' will be initialized after base class 'base_class'
    )
endif()
set_cpp_flag("-Werror=reorder")
# set_cpp_flag("-Werror=sign-compare") clang-cl is not loving this.
set_cpp_flag("-Werror=missing-braces")
set_cpp_flag("-Werror=unused-lambda-capture")

list(APPEND SWIFTSHADER_COMPILE_OPTIONS
    "-Werror=unused-lambda-capture"
    "-Werror=string-conversion"
    "-Werror=extra-semi"
    "-Werror=ignored-qualifiers"
    )

# Remove xor, and, or and friends from the list of keywords, they are used by Reactor
set_cpp_flag("-fno-operator-names")

# LLVM headers requires these flags to be set
set_def_flag("-D__STDC_CONSTANT_MACROS")
set_def_flag("-D__STDC_LIMIT_MACROS")

set_cpp_flag("-m64")
# set_cpp_flag("-fPIC")
set_cpp_flag("-march=x86-64")
set_cpp_flag("-mtune=generic")

if(LINUX_X86_64)
  set_def_flag("-DUSE_X11=1")
  set_def_flag("-DVK_USE_PLATFORM_XLIB_KHR")
endif()

if(DCHECK_ALWAYS_ON)
    list(APPEND SWIFTSHADER_COMPILE_OPTIONS "-DDCHECK_ALWAYS_ON")
endif()

set_def_flag("-DNDEBUG")

# Put each variable and function in its own section so that when linking with -gc-sections unused functions and
# variables are removed.
set_cpp_flag("-ffunction-sections")
set_cpp_flag("-fdata-sections")
set_cpp_flag("-fomit-frame-pointer")

if(MSVC)
  set_def_flag(-DWINVER=0x501)
  set_def_flag(-DNOMINMAX)
  set_def_flag(-DSTRICT)
endif()

###########################################################
# LLVM
###########################################################

set(LLVM_LIST
    ${LLVM_DIR}/lib/Analysis/AliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/AliasAnalysisSummary.cpp
    ${LLVM_DIR}/lib/Analysis/AliasSetTracker.cpp
    ${LLVM_DIR}/lib/Analysis/AssumptionCache.cpp
    ${LLVM_DIR}/lib/Analysis/BasicAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/BlockFrequencyInfo.cpp
    ${LLVM_DIR}/lib/Analysis/BlockFrequencyInfoImpl.cpp
    ${LLVM_DIR}/lib/Analysis/BranchProbabilityInfo.cpp
    ${LLVM_DIR}/lib/Analysis/CFG.cpp
    ${LLVM_DIR}/lib/Analysis/CFLAndersAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/CFLSteensAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/CallGraph.cpp
    ${LLVM_DIR}/lib/Analysis/CallGraphSCCPass.cpp
    ${LLVM_DIR}/lib/Analysis/CaptureTracking.cpp
    ${LLVM_DIR}/lib/Analysis/CmpInstAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/CodeMetrics.cpp
    ${LLVM_DIR}/lib/Analysis/ConstantFolding.cpp
    ${LLVM_DIR}/lib/Analysis/DemandedBits.cpp
    ${LLVM_DIR}/lib/Analysis/DependenceAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/DivergenceAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/DominanceFrontier.cpp
    ${LLVM_DIR}/lib/Analysis/EHPersonalities.cpp
    ${LLVM_DIR}/lib/Analysis/GlobalsModRef.cpp
    ${LLVM_DIR}/lib/Analysis/IVUsers.cpp
    ${LLVM_DIR}/lib/Analysis/InlineCost.cpp
    ${LLVM_DIR}/lib/Analysis/InstructionSimplify.cpp
    ${LLVM_DIR}/lib/Analysis/IteratedDominanceFrontier.cpp
    ${LLVM_DIR}/lib/Analysis/LazyBlockFrequencyInfo.cpp
    ${LLVM_DIR}/lib/Analysis/LazyBranchProbabilityInfo.cpp
    ${LLVM_DIR}/lib/Analysis/LazyValueInfo.cpp
    ${LLVM_DIR}/lib/Analysis/Loads.cpp
    ${LLVM_DIR}/lib/Analysis/LoopAnalysisManager.cpp
    ${LLVM_DIR}/lib/Analysis/LoopInfo.cpp
    ${LLVM_DIR}/lib/Analysis/LoopPass.cpp
    ${LLVM_DIR}/lib/Analysis/MemoryBuiltins.cpp
    ${LLVM_DIR}/lib/Analysis/MemoryDependenceAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/MemoryLocation.cpp
    ${LLVM_DIR}/lib/Analysis/MemorySSA.cpp
    ${LLVM_DIR}/lib/Analysis/MemorySSAUpdater.cpp
    ${LLVM_DIR}/lib/Analysis/MustExecute.cpp
    ${LLVM_DIR}/lib/Analysis/ObjCARCAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/ObjCARCAnalysisUtils.cpp
    ${LLVM_DIR}/lib/Analysis/ObjCARCInstKind.cpp
    ${LLVM_DIR}/lib/Analysis/OptimizationRemarkEmitter.cpp
    ${LLVM_DIR}/lib/Analysis/OrderedBasicBlock.cpp
    ${LLVM_DIR}/lib/Analysis/PHITransAddr.cpp
    ${LLVM_DIR}/lib/Analysis/PhiValues.cpp
    ${LLVM_DIR}/lib/Analysis/PostDominators.cpp
    ${LLVM_DIR}/lib/Analysis/ProfileSummaryInfo.cpp
    ${LLVM_DIR}/lib/Analysis/PtrUseVisitor.cpp
    ${LLVM_DIR}/lib/Analysis/ScalarEvolution.cpp
    ${LLVM_DIR}/lib/Analysis/ScalarEvolutionAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/ScalarEvolutionExpander.cpp
    ${LLVM_DIR}/lib/Analysis/ScalarEvolutionNormalization.cpp
    ${LLVM_DIR}/lib/Analysis/ScopedNoAliasAA.cpp
    ${LLVM_DIR}/lib/Analysis/TargetLibraryInfo.cpp
    ${LLVM_DIR}/lib/Analysis/TargetTransformInfo.cpp
    ${LLVM_DIR}/lib/Analysis/TypeBasedAliasAnalysis.cpp
    ${LLVM_DIR}/lib/Analysis/ValueLattice.cpp
    ${LLVM_DIR}/lib/Analysis/ValueLatticeUtils.cpp
    ${LLVM_DIR}/lib/Analysis/ValueTracking.cpp
    ${LLVM_DIR}/lib/Analysis/VectorUtils.cpp
    ${LLVM_DIR}/lib/AsmParser/LLLexer.cpp
    ${LLVM_DIR}/lib/AsmParser/LLParser.cpp
    ${LLVM_DIR}/lib/AsmParser/Parser.cpp
    ${LLVM_DIR}/lib/BinaryFormat/Dwarf.cpp
    ${LLVM_DIR}/lib/BinaryFormat/Magic.cpp
    ${LLVM_DIR}/lib/BinaryFormat/Wasm.cpp
    ${LLVM_DIR}/lib/Bitcode/Reader/BitcodeReader.cpp
    ${LLVM_DIR}/lib/Bitcode/Reader/BitstreamReader.cpp
    ${LLVM_DIR}/lib/Bitcode/Reader/MetadataLoader.cpp
    ${LLVM_DIR}/lib/Bitcode/Reader/ValueList.cpp
    ${LLVM_DIR}/lib/CodeGen/AggressiveAntiDepBreaker.cpp
    ${LLVM_DIR}/lib/CodeGen/AllocationOrder.cpp
    ${LLVM_DIR}/lib/CodeGen/Analysis.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/ARMException.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/AccelTable.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/AddressPool.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/AsmPrinter.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/AsmPrinterDwarf.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/AsmPrinterInlineAsm.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/CodeViewDebug.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DIE.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DIEHash.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DbgValueHistoryCalculator.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DebugHandlerBase.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DebugLocStream.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DwarfCFIException.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DwarfCompileUnit.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DwarfDebug.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DwarfExpression.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DwarfFile.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DwarfStringPool.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/DwarfUnit.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/EHStreamer.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/WinCFGuard.cpp
    ${LLVM_DIR}/lib/CodeGen/AsmPrinter/WinException.cpp
    ${LLVM_DIR}/lib/CodeGen/AtomicExpandPass.cpp
    ${LLVM_DIR}/lib/CodeGen/BasicTargetTransformInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/BranchFolding.cpp
    ${LLVM_DIR}/lib/CodeGen/BranchRelaxation.cpp
    ${LLVM_DIR}/lib/CodeGen/BreakFalseDeps.cpp
    ${LLVM_DIR}/lib/CodeGen/CFIInstrInserter.cpp
    ${LLVM_DIR}/lib/CodeGen/CalcSpillWeights.cpp
    ${LLVM_DIR}/lib/CodeGen/CallingConvLower.cpp
    ${LLVM_DIR}/lib/CodeGen/CodeGen.cpp
    ${LLVM_DIR}/lib/CodeGen/CodeGenPrepare.cpp
    ${LLVM_DIR}/lib/CodeGen/CriticalAntiDepBreaker.cpp
    ${LLVM_DIR}/lib/CodeGen/DFAPacketizer.cpp
    ${LLVM_DIR}/lib/CodeGen/DeadMachineInstructionElim.cpp
    ${LLVM_DIR}/lib/CodeGen/DetectDeadLanes.cpp
    ${LLVM_DIR}/lib/CodeGen/DwarfEHPrepare.cpp
    ${LLVM_DIR}/lib/CodeGen/EarlyIfConversion.cpp
    ${LLVM_DIR}/lib/CodeGen/EdgeBundles.cpp
    ${LLVM_DIR}/lib/CodeGen/ExecutionDomainFix.cpp
    ${LLVM_DIR}/lib/CodeGen/ExpandISelPseudos.cpp
    ${LLVM_DIR}/lib/CodeGen/ExpandMemCmp.cpp
    ${LLVM_DIR}/lib/CodeGen/ExpandPostRAPseudos.cpp
    ${LLVM_DIR}/lib/CodeGen/ExpandReductions.cpp
    ${LLVM_DIR}/lib/CodeGen/FEntryInserter.cpp
    ${LLVM_DIR}/lib/CodeGen/FaultMaps.cpp
    ${LLVM_DIR}/lib/CodeGen/FuncletLayout.cpp
    ${LLVM_DIR}/lib/CodeGen/GCMetadata.cpp
    ${LLVM_DIR}/lib/CodeGen/GCMetadataPrinter.cpp
    ${LLVM_DIR}/lib/CodeGen/GCRootLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/GCStrategy.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/CallLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/GlobalISel.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/IRTranslator.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/InstructionSelect.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/InstructionSelector.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/LegalityPredicates.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/LegalizeMutations.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/Legalizer.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/LegalizerHelper.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/LegalizerInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/Localizer.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/MachineIRBuilder.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/RegBankSelect.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/RegisterBank.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/RegisterBankInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/GlobalISel/Utils.cpp
    ${LLVM_DIR}/lib/CodeGen/IfConversion.cpp
    ${LLVM_DIR}/lib/CodeGen/ImplicitNullChecks.cpp
    ${LLVM_DIR}/lib/CodeGen/IndirectBrExpandPass.cpp
    ${LLVM_DIR}/lib/CodeGen/InlineSpiller.cpp
    ${LLVM_DIR}/lib/CodeGen/InterferenceCache.cpp
    ${LLVM_DIR}/lib/CodeGen/InterleavedAccessPass.cpp
    ${LLVM_DIR}/lib/CodeGen/IntrinsicLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/LLVMTargetMachine.cpp
    ${LLVM_DIR}/lib/CodeGen/LatencyPriorityQueue.cpp
    ${LLVM_DIR}/lib/CodeGen/LazyMachineBlockFrequencyInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/LexicalScopes.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveDebugValues.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveDebugVariables.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveInterval.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveIntervalUnion.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveIntervals.cpp
    ${LLVM_DIR}/lib/CodeGen/LivePhysRegs.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveRangeCalc.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveRangeEdit.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveRangeShrink.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveRegMatrix.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveRegUnits.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveStacks.cpp
    ${LLVM_DIR}/lib/CodeGen/LiveVariables.cpp
    ${LLVM_DIR}/lib/CodeGen/LocalStackSlotAllocation.cpp
    ${LLVM_DIR}/lib/CodeGen/LoopTraversal.cpp
    ${LLVM_DIR}/lib/CodeGen/LowLevelType.cpp
    ${LLVM_DIR}/lib/CodeGen/LowerEmuTLS.cpp
    ${LLVM_DIR}/lib/CodeGen/MIRCanonicalizerPass.cpp
    ${LLVM_DIR}/lib/CodeGen/MIRPrinter.cpp
    ${LLVM_DIR}/lib/CodeGen/MIRPrintingPass.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineBasicBlock.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineBlockFrequencyInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineBlockPlacement.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineBranchProbabilityInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineCSE.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineCombiner.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineCopyPropagation.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineDominanceFrontier.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineDominators.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFrameInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFunction.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFunctionPass.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineFunctionPrinterPass.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineInstr.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineInstrBundle.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineLICM.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineLoopInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineModuleInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineModuleInfoImpls.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineOperand.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineOptimizationRemarkEmitter.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineOutliner.cpp
    ${LLVM_DIR}/lib/CodeGen/MachinePassRegistry.cpp
    ${LLVM_DIR}/lib/CodeGen/MachinePipeliner.cpp
    ${LLVM_DIR}/lib/CodeGen/MachinePostDominators.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineRegionInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineRegisterInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineSSAUpdater.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineScheduler.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineSink.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineTraceMetrics.cpp
    ${LLVM_DIR}/lib/CodeGen/MachineVerifier.cpp
    ${LLVM_DIR}/lib/CodeGen/MacroFusion.cpp
    ${LLVM_DIR}/lib/CodeGen/OptimizePHIs.cpp
    ${LLVM_DIR}/lib/CodeGen/PHIElimination.cpp
    ${LLVM_DIR}/lib/CodeGen/PHIEliminationUtils.cpp
    ${LLVM_DIR}/lib/CodeGen/PatchableFunction.cpp
    ${LLVM_DIR}/lib/CodeGen/PeepholeOptimizer.cpp
    ${LLVM_DIR}/lib/CodeGen/PostRAHazardRecognizer.cpp
    ${LLVM_DIR}/lib/CodeGen/PostRASchedulerList.cpp
    ${LLVM_DIR}/lib/CodeGen/PreISelIntrinsicLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/ProcessImplicitDefs.cpp
    ${LLVM_DIR}/lib/CodeGen/PrologEpilogInserter.cpp
    ${LLVM_DIR}/lib/CodeGen/PseudoSourceValue.cpp
    ${LLVM_DIR}/lib/CodeGen/ReachingDefAnalysis.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocBase.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocBasic.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocFast.cpp
    ${LLVM_DIR}/lib/CodeGen/RegAllocGreedy.cpp
    ${LLVM_DIR}/lib/CodeGen/RegUsageInfoCollector.cpp
    ${LLVM_DIR}/lib/CodeGen/RegUsageInfoPropagate.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterClassInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterCoalescer.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterPressure.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterScavenging.cpp
    ${LLVM_DIR}/lib/CodeGen/RegisterUsageInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/RenameIndependentSubregs.cpp
    ${LLVM_DIR}/lib/CodeGen/ResetMachineFunctionPass.cpp
    ${LLVM_DIR}/lib/CodeGen/SafeStack.cpp
    ${LLVM_DIR}/lib/CodeGen/SafeStackColoring.cpp
    ${LLVM_DIR}/lib/CodeGen/SafeStackLayout.cpp
    ${LLVM_DIR}/lib/CodeGen/ScalarizeMaskedMemIntrin.cpp
    ${LLVM_DIR}/lib/CodeGen/ScheduleDAG.cpp
    ${LLVM_DIR}/lib/CodeGen/ScheduleDAGInstrs.cpp
    ${LLVM_DIR}/lib/CodeGen/ScheduleDAGPrinter.cpp
    ${LLVM_DIR}/lib/CodeGen/ScoreboardHazardRecognizer.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/DAGCombiner.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/FastISel.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/FunctionLoweringInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/InstrEmitter.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeDAG.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeFloatTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeIntegerTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeTypesGeneric.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeVectorOps.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/LegalizeVectorTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ResourcePriorityQueue.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ScheduleDAGRRList.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ScheduleDAGSDNodes.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/ScheduleDAGVLIW.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAG.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGAddressAnalysis.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGBuilder.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGDumper.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGISel.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGPrinter.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/SelectionDAGTargetInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/StatepointLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/SelectionDAG/TargetLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/ShadowStackGCLowering.cpp
    ${LLVM_DIR}/lib/CodeGen/ShrinkWrap.cpp
    ${LLVM_DIR}/lib/CodeGen/SjLjEHPrepare.cpp
    ${LLVM_DIR}/lib/CodeGen/SlotIndexes.cpp
    ${LLVM_DIR}/lib/CodeGen/SpillPlacement.cpp
    ${LLVM_DIR}/lib/CodeGen/SplitKit.cpp
    ${LLVM_DIR}/lib/CodeGen/StackColoring.cpp
    ${LLVM_DIR}/lib/CodeGen/StackMapLivenessAnalysis.cpp
    ${LLVM_DIR}/lib/CodeGen/StackMaps.cpp
    ${LLVM_DIR}/lib/CodeGen/StackProtector.cpp
    ${LLVM_DIR}/lib/CodeGen/StackSlotColoring.cpp
    ${LLVM_DIR}/lib/CodeGen/TailDuplication.cpp
    ${LLVM_DIR}/lib/CodeGen/TailDuplicator.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetFrameLoweringImpl.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetInstrInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetLoweringBase.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetLoweringObjectFileImpl.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetOptionsImpl.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetPassConfig.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetRegisterInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetSchedule.cpp
    ${LLVM_DIR}/lib/CodeGen/TargetSubtargetInfo.cpp
    ${LLVM_DIR}/lib/CodeGen/TwoAddressInstructionPass.cpp
    ${LLVM_DIR}/lib/CodeGen/UnreachableBlockElim.cpp
    ${LLVM_DIR}/lib/CodeGen/ValueTypes.cpp
    ${LLVM_DIR}/lib/CodeGen/VirtRegMap.cpp
    ${LLVM_DIR}/lib/CodeGen/WasmEHPrepare.cpp
    ${LLVM_DIR}/lib/CodeGen/WinEHPrepare.cpp
    ${LLVM_DIR}/lib/CodeGen/XRayInstrumentation.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/CVTypeVisitor.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/CodeViewError.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/CodeViewRecordIO.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/ContinuationRecordBuilder.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/Formatters.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/GlobalTypeTableBuilder.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/Line.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/RecordName.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/RecordSerialization.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/SimpleTypeSerializer.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/SymbolRecordMapping.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/TypeDumpVisitor.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/TypeHashing.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/TypeIndex.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/TypeIndexDiscovery.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/TypeRecordMapping.cpp
    ${LLVM_DIR}/lib/DebugInfo/CodeView/TypeTableCollection.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/ExecutionEngine.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/GDBRegistrationListener.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/Orc/Core.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/Orc/Legacy.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/Orc/OrcError.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/JITSymbol.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/RTDyldMemoryManager.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/RuntimeDyld.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/RuntimeDyldCOFF.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/RuntimeDyldChecker.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/RuntimeDyldELF.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/RuntimeDyldMachO.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/RuntimeDyld/Targets/RuntimeDyldELFMips.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/SectionMemoryManager.cpp
    ${LLVM_DIR}/lib/ExecutionEngine/TargetSelect.cpp
    ${LLVM_DIR}/lib/IR/AsmWriter.cpp
    ${LLVM_DIR}/lib/IR/Attributes.cpp
    ${LLVM_DIR}/lib/IR/AutoUpgrade.cpp
    ${LLVM_DIR}/lib/IR/BasicBlock.cpp
    ${LLVM_DIR}/lib/IR/Comdat.cpp
    ${LLVM_DIR}/lib/IR/ConstantFold.cpp
    ${LLVM_DIR}/lib/IR/ConstantRange.cpp
    ${LLVM_DIR}/lib/IR/Constants.cpp
    ${LLVM_DIR}/lib/IR/DIBuilder.cpp
    ${LLVM_DIR}/lib/IR/DataLayout.cpp
    ${LLVM_DIR}/lib/IR/DebugInfo.cpp
    ${LLVM_DIR}/lib/IR/DebugInfoMetadata.cpp
    ${LLVM_DIR}/lib/IR/DebugLoc.cpp
    ${LLVM_DIR}/lib/IR/DiagnosticHandler.cpp
    ${LLVM_DIR}/lib/IR/DiagnosticInfo.cpp
    ${LLVM_DIR}/lib/IR/DiagnosticPrinter.cpp
    ${LLVM_DIR}/lib/IR/Dominators.cpp
    ${LLVM_DIR}/lib/IR/Function.cpp
    ${LLVM_DIR}/lib/IR/GVMaterializer.cpp
    ${LLVM_DIR}/lib/IR/Globals.cpp
    ${LLVM_DIR}/lib/IR/IRBuilder.cpp
    ${LLVM_DIR}/lib/IR/IRPrintingPasses.cpp
    ${LLVM_DIR}/lib/IR/InlineAsm.cpp
    ${LLVM_DIR}/lib/IR/Instruction.cpp
    ${LLVM_DIR}/lib/IR/Instructions.cpp
    ${LLVM_DIR}/lib/IR/IntrinsicInst.cpp
    ${LLVM_DIR}/lib/IR/LLVMContext.cpp
    ${LLVM_DIR}/lib/IR/LLVMContextImpl.cpp
    ${LLVM_DIR}/lib/IR/LegacyPassManager.cpp
    ${LLVM_DIR}/lib/IR/MDBuilder.cpp
    ${LLVM_DIR}/lib/IR/Mangler.cpp
    ${LLVM_DIR}/lib/IR/Metadata.cpp
    ${LLVM_DIR}/lib/IR/Module.cpp
    ${LLVM_DIR}/lib/IR/Operator.cpp
    ${LLVM_DIR}/lib/IR/OptBisect.cpp
    ${LLVM_DIR}/lib/IR/Pass.cpp
    ${LLVM_DIR}/lib/IR/PassManager.cpp
    ${LLVM_DIR}/lib/IR/PassRegistry.cpp
    ${LLVM_DIR}/lib/IR/ProfileSummary.cpp
    ${LLVM_DIR}/lib/IR/Statepoint.cpp
    ${LLVM_DIR}/lib/IR/Type.cpp
    ${LLVM_DIR}/lib/IR/TypeFinder.cpp
    ${LLVM_DIR}/lib/IR/Use.cpp
    ${LLVM_DIR}/lib/IR/User.cpp
    ${LLVM_DIR}/lib/IR/Value.cpp
    ${LLVM_DIR}/lib/IR/ValueSymbolTable.cpp
    ${LLVM_DIR}/lib/IR/Verifier.cpp
    ${LLVM_DIR}/lib/MC/ELFObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/MCAsmBackend.cpp
    ${LLVM_DIR}/lib/MC/MCAsmInfo.cpp
    ${LLVM_DIR}/lib/MC/MCAsmInfoCOFF.cpp
    ${LLVM_DIR}/lib/MC/MCAsmInfoDarwin.cpp
    ${LLVM_DIR}/lib/MC/MCAsmInfoELF.cpp
    ${LLVM_DIR}/lib/MC/MCAsmMacro.cpp
    ${LLVM_DIR}/lib/MC/MCAsmStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCAssembler.cpp
    ${LLVM_DIR}/lib/MC/MCCodeEmitter.cpp
    ${LLVM_DIR}/lib/MC/MCCodePadder.cpp
    ${LLVM_DIR}/lib/MC/MCCodeView.cpp
    ${LLVM_DIR}/lib/MC/MCContext.cpp
    ${LLVM_DIR}/lib/MC/MCDisassembler/MCRelocationInfo.cpp
    ${LLVM_DIR}/lib/MC/MCDwarf.cpp
    ${LLVM_DIR}/lib/MC/MCELFObjectTargetWriter.cpp
    ${LLVM_DIR}/lib/MC/MCELFStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCExpr.cpp
    ${LLVM_DIR}/lib/MC/MCFragment.cpp
    ${LLVM_DIR}/lib/MC/MCInst.cpp
    ${LLVM_DIR}/lib/MC/MCInstPrinter.cpp
    ${LLVM_DIR}/lib/MC/MCInstrAnalysis.cpp
    ${LLVM_DIR}/lib/MC/MCInstrDesc.cpp
    ${LLVM_DIR}/lib/MC/MCLinkerOptimizationHint.cpp
    ${LLVM_DIR}/lib/MC/MCMachOStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCMachObjectTargetWriter.cpp
    ${LLVM_DIR}/lib/MC/MCNullStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCObjectFileInfo.cpp
    ${LLVM_DIR}/lib/MC/MCObjectStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/MCParser/AsmLexer.cpp
    ${LLVM_DIR}/lib/MC/MCParser/AsmParser.cpp
    ${LLVM_DIR}/lib/MC/MCParser/COFFAsmParser.cpp
    ${LLVM_DIR}/lib/MC/MCParser/DarwinAsmParser.cpp
    ${LLVM_DIR}/lib/MC/MCParser/ELFAsmParser.cpp
    ${LLVM_DIR}/lib/MC/MCParser/MCAsmLexer.cpp
    ${LLVM_DIR}/lib/MC/MCParser/MCAsmParser.cpp
    ${LLVM_DIR}/lib/MC/MCParser/MCAsmParserExtension.cpp
    ${LLVM_DIR}/lib/MC/MCParser/MCTargetAsmParser.cpp
    ${LLVM_DIR}/lib/MC/MCRegisterInfo.cpp
    ${LLVM_DIR}/lib/MC/MCSchedule.cpp
    ${LLVM_DIR}/lib/MC/MCSection.cpp
    ${LLVM_DIR}/lib/MC/MCSectionCOFF.cpp
    ${LLVM_DIR}/lib/MC/MCSectionELF.cpp
    ${LLVM_DIR}/lib/MC/MCSectionMachO.cpp
    ${LLVM_DIR}/lib/MC/MCSectionWasm.cpp
    ${LLVM_DIR}/lib/MC/MCStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCSubtargetInfo.cpp
    ${LLVM_DIR}/lib/MC/MCSymbol.cpp
    ${LLVM_DIR}/lib/MC/MCSymbolELF.cpp
    ${LLVM_DIR}/lib/MC/MCTargetOptions.cpp
    ${LLVM_DIR}/lib/MC/MCValue.cpp
    ${LLVM_DIR}/lib/MC/MCWasmStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCWin64EH.cpp
    ${LLVM_DIR}/lib/MC/MCWinCOFFStreamer.cpp
    ${LLVM_DIR}/lib/MC/MCWinEH.cpp
    ${LLVM_DIR}/lib/MC/MachObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/StringTableBuilder.cpp
    ${LLVM_DIR}/lib/MC/SubtargetFeature.cpp
    ${LLVM_DIR}/lib/MC/WasmObjectWriter.cpp
    ${LLVM_DIR}/lib/MC/WinCOFFObjectWriter.cpp
    ${LLVM_DIR}/lib/Object/Archive.cpp
    ${LLVM_DIR}/lib/Object/Binary.cpp
    ${LLVM_DIR}/lib/Object/COFFObjectFile.cpp
    ${LLVM_DIR}/lib/Object/ELF.cpp
    ${LLVM_DIR}/lib/Object/ELFObjectFile.cpp
    ${LLVM_DIR}/lib/Object/Error.cpp
    ${LLVM_DIR}/lib/Object/IRObjectFile.cpp
    ${LLVM_DIR}/lib/Object/IRSymtab.cpp
    ${LLVM_DIR}/lib/Object/MachOObjectFile.cpp
    ${LLVM_DIR}/lib/Object/MachOUniversal.cpp
    ${LLVM_DIR}/lib/Object/ModuleSymbolTable.cpp
    ${LLVM_DIR}/lib/Object/ObjectFile.cpp
    ${LLVM_DIR}/lib/Object/RecordStreamer.cpp
    ${LLVM_DIR}/lib/Object/SymbolicFile.cpp
    ${LLVM_DIR}/lib/Object/WasmObjectFile.cpp
    ${LLVM_DIR}/lib/Object/WindowsResource.cpp
    ${LLVM_DIR}/lib/ProfileData/InstrProf.cpp
    ${LLVM_DIR}/lib/Support/APFloat.cpp
    ${LLVM_DIR}/lib/Support/APInt.cpp
    ${LLVM_DIR}/lib/Support/APSInt.cpp
    ${LLVM_DIR}/lib/Support/ARMAttributeParser.cpp
    ${LLVM_DIR}/lib/Support/ARMBuildAttrs.cpp
    ${LLVM_DIR}/lib/Support/BinaryStreamError.cpp
    ${LLVM_DIR}/lib/Support/BinaryStreamReader.cpp
    ${LLVM_DIR}/lib/Support/BinaryStreamRef.cpp
    ${LLVM_DIR}/lib/Support/BinaryStreamWriter.cpp
    ${LLVM_DIR}/lib/Support/BlockFrequency.cpp
    ${LLVM_DIR}/lib/Support/BranchProbability.cpp
    ${LLVM_DIR}/lib/Support/Chrono.cpp
    ${LLVM_DIR}/lib/Support/CodeGenCoverage.cpp
    ${LLVM_DIR}/lib/Support/CommandLine.cpp
    ${LLVM_DIR}/lib/Support/Compression.cpp
    ${LLVM_DIR}/lib/Support/ConvertUTF.cpp
    ${LLVM_DIR}/lib/Support/ConvertUTFWrapper.cpp
    ${LLVM_DIR}/lib/Support/DJB.cpp
    ${LLVM_DIR}/lib/Support/DataExtractor.cpp
    ${LLVM_DIR}/lib/Support/Debug.cpp
    ${LLVM_DIR}/lib/Support/DebugCounter.cpp
    ${LLVM_DIR}/lib/Support/DynamicLibrary.cpp
    ${LLVM_DIR}/lib/Support/Errno.cpp
    ${LLVM_DIR}/lib/Support/Error.cpp
    ${LLVM_DIR}/lib/Support/ErrorHandling.cpp
    ${LLVM_DIR}/lib/Support/FoldingSet.cpp
    ${LLVM_DIR}/lib/Support/FormatVariadic.cpp
    ${LLVM_DIR}/lib/Support/FormattedStream.cpp
    ${LLVM_DIR}/lib/Support/GraphWriter.cpp
    ${LLVM_DIR}/lib/Support/Hashing.cpp
    ${LLVM_DIR}/lib/Support/Host.cpp
    ${LLVM_DIR}/lib/Support/IntEqClasses.cpp
    ${LLVM_DIR}/lib/Support/IntervalMap.cpp
    ${LLVM_DIR}/lib/Support/JamCRC.cpp
    ${LLVM_DIR}/lib/Support/KnownBits.cpp
    ${LLVM_DIR}/lib/Support/LEB128.cpp
    ${LLVM_DIR}/lib/Support/LineIterator.cpp
    ${LLVM_DIR}/lib/Support/Locale.cpp
    ${LLVM_DIR}/lib/Support/LowLevelType.cpp
    ${LLVM_DIR}/lib/Support/MD5.cpp
    ${LLVM_DIR}/lib/Support/ManagedStatic.cpp
    ${LLVM_DIR}/lib/Support/MathExtras.cpp
    ${LLVM_DIR}/lib/Support/Memory.cpp
    ${LLVM_DIR}/lib/Support/MemoryBuffer.cpp
    ${LLVM_DIR}/lib/Support/Mutex.cpp
    ${LLVM_DIR}/lib/Support/NativeFormatting.cpp
    ${LLVM_DIR}/lib/Support/Path.cpp
    ${LLVM_DIR}/lib/Support/PrettyStackTrace.cpp
    ${LLVM_DIR}/lib/Support/Process.cpp
    ${LLVM_DIR}/lib/Support/Program.cpp
    ${LLVM_DIR}/lib/Support/RWMutex.cpp
    ${LLVM_DIR}/lib/Support/RandomNumberGenerator.cpp
    ${LLVM_DIR}/lib/Support/Regex.cpp
    ${LLVM_DIR}/lib/Support/SHA1.cpp
    ${LLVM_DIR}/lib/Support/ScaledNumber.cpp
    ${LLVM_DIR}/lib/Support/ScopedPrinter.cpp
    ${LLVM_DIR}/lib/Support/Signals.cpp
    ${LLVM_DIR}/lib/Support/SmallPtrSet.cpp
    ${LLVM_DIR}/lib/Support/SmallVector.cpp
    ${LLVM_DIR}/lib/Support/SourceMgr.cpp
    ${LLVM_DIR}/lib/Support/Statistic.cpp
    ${LLVM_DIR}/lib/Support/StringExtras.cpp
    ${LLVM_DIR}/lib/Support/StringMap.cpp
    ${LLVM_DIR}/lib/Support/StringRef.cpp
    ${LLVM_DIR}/lib/Support/StringSaver.cpp
    ${LLVM_DIR}/lib/Support/TargetParser.cpp
    ${LLVM_DIR}/lib/Support/TargetRegistry.cpp
    ${LLVM_DIR}/lib/Support/Threading.cpp
    ${LLVM_DIR}/lib/Support/Timer.cpp
    ${LLVM_DIR}/lib/Support/ToolOutputFile.cpp
    ${LLVM_DIR}/lib/Support/Triple.cpp
    ${LLVM_DIR}/lib/Support/Twine.cpp
    ${LLVM_DIR}/lib/Support/Unicode.cpp
    ${LLVM_DIR}/lib/Support/UnicodeCaseFold.cpp
    ${LLVM_DIR}/lib/Support/Valgrind.cpp
    ${LLVM_DIR}/lib/Support/YAMLParser.cpp
    ${LLVM_DIR}/lib/Support/YAMLTraits.cpp
    ${LLVM_DIR}/lib/Support/circular_raw_ostream.cpp
    ${LLVM_DIR}/lib/Support/raw_ostream.cpp
    ${LLVM_DIR}/lib/Support/regcomp.c
    ${LLVM_DIR}/lib/Support/regerror.c
    ${LLVM_DIR}/lib/Support/regexec.c
    ${LLVM_DIR}/lib/Support/regfree.c
    ${LLVM_DIR}/lib/Support/regstrlcpy.c
    ${LLVM_DIR}/lib/Target/TargetLoweringObjectFile.cpp
    ${LLVM_DIR}/lib/Target/TargetMachine.cpp
    ${LLVM_DIR}/lib/Transforms/Coroutines/CoroCleanup.cpp
    ${LLVM_DIR}/lib/Transforms/Coroutines/CoroEarly.cpp
    ${LLVM_DIR}/lib/Transforms/Coroutines/CoroElide.cpp
    ${LLVM_DIR}/lib/Transforms/Coroutines/CoroFrame.cpp
    ${LLVM_DIR}/lib/Transforms/Coroutines/CoroSplit.cpp
    ${LLVM_DIR}/lib/Transforms/Coroutines/Coroutines.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineAddSub.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineAndOrXor.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineCalls.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineCasts.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineCompares.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineLoadStoreAlloca.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineMulDivRem.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombinePHI.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineSelect.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineShifts.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineSimplifyDemanded.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstCombineVectorOps.cpp
    ${LLVM_DIR}/lib/Transforms/InstCombine/InstructionCombining.cpp
    ${LLVM_DIR}/lib/Transforms/IPO/BarrierNoopPass.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/ADCE.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/ConstantHoisting.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/DeadStoreElimination.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/EarlyCSE.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/GVN.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/LICM.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/LoopStrengthReduce.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/MergeICmps.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/PartiallyInlineLibCalls.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/Reassociate.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/SCCP.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/SROA.cpp
    ${LLVM_DIR}/lib/Transforms/Scalar/SimplifyCFGPass.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/BasicBlockUtils.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/BreakCriticalEdges.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/BuildLibCalls.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/BypassSlowDivision.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/CloneFunction.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/DemoteRegToStack.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/EntryExitInstrumenter.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/EscapeEnumerator.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/InlineFunction.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LCSSA.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/Local.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LoopSimplify.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LoopUtils.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/LowerInvoke.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/OrderedInstructions.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/PromoteMemoryToRegister.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/SSAUpdater.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/SimplifyCFG.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/SimplifyLibCalls.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/SymbolRewriter.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/VNCoercion.cpp
    ${LLVM_DIR}/lib/Transforms/Utils/ValueMapper.cpp
    ${LLVM_DIR}/../stubs/Stubs.cpp
)

    list(APPEND LLVM_LIST
        ${LLVM_DIR}/lib/Target/X86/AsmParser/X86AsmInstrumentation.cpp
        ${LLVM_DIR}/lib/Target/X86/AsmParser/X86AsmParser.cpp
        ${LLVM_DIR}/lib/Target/X86/InstPrinter/X86ATTInstPrinter.cpp
        ${LLVM_DIR}/lib/Target/X86/InstPrinter/X86InstComments.cpp
        ${LLVM_DIR}/lib/Target/X86/InstPrinter/X86InstPrinterCommon.cpp
        ${LLVM_DIR}/lib/Target/X86/InstPrinter/X86IntelInstPrinter.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86AsmBackend.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86ELFObjectWriter.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MCAsmInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MCCodeEmitter.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MCTargetDesc.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86MachObjectWriter.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86WinCOFFObjectWriter.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86WinCOFFStreamer.cpp
        ${LLVM_DIR}/lib/Target/X86/MCTargetDesc/X86WinCOFFTargetStreamer.cpp
        ${LLVM_DIR}/lib/Target/X86/ShadowCallStack.cpp
        ${LLVM_DIR}/lib/Target/X86/TargetInfo/X86TargetInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/Utils/X86ShuffleDecode.cpp
        ${LLVM_DIR}/lib/Target/X86/X86AsmPrinter.cpp
        ${LLVM_DIR}/lib/Target/X86/X86AvoidStoreForwardingBlocks.cpp
        ${LLVM_DIR}/lib/Target/X86/X86CallFrameOptimization.cpp
        ${LLVM_DIR}/lib/Target/X86/X86CallLowering.cpp
        ${LLVM_DIR}/lib/Target/X86/X86CallingConv.cpp
        ${LLVM_DIR}/lib/Target/X86/X86CmovConversion.cpp
        ${LLVM_DIR}/lib/Target/X86/X86DomainReassignment.cpp
        ${LLVM_DIR}/lib/Target/X86/X86EvexToVex.cpp
        ${LLVM_DIR}/lib/Target/X86/X86ExpandPseudo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86FastISel.cpp
        ${LLVM_DIR}/lib/Target/X86/X86FixupBWInsts.cpp
        ${LLVM_DIR}/lib/Target/X86/X86FixupLEAs.cpp
        ${LLVM_DIR}/lib/Target/X86/X86FixupSetCC.cpp
        ${LLVM_DIR}/lib/Target/X86/X86FlagsCopyLowering.cpp
        ${LLVM_DIR}/lib/Target/X86/X86FloatingPoint.cpp
        ${LLVM_DIR}/lib/Target/X86/X86FrameLowering.cpp
        ${LLVM_DIR}/lib/Target/X86/X86ISelDAGToDAG.cpp
        ${LLVM_DIR}/lib/Target/X86/X86ISelLowering.cpp
        ${LLVM_DIR}/lib/Target/X86/X86IndirectBranchTracking.cpp
        ${LLVM_DIR}/lib/Target/X86/X86InstrFMA3Info.cpp
        ${LLVM_DIR}/lib/Target/X86/X86InstrFoldTables.cpp
        ${LLVM_DIR}/lib/Target/X86/X86InstrInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86InstructionSelector.cpp
        ${LLVM_DIR}/lib/Target/X86/X86InterleavedAccess.cpp
        ${LLVM_DIR}/lib/Target/X86/X86LegalizerInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86MCInstLower.cpp
        ${LLVM_DIR}/lib/Target/X86/X86MachineFunctionInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86MacroFusion.cpp
        ${LLVM_DIR}/lib/Target/X86/X86OptimizeLEAs.cpp
        ${LLVM_DIR}/lib/Target/X86/X86PadShortFunction.cpp
        ${LLVM_DIR}/lib/Target/X86/X86RegisterBankInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86RegisterInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86RetpolineThunks.cpp
        ${LLVM_DIR}/lib/Target/X86/X86SelectionDAGInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86ShuffleDecodeConstantPool.cpp
        ${LLVM_DIR}/lib/Target/X86/X86SpeculativeLoadHardening.cpp
        ${LLVM_DIR}/lib/Target/X86/X86Subtarget.cpp
        ${LLVM_DIR}/lib/Target/X86/X86TargetMachine.cpp
        ${LLVM_DIR}/lib/Target/X86/X86TargetObjectFile.cpp
        ${LLVM_DIR}/lib/Target/X86/X86TargetTransformInfo.cpp
        ${LLVM_DIR}/lib/Target/X86/X86VZeroUpper.cpp
        ${LLVM_DIR}/lib/Target/X86/X86WinAllocaExpander.cpp
        ${LLVM_DIR}/lib/Target/X86/X86WinEHState.cpp
    )

# This relies on boost, which we are not going to bring aboard.
#if(REACTOR_EMIT_DEBUG_INFO)
#    list(APPEND LLVM_LIST
#        ${LLVM_DIR}/lib/Demangle/ItaniumDemangle.cpp
#    )
#endif(REACTOR_EMIT_DEBUG_INFO)

set(LLVM_INCLUDE_DIR "")

if(WINDOWS)
    list(APPEND LLVM_INCLUDE_DIR ${LLVM_CONFIG_DIR}/windows/include)
elseif(LINUX_X86_64)
    list(APPEND LLVM_INCLUDE_DIR ${LLVM_CONFIG_DIR}/linux/include)
    list(APPEND LIBBACKTRACE_INCLUDE_DIR ${LIBBACKTRACE_CONFIG_DIR}/linux/include)
elseif(DARWIN_X86_64)
    list(APPEND LLVM_INCLUDE_DIR ${LLVM_CONFIG_DIR}/darwin/include)
    list(APPEND LIBBACKTRACE_INCLUDE_DIR ${LIBBACKTRACE_CONFIG_DIR}/darwin/include)
endif()

list(APPEND LLVM_INCLUDE_DIR
    ${LLVM_DIR}/include
    ${LLVM_DIR}/lib/Target/AArch64
    ${LLVM_DIR}/lib/Target/ARM
    ${LLVM_DIR}/lib/Target/Mips
    ${LLVM_DIR}/lib/Target/X86
    ${LLVM_CONFIG_DIR}/common/include
    ${LLVM_CONFIG_DIR}/common/lib/IR
    ${LLVM_CONFIG_DIR}/common/lib/Target/AArch64
    ${LLVM_CONFIG_DIR}/common/lib/Target/ARM
    ${LLVM_CONFIG_DIR}/common/lib/Target/Mips
    ${LLVM_CONFIG_DIR}/common/lib/Target/X86
    ${LLVM_CONFIG_DIR}/common/lib/Transforms/InstCombine
)

# Suppress known LLVM warnings
set(LLVM_COMPILE_OPTIONS)
if(WIDNOWS_MSVC_X86_64)
    list(APPEND LLVM_COMPILE_OPTIONS
        "/wd4141" # 'inline': used more than once
        "/wd4146" # unary minus operator applied to unsigned type, result still unsigned
        "/wd4244" # 'X': conversion from 'uint64_t' to 'const unsigned int', possible loss of data
        "/wd4267" # 'X': conversion from 'size_t' to 'unsigned int', possible loss of data
        "/wd4291" # 'void X new(size_t,unsigned int,unsigned int)': no matching operator delete found; memory will not be freed if initialization throws an exception
        "/wd4319" # 'X': zero extending 'unsigned int' to 'uint64_t' of greater size
        "/wd4624" # 'X': destructor was implicitly defined as deleted
        "/wd4805" # 'X': unsafe mix of type 'unsigned int' and type 'bool' in operation
        "/wd4996" # 'X': Call to 'Y' with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct.
    )
endif()

set(llvm_src ${LLVM_LIST})
android_add_library(llvm)
target_include_directories(llvm PRIVATE ${LLVM_INCLUDE_DIR})
target_compile_options(llvm PRIVATE ${LLVM_COMPILE_OPTIONS})
set_target_properties(llvm PROPERTIES POSITION_INDEPENDENT_CODE 1)

# Add required libraries for LLVM
android_target_link_libraries(llvm linux-x86_64 PRIVATE dl)

#
# Subzero
#

if(${REACTOR_BACKEND} STREQUAL "Subzero")
  set(SUBZERO_LIST
      ${SUBZERO_DIR}/src/IceAssembler.cpp
      ${SUBZERO_DIR}/src/IceCfg.cpp
      ${SUBZERO_DIR}/src/IceCfgNode.cpp
      ${SUBZERO_DIR}/src/IceClFlags.cpp
      ${SUBZERO_DIR}/src/IceELFObjectWriter.cpp
      ${SUBZERO_DIR}/src/IceELFSection.cpp
      ${SUBZERO_DIR}/src/IceFixups.cpp
      ${SUBZERO_DIR}/src/IceGlobalContext.cpp
      ${SUBZERO_DIR}/src/IceGlobalInits.cpp
      ${SUBZERO_DIR}/src/IceInst.cpp
      ${SUBZERO_DIR}/src/IceInstrumentation.cpp
      ${SUBZERO_DIR}/src/IceIntrinsics.cpp
      ${SUBZERO_DIR}/src/IceLiveness.cpp
      ${SUBZERO_DIR}/src/IceLoopAnalyzer.cpp
      ${SUBZERO_DIR}/src/IceMangling.cpp
      ${SUBZERO_DIR}/src/IceMemory.cpp
      ${SUBZERO_DIR}/src/IceOperand.cpp
      ${SUBZERO_DIR}/src/IceRangeSpec.cpp
      ${SUBZERO_DIR}/src/IceRegAlloc.cpp
      ${SUBZERO_DIR}/src/IceRevision.cpp
      ${SUBZERO_DIR}/src/IceRNG.cpp
      ${SUBZERO_DIR}/src/IceSwitchLowering.cpp
      ${SUBZERO_DIR}/src/IceTargetLowering.cpp
      ${SUBZERO_DIR}/src/IceThreading.cpp
      ${SUBZERO_DIR}/src/IceTimerTree.cpp
      ${SUBZERO_DIR}/src/IceTypes.cpp
      ${SUBZERO_DIR}/src/IceVariableSplitting.cpp)

  # FIXME: Shouldn't depend on external source files directly.
  list(APPEND SUBZERO_LIST ${SOURCE_DIR}/Common/Memory.cpp)

  list(APPEND SUBZERO_LIST ${SUBZERO_DIR}/src/IceTargetLoweringX86.cpp ${SUBZERO_DIR}/src/IceInstX8664.cpp
              ${SUBZERO_DIR}/src/IceTargetLoweringX8664.cpp)
  set(SUBZERO_TARGET X8664)

  file(GLOB_RECURSE SUBZERO_DEPENDENCIES_LIST ${SUBZERO_LLVM_DIR}/*.cpp ${SUBZERO_LLVM_DIR}/*.c ${SUBZERO_LLVM_DIR}/*.h)

  set(SUBZERO_REACTOR_LIST
      ${SOURCE_DIR}/Reactor/SubzeroReactor.cpp
      ${SOURCE_DIR}/Reactor/Routine.cpp
      ${SOURCE_DIR}/Reactor/Optimizer.cpp
      ${SOURCE_DIR}/Reactor/Nucleus.hpp
      ${SOURCE_DIR}/Reactor/Routine.hpp
      ${SOURCE_DIR}/Reactor/Debug.cpp
      ${SOURCE_DIR}/Reactor/Debug.hpp
      ${SOURCE_DIR}/Reactor/ExecutableMemory.cpp
      ${SOURCE_DIR}/Reactor/ExecutableMemory.hpp)

  set(SUBZERO_INCLUDE_DIR ${SUBZERO_DIR}/ ${SUBZERO_LLVM_DIR}/include/ ${SUBZERO_DIR}/pnacl-llvm/include/)

  if(WINDOWS)
    list(APPEND SUBZERO_INCLUDE_DIR ${SUBZERO_LLVM_DIR}/build/Windows/include/)
  elseif(LINUX_X86_64)
    list(APPEND SUBZERO_INCLUDE_DIR ${SUBZERO_LLVM_DIR}/build/Linux/include/)
  elseif(DARWIN_X86_64)
    list(APPEND SUBZERO_INCLUDE_DIR ${SUBZERO_LLVM_DIR}/build/MacOS/include/)
  endif()

  set(ReactorSubzero_src ${SUBZERO_LIST} ${SUBZERO_DEPENDENCIES_LIST} ${SUBZERO_REACTOR_LIST})
  android_add_library(ReactorSubzero)
  target_compile_options(ReactorSubzero PRIVATE ${SWIFTSHADER_CXX_FLAGS})
  target_compile_definitions(ReactorSubzero PRIVATE ${SWIFTSHADER_DEFS})
  target_include_directories(ReactorSubzero PRIVATE ${SUBZERO_INCLUDE_DIR})
  target_compile_definitions(ReactorSubzero
                             PRIVATE
                             "-DSZTARGET=${SUBZERO_TARGET}"
                             "-DALLOW_DUMP=0"
                             "-DALLOW_TIMERS=0"
                             "-DALLOW_LLVM_CL=0"
                             "-DALLOW_LLVM_IR=0"
                             "-DALLOW_LLVM_IR_AS_INPUT=0"
                             "-DALLOW_MINIMAL_BUILD=0"
                             "-DALLOW_WASM=0"
                             "-DICE_THREAD_LOCAL_HACK=0")
  android_target_compile_definitions(ReactorSubzero windows PRIVATE -DSUBZERO_USE_MICROSOFT_ABI)

endif()

#
# Include Directories
#

set(COMMON_INCLUDE_DIR ${SOURCE_DIR} ${SWIFTSHADER_ROOT}/include ${LLVM_INCLUDE_DIR})
set(OPENGL_INCLUDE_DIR ${OPENGL_DIR} ${COMMON_INCLUDE_DIR})
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR} ${COMMON_INCLUDE_DIR})

###########################################################
# File Lists
###########################################################

file(GLOB_RECURSE SWIFTSHADER_LIST
    ${SOURCE_DIR}/Common/*.cpp
    ${SOURCE_DIR}/Common/*.h
    ${SOURCE_DIR}/Common/*.hpp
    ${SOURCE_DIR}/Renderer/*.cpp
    ${SOURCE_DIR}/Renderer/*.h
    ${SOURCE_DIR}/Renderer/*.hpp
    ${SOURCE_DIR}/Shader/*.cpp
    ${SOURCE_DIR}/Shader/*.h
    ${SOURCE_DIR}/Shader/*.hpp
)
list(APPEND SWIFTSHADER_LIST
    ${SOURCE_DIR}/Main/Config.cpp
    ${SOURCE_DIR}/Main/Config.hpp
    ${SOURCE_DIR}/Main/FrameBuffer.cpp
    ${SOURCE_DIR}/Main/FrameBuffer.hpp
    ${SOURCE_DIR}/Main/SwiftConfig.cpp
    ${SOURCE_DIR}/Main/SwiftConfig.hpp
)
list(REMOVE_ITEM SWIFTSHADER_LIST
    ${SOURCE_DIR}/Common/DebugAndroid.cpp
    ${SOURCE_DIR}/Common/DebugAndroid.hpp
    ${SOURCE_DIR}/Common/GrallocAndroid.cpp
    ${SOURCE_DIR}/Common/GrallocAndroid.hpp
)

if(REACTOR_EMIT_DEBUG_INFO)
    set(LIBBACKTRACE_LIST
        ${LIBBACKTRACE_DIR}/atomic.c
        ${LIBBACKTRACE_DIR}/backtrace.c
        ${LIBBACKTRACE_DIR}/backtrace.h
        ${LIBBACKTRACE_DIR}/dwarf.c
        ${LIBBACKTRACE_DIR}/fileline.c
        ${LIBBACKTRACE_DIR}/filenames.h
        ${LIBBACKTRACE_DIR}/internal.h
        ${LIBBACKTRACE_DIR}/mmap.c
        ${LIBBACKTRACE_DIR}/mmapio.c
        ${LIBBACKTRACE_DIR}/posix.c
        ${LIBBACKTRACE_DIR}/print.c
        ${LIBBACKTRACE_DIR}/simple.c
        ${LIBBACKTRACE_DIR}/sort.c
        ${LIBBACKTRACE_DIR}/state.c
        ${LIBBACKTRACE_INCLUDE_DIR}/backtrace-supported.h
        ${LIBBACKTRACE_INCLUDE_DIR}/config.h
    )

    if(WIN32)
        list(APPEND LIBBACKTRACE_LIST ${LIBBACKTRACE_DIR}/pecoff.c)
    elseif(LINUX)
        list(APPEND LIBBACKTRACE_LIST ${LIBBACKTRACE_DIR}/elf.c)
    elseif(APPLE)
        message(FATAL_ERROR "libbacktrace does not support mach-o yet")
    endif()
endif(REACTOR_EMIT_DEBUG_INFO)

set(REACTOR_LLVM_LIST
    ${SOURCE_DIR}/Reactor/Reactor.cpp
    ${SOURCE_DIR}/Reactor/Reactor.hpp
    ${SOURCE_DIR}/Reactor/LLVMReactor.cpp
    ${SOURCE_DIR}/Reactor/LLVMReactor.hpp
    ${SOURCE_DIR}/Reactor/LLVMReactorDebugInfo.cpp
    ${SOURCE_DIR}/Reactor/LLVMReactorDebugInfo.hpp
    ${SOURCE_DIR}/Reactor/Nucleus.hpp
    ${SOURCE_DIR}/Reactor/Routine.cpp
    ${SOURCE_DIR}/Reactor/Routine.hpp
    ${SOURCE_DIR}/Reactor/LLVMRoutine.cpp
    ${SOURCE_DIR}/Reactor/LLVMRoutine.hpp
    ${SOURCE_DIR}/Reactor/CPUID.cpp
    ${SOURCE_DIR}/Reactor/CPUID.hpp
    ${SOURCE_DIR}/Reactor/Debug.cpp
    ${SOURCE_DIR}/Reactor/Debug.hpp
    ${SOURCE_DIR}/Reactor/ExecutableMemory.cpp
    ${SOURCE_DIR}/Reactor/ExecutableMemory.hpp
)

file(GLOB_RECURSE EGL_LIST
    ${OPENGL_DIR}/libEGL/*.cpp
    ${OPENGL_DIR}/libEGL/*.h
    ${OPENGL_DIR}/libEGL/*.hpp
    ${OPENGL_DIR}/common/Object.cpp
    ${OPENGL_DIR}/common/Object.hpp
    ${OPENGL_DIR}/common/debug.cpp
    ${OPENGL_DIR}/common/debug.h
    ${SOURCE_DIR}/Common/SharedLibrary.cpp
    ${SOURCE_DIR}/Common/SharedLibrary.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

file(GLOB_RECURSE GLES2_LIST
    ${OPENGL_DIR}/libGLESv2/*.cpp
    ${OPENGL_DIR}/libGLESv2/*.h
    ${OPENGL_DIR}/libGLESv2/*.hpp
    ${OPENGL_DIR}/common/*.cpp
    ${OPENGL_DIR}/common/*.h
    ${OPENGL_DIR}/common/*.hpp
    ${SOURCE_DIR}/Common/SharedLibrary.cpp
    ${SOURCE_DIR}/Common/SharedLibrary.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/KHR/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GLES2/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GLES3/*.h
)

file(GLOB_RECURSE GLES_CM_LIST
    ${OPENGL_DIR}/libGLES_CM/*.cpp
    ${OPENGL_DIR}/libGLES_CM/*.h
    ${OPENGL_DIR}/libGLES_CM/*.hpp
    ${OPENGL_DIR}/common/*.cpp
    ${OPENGL_DIR}/common/*.h
    ${OPENGL_DIR}/common/*.hpp
    ${SOURCE_DIR}/Common/SharedLibrary.cpp
    ${SOURCE_DIR}/Common/SharedLibrary.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/KHR/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GLES/*.h
)

file(GLOB_RECURSE OPENGL_COMPILER_LIST
    ${OPENGL_COMPILER_DIR}/*.cpp
    ${OPENGL_COMPILER_DIR}/*.h
    ${OPENGL_COMPILER_DIR}/*.hpp
)
file(GLOB_RECURSE OPENGL_COMPILER_OS_SPECIFIC_LIST
    ${OPENGL_COMPILER_DIR}/ossource*.cpp
    ${OPENGL_COMPILER_DIR}/ossource*.h
    ${OPENGL_COMPILER_DIR}/ossource*.hpp
)
list(REMOVE_ITEM OPENGL_COMPILER_LIST ${OPENGL_COMPILER_OS_SPECIFIC_LIST})

file(GLOB_RECURSE VULKAN_LIST
    ${VULKAN_DIR}/*.cpp
    ${VULKAN_DIR}/*.h
    ${VULKAN_DIR}/*.hpp
    ${SOURCE_DIR}/System/CPUID.cpp
    ${SOURCE_DIR}/System/CPUID.hpp
    ${SOURCE_DIR}/System/Configurator.cpp
    ${SOURCE_DIR}/System/Configurator.hpp
    ${SOURCE_DIR}/System/Debug.cpp
    ${SOURCE_DIR}/System/Debug.hpp
    ${SOURCE_DIR}/System/Half.cpp
    ${SOURCE_DIR}/System/Half.hpp
    ${SOURCE_DIR}/System/Math.cpp
    ${SOURCE_DIR}/System/Math.hpp
    ${SOURCE_DIR}/System/Memory.cpp
    ${SOURCE_DIR}/System/Memory.hpp
    ${SOURCE_DIR}/System/Resource.cpp
    ${SOURCE_DIR}/System/Resource.hpp
    ${SOURCE_DIR}/System/Socket.cpp
    ${SOURCE_DIR}/System/Socket.hpp
    ${SOURCE_DIR}/System/Synchronization.hpp
    ${SOURCE_DIR}/System/Thread.hpp
    ${SOURCE_DIR}/System/Timer.cpp
    ${SOURCE_DIR}/System/Timer.hpp
    ${SOURCE_DIR}/Device/*.cpp
    ${SOURCE_DIR}/Device/*.hpp
    ${SOURCE_DIR}/Pipeline/*.cpp
    ${SOURCE_DIR}/Pipeline/*.hpp
    ${SOURCE_DIR}/WSI/VkSurfaceKHR.cpp
    ${SOURCE_DIR}/WSI/VkSurfaceKHR.hpp
    ${SOURCE_DIR}/WSI/VkSwapchainKHR.cpp
    ${SOURCE_DIR}/WSI/VkSwapchainKHR.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/*.h}
)

###########################################################
# Append OS specific files to lists
###########################################################

if(WINDOWS)
    list(APPEND SWIFTSHADER_LIST
        ${SOURCE_DIR}/Main/FrameBufferDD.cpp
        ${SOURCE_DIR}/Main/FrameBufferDD.hpp
        ${SOURCE_DIR}/Main/FrameBufferGDI.cpp
        ${SOURCE_DIR}/Main/FrameBufferGDI.hpp
        ${SOURCE_DIR}/Main/FrameBufferWin.cpp
        ${SOURCE_DIR}/Main/FrameBufferWin.hpp
    )
    list(APPEND OPENGL_COMPILER_LIST ${OPENGL_COMPILER_DIR}/ossource_win.cpp)
    list(APPEND EGL_LIST ${OPENGL_DIR}/libEGL/libEGL.rc)
    list(APPEND GLES2_LIST ${OPENGL_DIR}/libGLESv2/libGLESv2.rc)
    list(APPEND GLES_CM_LIST ${OPENGL_DIR}/libGLES_CM/libGLES_CM.rc)
elseif(LINUX_X86_64)
    list(APPEND SWIFTSHADER_LIST
        ${SOURCE_DIR}/Main/FrameBufferX11.cpp
        ${SOURCE_DIR}/Main/FrameBufferX11.hpp
        ${SOURCE_DIR}/Common/SharedLibrary.hpp
        ${SOURCE_DIR}/Main/libX11.cpp
        ${SOURCE_DIR}/Main/libX11.hpp
    )
    list(APPEND EGL_LIST
        ${SOURCE_DIR}/Common/SharedLibrary.hpp
        ${SOURCE_DIR}/Main/libX11.cpp
        ${SOURCE_DIR}/Main/libX11.hpp
    )
    list(APPEND OPENGL_COMPILER_LIST
        ${OPENGL_COMPILER_DIR}/ossource_posix.cpp
    )

    list(APPEND VULKAN_LIST
        ${SOURCE_DIR}/WSI/libX11.cpp
        ${SOURCE_DIR}/WSI/libX11.hpp
        ${SOURCE_DIR}/WSI/XlibSurfaceKHR.cpp
        ${SOURCE_DIR}/WSI/XlibSurfaceKHR.hpp
    )
elseif(DARWIN_X86_64)
    list(APPEND SWIFTSHADER_LIST
        ${SOURCE_DIR}/Main/FrameBufferOSX.mm
        ${SOURCE_DIR}/Main/FrameBufferOSX.hpp
    )
    list(APPEND EGL_LIST
        ${OPENGL_DIR}/libEGL/OSXUtils.mm
        ${OPENGL_DIR}/libEGL/OSXUtils.hpp
    )
    list(APPEND OPENGL_COMPILER_LIST
        ${OPENGL_COMPILER_DIR}/ossource_posix.cpp
    )
elseif(ANDROID)
    list(APPEND SWIFTSHADER_LIST
        ${SOURCE_DIR}/Main/FrameBufferAndroid.cpp
        ${SOURCE_DIR}/Main/FrameBufferAndroid.hpp
    )
    list(APPEND OPENGL_COMPILER_LIST
        ${OPENGL_COMPILER_DIR}/ossource_posix.cpp
    )
endif()


if(WINDOWS)
  set(OS_LIBS odbc32 odbccp32 WS2_32 dxguid)
elseif(LINUX_X86_64)
  set(OS_LIBS dl pthread)
elseif(DARWIN_X86_64)
  set(OS_LIBS "-framework Quartz" "-framework CoreFoundation" "-framework IOSurface" "-framework Cocoa")
endif()

#
# SwiftShader Targets
#
set(SwiftShader_src ${SWIFTSHADER_LIST})
android_add_library(SwiftShader)
target_include_directories(SwiftShader PRIVATE ${COMMON_INCLUDE_DIR})
target_compile_definitions(SwiftShader PRIVATE "-DNO_SANITIZE_FUNCTION=")
target_link_libraries(SwiftShader PRIVATE ${OS_LIBS})
target_compile_options(SwiftShader PRIVATE ${SWIFTSHADER_CXX_FLAGS} ${SWIFTSHADER_COMPILE_OPTIONS})
target_compile_definitions(SwiftShader PRIVATE ${SWIFTSHADER_DEFS})

# set(ReactorLLVM_src ${REACTOR_LLVM_LIST}) android_add_library(ReactorLLVM) target_include_directories(ReactorLLVM
# PRIVATE ${COMMON_INCLUDE_DIR}) target_link_libraries(ReactorLLVM PRIVATE llvm ${OS_LIBS})

if(${REACTOR_BACKEND} STREQUAL "LLVM")
  set(Reactor ReactorLLVM)
  set(ReactorLLVM_src ${REACTOR_LLVM_LIST})
  android_add_library(ReactorLLVM)
  target_include_directories(ReactorLLVM PRIVATE ${COMMON_INCLUDE_DIR})
  target_compile_options(ReactorLLVM PRIVATE ${SWIFTSHADER_COMPILE_OPTIONS} ${SWIFTSHADER_CXX_FLAGS})
  set_target_properties(ReactorLLVM PROPERTIES POSITION_INDEPENDENT_CODE 1)
  target_link_libraries(ReactorLLVM PRIVATE llvm ${OS_LIBS})
elseif(${REACTOR_BACKEND} STREQUAL "Subzero")
  set(Reactor ReactorSubzero)
else()
  message(FATAL_ERROR "REACTOR_BACKEND must be 'LLVM' or 'Subzero'")
endif()

set(GLCompiler_src ${OPENGL_COMPILER_LIST})
android_add_library(GLCompiler)
target_include_directories(GLCompiler PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(GLCompiler PRIVATE ${OS_LIBS})
target_compile_options(GLCompiler PRIVATE ${SWIFTSHADER_CXX_FLAGS} ${SWIFTSHADER_COMPILE_OPTIONS})
target_compile_definitions(GLCompiler PRIVATE ${SWIFTSHADER_DEFS})

if(BUILD_EGL)
  set(libEGL_src ${EGL_LIST})
  android_add_shared_library(libEGL)
  target_compile_options(libEGL PRIVATE ${SWIFTSHADER_CXX_FLAGS} ${SWIFTSHADER_COMPILE_OPTIONS})
  target_compile_definitions(libEGL
                             PRIVATE
                             ${SWIFTSHADER_DEFS}
                             "-DEGL_EGLEXT_PROTOTYPES"
                             "-DEGLAPI="
                             "-DNO_SANITIZE_FUNCTION=")
  set_target_properties(libEGL PROPERTIES
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader)
  target_include_directories(libEGL PRIVATE ${OPENGL_INCLUDE_DIR})
  set_shared_library_export_map(libEGL ${SOURCE_DIR}/OpenGL/libEGL)
  target_link_libraries(libEGL PRIVATE ${OS_LIBS})
  install(TARGETS libEGL
      RUNTIME DESTINATION lib64/gles_swiftshader # We don't want windows to binplace dlls in the exe dir
      LIBRARY DESTINATION lib64/gles_swiftshader)
  android_extract_symbols(libEGL)
  android_upload_symbols(libEGL)


  # Needed for unit tests
  add_custom_command(TARGET libEGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libEGL> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

if(BUILD_GLESv2)
  set(libGLESv2_src ${GLES2_LIST})
  android_add_shared_library(libGLESv2)
  target_compile_options(libGLESv2 PRIVATE ${SWIFTSHADER_CXX_FLAGS} ${SWIFTSHADER_COMPILE_OPTIONS})
  target_compile_definitions(libGLESv2
                             PRIVATE
                             ${SWIFTSHADER_DEFS}
                             "-DGL_GLEXT_PROTOTYPES"
                             "-DGL_API="
                             "-DGL_APICALL="
                             "-DGLAPI="
                             "-DNO_SANITIZE_FUNCTION=")
  set_target_properties(libGLESv2 PROPERTIES
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader)
  set_shared_library_export_map(libGLESv2 ${SOURCE_DIR}/OpenGL/libGLESv2)
  target_include_directories(libGLESv2 PRIVATE ${OPENGL_INCLUDE_DIR})
  target_link_libraries(libGLESv2 PRIVATE SwiftShader ${Reactor} GLCompiler ${OS_LIBS})
  install(TARGETS libGLESv2
      RUNTIME DESTINATION lib64/gles_swiftshader # We don't want windows to binplace dlls in the exe dir
      LIBRARY DESTINATION lib64/gles_swiftshader)
  android_extract_symbols(libGLESv2)
  android_upload_symbols(libGLESv2)


  # Needed for unit tests
  add_custom_command(TARGET libGLESv2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libGLESv2> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

if(BUILD_GLES_CM)
  set(libGLES_CM_src ${GLES_CM_LIST})
  android_add_shared_library(libGLES_CM)
  target_include_directories(libGLES_CM PRIVATE ${OPENGL_INCLUDE_DIR})
  target_compile_options(libGLES_CM PRIVATE ${SWIFTSHADER_CXX_FLAGS} ${SWIFTSHADER_COMPILE_OPTIONS})
  target_compile_definitions(libGLES_CM
                             PRIVATE
                             ${SWIFTSHADER_DEFS}
                             "-DGL_GLEXT_PROTOTYPES"
                             "-DEGLAPI="
                             "-DGL_API="
                             "-DGL_APICALL="
                             "-DGLAPI=")
  set_target_properties(libGLES_CM PROPERTIES
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader)
  set_shared_library_export_map(libGLES_CM ${SOURCE_DIR}/OpenGL/libGLES_CM)
  target_link_libraries(libGLES_CM PRIVATE SwiftShader ${Reactor} GLCompiler ${OS_LIBS})
  install(TARGETS libGLES_CM
      RUNTIME DESTINATION lib64/gles_swiftshader # We don't want windows to binplace dlls in the exe dir
      LIBRARY DESTINATION lib64/gles_swiftshader)
  android_extract_symbols(libGLES_CM)
  android_upload_symbols(libGLES_CM)

  # Needed for unit tests
  add_custom_command(TARGET libGLES_CM POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:libGLES_CM> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

set(ReactorUnitTests_src ${SOURCE_DIR}/Reactor/ReactorUnitTests.cpp)

android_add_test(ReactorUnitTests)
target_link_libraries(ReactorUnitTests PRIVATE gtest Threads::Threads ${Reactor})
target_compile_options(ReactorUnitTests PRIVATE ${SWIFTSHADER_CXX_FLAGS})
target_compile_definitions(ReactorUnitTests PRIVATE ${SWIFTSHADER_DEFS})
android_target_link_libraries(ReactorUnitTests linux-x86_64 PRIVATE dl)

set(swiftshader_tests_src ${SWIFTSHADER_ROOT}/tests/GLESUnitTests/main.cpp
    ${SWIFTSHADER_ROOT}/tests/GLESUnitTests/unittests.cpp)
android_add_test(swiftshader_tests)
target_compile_options(swiftshader_tests PRIVATE ${SWIFTSHADER_CXX_FLAGS})
target_compile_definitions(swiftshader_tests PRIVATE ${SWIFTSHADER_DEFS})
target_include_directories(swiftshader_tests PRIVATE ${SWIFTSHADER_ROOT}/include)

target_link_libraries(swiftshader_tests PRIVATE libEGL libGLESv2 ${OS_LIBS} gmock_main)
