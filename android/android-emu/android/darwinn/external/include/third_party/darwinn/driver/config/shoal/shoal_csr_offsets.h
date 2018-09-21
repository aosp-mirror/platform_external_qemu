// AUTO GENERATED.

#ifndef THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CSR_OFFSETS_H_
#define THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CSR_OFFSETS_H_

#include "third_party/darwinn/driver/config/aon_reset_csr_offsets.h"
#include "third_party/darwinn/driver/config/breakpoint_csr_offsets.h"
#include "third_party/darwinn/driver/config/debug_hib_user_csr_offsets.h"
#include "third_party/darwinn/driver/config/debug_scalar_core_csr_offsets.h"
#include "third_party/darwinn/driver/config/debug_tile_csr_offsets.h"
#include "third_party/darwinn/driver/config/hib_kernel_csr_offsets.h"
#include "third_party/darwinn/driver/config/hib_user_csr_offsets.h"
#include "third_party/darwinn/driver/config/interrupt_csr_offsets.h"
#include "third_party/darwinn/driver/config/memory_csr_offsets.h"
#include "third_party/darwinn/driver/config/queue_csr_offsets.h"
#include "third_party/darwinn/driver/config/register_file_csr_offsets.h"
#include "third_party/darwinn/driver/config/scalar_core_csr_offsets.h"
#include "third_party/darwinn/driver/config/sync_flag_csr_offsets.h"
#include "third_party/darwinn/driver/config/tile_config_csr_offsets.h"
#include "third_party/darwinn/driver/config/tile_csr_offsets.h"
#include "third_party/darwinn/driver/config/trace_csr_offsets.h"
#include "third_party/darwinn/driver/config/wire_csr_offsets.h"

namespace platforms {
namespace darwinn {
namespace driver {
namespace config {

const InterruptCsrOffsets kShoalFatalErrIntInterruptCsrOffsets = {
    0x81c0,  // NOLINT: fatal_err_int_control
    0x81c8,  // NOLINT: fatal_err_int_status
};

const InterruptCsrOffsets kShoalScHostIntInterruptCsrOffsets = {
    0x81a0,  // NOLINT: sc_host_int_control
    0x81a8,  // NOLINT: sc_host_int_status
};

const InterruptCsrOffsets kShoalTopLevelIntInterruptCsrOffsets = {
    0x81b0,  // NOLINT: top_level_int_control
    0x81b8,  // NOLINT: top_level_int_status
};

const BreakpointCsrOffsets kShoalAvdatapopBreakpointCsrOffsets = {
    0x40e0,  // NOLINT: avDataPopRunControl
    0x40f0,  // NOLINT: avDataPopRunStatus
    0x40e8,  // NOLINT: avDataPopBreakPoint
};

const BreakpointCsrOffsets kShoalInfeedBreakpointCsrOffsets = {
    0x4160,  // NOLINT: infeedRunControl
    0x4168,  // NOLINT: infeedRunStatus
    0x4170,  // NOLINT: infeedBreakPoint
};

const BreakpointCsrOffsets kShoalOutfeedBreakpointCsrOffsets = {
    0x41a0,  // NOLINT: outfeedRunControl
    0x41a8,  // NOLINT: outfeedRunStatus
    0x41b0,  // NOLINT: outfeedBreakPoint
};

const BreakpointCsrOffsets kShoalParameterpopBreakpointCsrOffsets = {
    0x4120,  // NOLINT: parameterPopRunControl
    0x4130,  // NOLINT: parameterPopRunStatus
    0x4128,  // NOLINT: parameterPopBreakPoint
};

const BreakpointCsrOffsets kShoalScalarcoreBreakpointCsrOffsets = {
    0x4018,  // NOLINT: scalarCoreRunControl
    0x41e0,  // NOLINT: scalarCoreRunStatus
    0x4020,  // NOLINT: scalarCoreBreakPoint
};

const RegisterFileCsrOffsets kShoalPredicateRegisterFileCsrOffsets = {
    0x4500,  // NOLINT: predicateRegisterFile
};

const RegisterFileCsrOffsets kShoalScalarRegisterFileCsrOffsets = {
    0x4400,  // NOLINT: scalarRegisterFile
};

const SyncFlagCsrOffsets kShoalAvdataInfeedSyncFlagCsrOffsets = {
    0x4060,  // NOLINT: SyncCounter_AVDATA_INFEED
};

const SyncFlagCsrOffsets kShoalAvdataPopSyncFlagCsrOffsets = {
    0x4050,  // NOLINT: SyncCounter_AVDATA_POP
};

const SyncFlagCsrOffsets kShoalParameterInfeedSyncFlagCsrOffsets = {
    0x4068,  // NOLINT: SyncCounter_PARAMETER_INFEED
};

const SyncFlagCsrOffsets kShoalParameterPopSyncFlagCsrOffsets = {
    0x4058,  // NOLINT: SyncCounter_PARAMETER_POP
};

const SyncFlagCsrOffsets kShoalProducerASyncFlagCsrOffsets = {
    0x4078,  // NOLINT: SyncCounter_PRODUCER_A
};

const SyncFlagCsrOffsets kShoalProducerBSyncFlagCsrOffsets = {
    0x4080,  // NOLINT: SyncCounter_PRODUCER_B
};

const SyncFlagCsrOffsets kShoalRingOutfeedSyncFlagCsrOffsets = {
    0x4088,  // NOLINT: SyncCounter_RING_OUTFEED
};

const SyncFlagCsrOffsets kShoalScalarInfeedSyncFlagCsrOffsets = {
    0x4070,  // NOLINT: SyncCounter_SCALAR_INFEED
};

const SyncFlagCsrOffsets kShoalScalarPipelineSyncFlagCsrOffsets = {
    0x4090,  // NOLINT: SyncCounter_SCALAR_PIPELINE
};

const TraceCsrOffsets kShoalAvdatapopTraceCsrOffsets = {
    0x40f8,  // NOLINT: avDataPopOverwriteMode
    0x4100,  // NOLINT: avDataPopEnableTracing
    0x4240,  // NOLINT: avDataPopTrace
};

const TraceCsrOffsets kShoalInfeedTraceCsrOffsets = {
    0x4178,  // NOLINT: infeedOverwriteMode
    0x4180,  // NOLINT: infeedEnableTracing
    0x42c0,  // NOLINT: infeedTrace
};

const TraceCsrOffsets kShoalOutfeedTraceCsrOffsets = {
    0x41b8,  // NOLINT: outfeedOverwriteMode
    0x41c0,  // NOLINT: outfeedEnableTracing
    0x4300,  // NOLINT: outfeedTrace
};

const TraceCsrOffsets kShoalParameterpopTraceCsrOffsets = {
    0x4138,  // NOLINT: parameterPopOverwriteMode
    0x4140,  // NOLINT: parameterPopEnableTracing
    0x4280,  // NOLINT: parameterPopTrace
};

const BreakpointCsrOffsets kShoalMeshbus0BreakpointCsrOffsets = {
    0x22f8,  // NOLINT: meshBus0RunControl
    0x2300,  // NOLINT: meshBus0RunStatus
    0x2308,  // NOLINT: meshBus0BreakPoint
};

const BreakpointCsrOffsets kShoalMeshbus1BreakpointCsrOffsets = {
    0x2340,  // NOLINT: meshBus1RunControl
    0x2348,  // NOLINT: meshBus1RunStatus
    0x2350,  // NOLINT: meshBus1BreakPoint
};

const BreakpointCsrOffsets kShoalMeshbus2BreakpointCsrOffsets = {
    0x2388,  // NOLINT: meshBus2RunControl
    0x2390,  // NOLINT: meshBus2RunStatus
    0x2398,  // NOLINT: meshBus2BreakPoint
};

const BreakpointCsrOffsets kShoalMeshbus3BreakpointCsrOffsets = {
    0x23d0,  // NOLINT: meshBus3RunControl
    0x23d8,  // NOLINT: meshBus3RunStatus
    0x23e0,  // NOLINT: meshBus3BreakPoint
};

const BreakpointCsrOffsets kShoalNarrowtowideBreakpointCsrOffsets = {
    0x21f8,  // NOLINT: narrowToWideRunControl
    0x2200,  // NOLINT: narrowToWideRunStatus
    0x2208,  // NOLINT: narrowToWideBreakPoint
};

const BreakpointCsrOffsets kShoalOpBreakpointCsrOffsets = {
    0x2168,  // NOLINT: opRunControl
    0x2188,  // NOLINT: opRunStatus
    0x2178,  // NOLINT: opBreakPoint
};

const BreakpointCsrOffsets kShoalRingbusconsumer0BreakpointCsrOffsets = {
    0x2238,  // NOLINT: ringBusConsumer0RunControl
    0x2240,  // NOLINT: ringBusConsumer0RunStatus
    0x2248,  // NOLINT: ringBusConsumer0BreakPoint
};

const BreakpointCsrOffsets kShoalRingbusconsumer1BreakpointCsrOffsets = {
    0x2278,  // NOLINT: ringBusConsumer1RunControl
    0x2280,  // NOLINT: ringBusConsumer1RunStatus
    0x2288,  // NOLINT: ringBusConsumer1BreakPoint
};

const BreakpointCsrOffsets kShoalRingbusproducerBreakpointCsrOffsets = {
    0x22b8,  // NOLINT: ringBusProducerRunControl
    0x22c0,  // NOLINT: ringBusProducerRunStatus
    0x22c8,  // NOLINT: ringBusProducerBreakPoint
};

const BreakpointCsrOffsets kShoalWidetonarrowBreakpointCsrOffsets = {
    0x21b8,  // NOLINT: wideToNarrowRunControl
    0x21c0,  // NOLINT: wideToNarrowRunStatus
    0x21c8,  // NOLINT: wideToNarrowBreakPoint
};

const SyncFlagCsrOffsets kShoalAvdataSyncFlagCsrOffsets = {
    0x20c0,  // NOLINT: SyncCounter_AVDATA
};

const SyncFlagCsrOffsets kShoalMeshEastInSyncFlagCsrOffsets = {
    0x20e0,  // NOLINT: SyncCounter_MESH_EAST_IN
};

const SyncFlagCsrOffsets kShoalMeshEastOutSyncFlagCsrOffsets = {
    0x2100,  // NOLINT: SyncCounter_MESH_EAST_OUT
};

const SyncFlagCsrOffsets kShoalMeshNorthInSyncFlagCsrOffsets = {
    0x20d8,  // NOLINT: SyncCounter_MESH_NORTH_IN
};

const SyncFlagCsrOffsets kShoalMeshNorthOutSyncFlagCsrOffsets = {
    0x20f8,  // NOLINT: SyncCounter_MESH_NORTH_OUT
};

const SyncFlagCsrOffsets kShoalMeshSouthInSyncFlagCsrOffsets = {
    0x20e8,  // NOLINT: SyncCounter_MESH_SOUTH_IN
};

const SyncFlagCsrOffsets kShoalMeshSouthOutSyncFlagCsrOffsets = {
    0x2108,  // NOLINT: SyncCounter_MESH_SOUTH_OUT
};

const SyncFlagCsrOffsets kShoalMeshWestInSyncFlagCsrOffsets = {
    0x20f0,  // NOLINT: SyncCounter_MESH_WEST_IN
};

const SyncFlagCsrOffsets kShoalMeshWestOutSyncFlagCsrOffsets = {
    0x2110,  // NOLINT: SyncCounter_MESH_WEST_OUT
};

const SyncFlagCsrOffsets kShoalNarrowToWideSyncFlagCsrOffsets = {
    0x2128,  // NOLINT: SyncCounter_NARROW_TO_WIDE
};

const SyncFlagCsrOffsets kShoalParametersSyncFlagCsrOffsets = {
    0x20c8,  // NOLINT: SyncCounter_PARAMETERS
};

const SyncFlagCsrOffsets kShoalPartialSumsSyncFlagCsrOffsets = {
    0x20d0,  // NOLINT: SyncCounter_PARTIAL_SUMS
};

const SyncFlagCsrOffsets kShoalRingProducerASyncFlagCsrOffsets = {
    0x2148,  // NOLINT: SyncCounter_RING_PRODUCER_A
};

const SyncFlagCsrOffsets kShoalRingProducerBSyncFlagCsrOffsets = {
    0x2150,  // NOLINT: SyncCounter_RING_PRODUCER_B
};

const SyncFlagCsrOffsets kShoalRingReadASyncFlagCsrOffsets = {
    0x2130,  // NOLINT: SyncCounter_RING_READ_A
};

const SyncFlagCsrOffsets kShoalRingReadBSyncFlagCsrOffsets = {
    0x2138,  // NOLINT: SyncCounter_RING_READ_B
};

const SyncFlagCsrOffsets kShoalRingWriteSyncFlagCsrOffsets = {
    0x2140,  // NOLINT: SyncCounter_RING_WRITE
};

const SyncFlagCsrOffsets kShoalWideToNarrowSyncFlagCsrOffsets = {
    0x2118,  // NOLINT: SyncCounter_WIDE_TO_NARROW
};

const SyncFlagCsrOffsets kShoalWideToScalingSyncFlagCsrOffsets = {
    0x2120,  // NOLINT: SyncCounter_WIDE_TO_SCALING
};

const TraceCsrOffsets kShoalDmameshbus0TraceCsrOffsets = {
    0x2318,  // NOLINT: dmaMeshBus0OverwriteMode
    0x2320,  // NOLINT: dmaMeshBus0EnableTracing
    0x2840,  // NOLINT: dmaMeshBus0Trace
};

const TraceCsrOffsets kShoalDmameshbus1TraceCsrOffsets = {
    0x2360,  // NOLINT: dmaMeshBus1OverwriteMode
    0x2368,  // NOLINT: dmaMeshBus1EnableTracing
    0x28c0,  // NOLINT: dmaMeshBus1Trace
};

const TraceCsrOffsets kShoalDmameshbus2TraceCsrOffsets = {
    0x23a8,  // NOLINT: dmaMeshBus2OverwriteMode
    0x23b0,  // NOLINT: dmaMeshBus2EnableTracing
    0x2940,  // NOLINT: dmaMeshBus2Trace
};

const TraceCsrOffsets kShoalDmameshbus3TraceCsrOffsets = {
    0x23f0,  // NOLINT: dmaMeshBus3OverwriteMode
    0x23f8,  // NOLINT: dmaMeshBus3EnableTracing
    0x29c0,  // NOLINT: dmaMeshBus3Trace
};

const TraceCsrOffsets kShoalDmanarrowtowideTraceCsrOffsets = {
    0x2210,  // NOLINT: dmaNarrowToWideOverwriteMode
    0x2218,  // NOLINT: dmaNarrowToWideEnableTracing
    0x2700,  // NOLINT: dmaNarrowToWideTrace
};

const TraceCsrOffsets kShoalDmaringbusconsumer0TraceCsrOffsets = {
    0x2250,  // NOLINT: dmaRingBusConsumer0OverwriteMode
    0x2258,  // NOLINT: dmaRingBusConsumer0EnableTracing
    0x2740,  // NOLINT: dmaRingBusConsumer0Trace
};

const TraceCsrOffsets kShoalDmaringbusconsumer1TraceCsrOffsets = {
    0x2290,  // NOLINT: dmaRingBusConsumer1OverwriteMode
    0x2298,  // NOLINT: dmaRingBusConsumer1EnableTracing
    0x2780,  // NOLINT: dmaRingBusConsumer1Trace
};

const TraceCsrOffsets kShoalDmaringbusproducerTraceCsrOffsets = {
    0x22d0,  // NOLINT: dmaRingBusProducerOverwriteMode
    0x22d8,  // NOLINT: dmaRingBusProducerEnableTracing
    0x27c0,  // NOLINT: dmaRingBusProducerTrace
};

const TraceCsrOffsets kShoalDmawidetonarrowTraceCsrOffsets = {
    0x21d0,  // NOLINT: dmaWideToNarrowOverwriteMode
    0x21d8,  // NOLINT: dmaWideToNarrowEnableTracing
    0x2600,  // NOLINT: dmaWideToNarrowTrace
};

const TraceCsrOffsets kShoalOpTraceCsrOffsets = {
    0x2190,  // NOLINT: OpOverwriteMode
    0x2198,  // NOLINT: OpEnableTracing
    0x2500,  // NOLINT: OpTrace
};

const DebugHibUserCsrOffsets kShoalDebugHibUserCsrOffsets = {
    0x8410,  // NOLINT: instruction_inbound_queue_total_occupancy
    0x8418,  // NOLINT: instruction_inbound_queue_threshold_counter
    0x8420,  // NOLINT: instruction_inbound_queue_insertion_counter
    0x8428,  // NOLINT: instruction_inbound_queue_full_counter
    0x8430,  // NOLINT: input_actv_inbound_queue_total_occupancy
    0x8438,  // NOLINT: input_actv_inbound_queue_threshold_counter
    0x8440,  // NOLINT: input_actv_inbound_queue_insertion_counter
    0x8448,  // NOLINT: input_actv_inbound_queue_full_counter
    0x8450,  // NOLINT: param_inbound_queue_total_occupancy
    0x8458,  // NOLINT: param_inbound_queue_threshold_counter
    0x8460,  // NOLINT: param_inbound_queue_insertion_counter
    0x8468,  // NOLINT: param_inbound_queue_full_counter
    0x8470,  // NOLINT: output_actv_inbound_queue_total_occupancy
    0x8478,  // NOLINT: output_actv_inbound_queue_threshold_counter
    0x8480,  // NOLINT: output_actv_inbound_queue_insertion_counter
    0x8488,  // NOLINT: output_actv_inbound_queue_full_counter
    0x8490,  // NOLINT: status_block_write_inbound_queue_total_occupancy
    0x8498,  // NOLINT: status_block_write_inbound_queue_threshold_counter
    0x84a0,  // NOLINT: status_block_write_inbound_queue_insertion_counter
    0x84a8,  // NOLINT: status_block_write_inbound_queue_full_counter
    0x84b0,  // NOLINT: queue_fetch_inbound_queue_total_occupancy
    0x84b8,  // NOLINT: queue_fetch_inbound_queue_threshold_counter
    0x84c0,  // NOLINT: queue_fetch_inbound_queue_insertion_counter
    0x84c8,  // NOLINT: queue_fetch_inbound_queue_full_counter
    0x84d0,  // NOLINT: instruction_outbound_queue_total_occupancy
    0x84d8,  // NOLINT: instruction_outbound_queue_threshold_counter
    0x84e0,  // NOLINT: instruction_outbound_queue_insertion_counter
    0x84e8,  // NOLINT: instruction_outbound_queue_full_counter
    0x84f0,  // NOLINT: input_actv_outbound_queue_total_occupancy
    0x84f8,  // NOLINT: input_actv_outbound_queue_threshold_counter
    0x8500,  // NOLINT: input_actv_outbound_queue_insertion_counter
    0x8508,  // NOLINT: input_actv_outbound_queue_full_counter
    0x8510,  // NOLINT: param_outbound_queue_total_occupancy
    0x8518,  // NOLINT: param_outbound_queue_threshold_counter
    0x8520,  // NOLINT: param_outbound_queue_insertion_counter
    0x8528,  // NOLINT: param_outbound_queue_full_counter
    0x8530,  // NOLINT: output_actv_outbound_queue_total_occupancy
    0x8538,  // NOLINT: output_actv_outbound_queue_threshold_counter
    0x8540,  // NOLINT: output_actv_outbound_queue_insertion_counter
    0x8548,  // NOLINT: output_actv_outbound_queue_full_counter
    0x8550,  // NOLINT: status_block_write_outbound_queue_total_occupancy
    0x8558,  // NOLINT: status_block_write_outbound_queue_threshold_counter
    0x8560,  // NOLINT: status_block_write_outbound_queue_insertion_counter
    0x8568,  // NOLINT: status_block_write_outbound_queue_full_counter
    0x8570,  // NOLINT: queue_fetch_outbound_queue_total_occupancy
    0x8578,  // NOLINT: queue_fetch_outbound_queue_threshold_counter
    0x8580,  // NOLINT: queue_fetch_outbound_queue_insertion_counter
    0x8588,  // NOLINT: queue_fetch_outbound_queue_full_counter
    0x8590,  // NOLINT: page_table_request_outbound_queue_total_occupancy
    0x8598,  // NOLINT: page_table_request_outbound_queue_threshold_counter
    0x85a0,  // NOLINT: page_table_request_outbound_queue_insertion_counter
    0x85a8,  // NOLINT: page_table_request_outbound_queue_full_counter
    0x85b0,  // NOLINT: read_tracking_fifo_total_occupancy
    0x85b8,  // NOLINT: read_tracking_fifo_threshold_counter
    0x85c0,  // NOLINT: read_tracking_fifo_insertion_counter
    0x85c8,  // NOLINT: read_tracking_fifo_full_counter
    0x85d0,  // NOLINT: write_tracking_fifo_total_occupancy
    0x85d8,  // NOLINT: write_tracking_fifo_threshold_counter
    0x85e0,  // NOLINT: write_tracking_fifo_insertion_counter
    0x85e8,  // NOLINT: write_tracking_fifo_full_counter
    0x85f0,  // NOLINT: read_buffer_total_occupancy
    0x85f8,  // NOLINT: read_buffer_threshold_counter
    0x8600,  // NOLINT: read_buffer_insertion_counter
    0x8608,  // NOLINT: read_buffer_full_counter
    0x8610,  // NOLINT: axi_aw_credit_shim_total_occupancy
    0x8618,  // NOLINT: axi_aw_credit_shim_threshold_counter
    0x8620,  // NOLINT: axi_aw_credit_shim_insertion_counter
    0x8628,  // NOLINT: axi_aw_credit_shim_full_counter
    0x8630,  // NOLINT: axi_ar_credit_shim_total_occupancy
    0x8638,  // NOLINT: axi_ar_credit_shim_threshold_counter
    0x8640,  // NOLINT: axi_ar_credit_shim_insertion_counter
    0x8648,  // NOLINT: axi_ar_credit_shim_full_counter
    0x8650,  // NOLINT: axi_w_credit_shim_total_occupancy
    0x8658,  // NOLINT: axi_w_credit_shim_threshold_counter
    0x8660,  // NOLINT: axi_w_credit_shim_insertion_counter
    0x8668,  // NOLINT: axi_w_credit_shim_full_counter
    0x8670,  // NOLINT: instruction_inbound_queue_empty_cycles_count
    0x8678,  // NOLINT: input_actv_inbound_queue_empty_cycles_count
    0x8680,  // NOLINT: param_inbound_queue_empty_cycles_count
    0x8688,  // NOLINT: output_actv_inbound_queue_empty_cycles_count
    0x8690,  // NOLINT: status_block_write_inbound_queue_empty_cycles_count
    0x8698,  // NOLINT: queue_fetch_inbound_queue_empty_cycles_count
    0x86a0,  // NOLINT: instruction_outbound_queue_empty_cycles_count
    0x86a8,  // NOLINT: input_actv_outbound_queue_empty_cycles_count
    0x86b0,  // NOLINT: param_outbound_queue_empty_cycles_count
    0x86b8,  // NOLINT: output_actv_outbound_queue_empty_cycles_count
    0x86c0,  // NOLINT: status_block_write_outbound_queue_empty_cycles_count
    0x86c8,  // NOLINT: queue_fetch_outbound_queue_empty_cycles_count
    0x86d0,  // NOLINT: page_table_request_outbound_queue_empty_cycles_count
    0x86d8,  // NOLINT: read_tracking_fifo_empty_cycles_count
    0x86e0,  // NOLINT: write_tracking_fifo_empty_cycles_count
    0x86e8,  // NOLINT: read_buffer_empty_cycles_count
    0x86f0,  // NOLINT: read_request_arbiter_instruction_request_cycles
    0x86f8,  // NOLINT: read_request_arbiter_instruction_blocked_cycles
    0x8700,  // NOLINT:
             // read_request_arbiter_instruction_blocked_by_arbitration_cycles
    0x8708,  // NOLINT:
             // read_request_arbiter_instruction_cycles_blocked_over_threshold
    0x8710,  // NOLINT: read_request_arbiter_input_actv_request_cycles
    0x8718,  // NOLINT: read_request_arbiter_input_actv_blocked_cycles
    0x8720,  // NOLINT:
             // read_request_arbiter_input_actv_blocked_by_arbitration_cycles
    0x8728,  // NOLINT:
             // read_request_arbiter_input_actv_cycles_blocked_over_threshold
    0x8730,  // NOLINT: read_request_arbiter_param_request_cycles
    0x8738,  // NOLINT: read_request_arbiter_param_blocked_cycles
    0x8740,  // NOLINT: read_request_arbiter_param_blocked_by_arbitration_cycles
    0x8748,  // NOLINT: read_request_arbiter_param_cycles_blocked_over_threshold
    0x8750,  // NOLINT: read_request_arbiter_queue_fetch_request_cycles
    0x8758,  // NOLINT: read_request_arbiter_queue_fetch_blocked_cycles
    0x8760,  // NOLINT:
             // read_request_arbiter_queue_fetch_blocked_by_arbitration_cycles
    0x8768,  // NOLINT:
             // read_request_arbiter_queue_fetch_cycles_blocked_over_threshold
    0x8770,  // NOLINT: read_request_arbiter_page_table_request_request_cycles
    0x8778,  // NOLINT: read_request_arbiter_page_table_request_blocked_cycles
    0x8780,  // NOLINT:
             // read_request_arbiter_page_table_request_blocked_by_arbitration_cycles
    0x8788,  // NOLINT:
             // read_request_arbiter_page_table_request_cycles_blocked_over_threshold
    0x8790,  // NOLINT: write_request_arbiter_output_actv_request_cycles
    0x8798,  // NOLINT: write_request_arbiter_output_actv_blocked_cycles
    0x87a0,  // NOLINT:
             // write_request_arbiter_output_actv_blocked_by_arbitration_cycles
    0x87a8,  // NOLINT:
             // write_request_arbiter_output_actv_cycles_blocked_over_threshold
    0x87b0,  // NOLINT: write_request_arbiter_status_block_write_request_cycles
    0x87b8,  // NOLINT: write_request_arbiter_status_block_write_blocked_cycles
    0x87c0,  // NOLINT:
             // write_request_arbiter_status_block_write_blocked_by_arbitration_cycles
    0x87c8,  // NOLINT:
             // write_request_arbiter_status_block_write_cycles_blocked_over_threshold
    0x87d0,  // NOLINT: address_translation_arbiter_instruction_request_cycles
    0x87d8,  // NOLINT: address_translation_arbiter_instruction_blocked_cycles
    0x87e0,  // NOLINT:
             // address_translation_arbiter_instruction_blocked_by_arbitration_cycles
    0x87e8,  // NOLINT:
             // address_translation_arbiter_instruction_cycles_blocked_over_threshold
    0x87f0,  // NOLINT: address_translation_arbiter_input_actv_request_cycles
    0x87f8,  // NOLINT: address_translation_arbiter_input_actv_blocked_cycles
    0x8800,  // NOLINT:
             // address_translation_arbiter_input_actv_blocked_by_arbitration_cycles
    0x8808,  // NOLINT:
             // address_translation_arbiter_input_actv_cycles_blocked_over_threshold
    0x8810,  // NOLINT: address_translation_arbiter_param_request_cycles
    0x8818,  // NOLINT: address_translation_arbiter_param_blocked_cycles
    0x8820,  // NOLINT:
             // address_translation_arbiter_param_blocked_by_arbitration_cycles
    0x8828,  // NOLINT:
             // address_translation_arbiter_param_cycles_blocked_over_threshold
    0x8830,  // NOLINT:
             // address_translation_arbiter_status_block_write_request_cycles
    0x8838,  // NOLINT:
             // address_translation_arbiter_status_block_write_blocked_cycles
    0x8840,  // NOLINT:
             // address_translation_arbiter_status_block_write_blocked_by_arbitration_cycles
    0x8848,  // NOLINT:
             // address_translation_arbiter_status_block_write_cycles_blocked_over_threshold
    0x8850,  // NOLINT: address_translation_arbiter_output_actv_request_cycles
    0x8858,  // NOLINT: address_translation_arbiter_output_actv_blocked_cycles
    0x8860,  // NOLINT:
             // address_translation_arbiter_output_actv_blocked_by_arbitration_cycles
    0x8868,  // NOLINT:
             // address_translation_arbiter_output_actv_cycles_blocked_over_threshold
    0x8870,  // NOLINT: address_translation_arbiter_queue_fetch_request_cycles
    0x8878,  // NOLINT: address_translation_arbiter_queue_fetch_blocked_cycles
    0x8880,  // NOLINT:
             // address_translation_arbiter_queue_fetch_blocked_by_arbitration_cycles
    0x8888,  // NOLINT:
             // address_translation_arbiter_queue_fetch_cycles_blocked_over_threshold
    0x8890,  // NOLINT: issued_interrupt_count
    0x8898,  // NOLINT: data_read_16byte_count
    0x88a0,  // NOLINT: waiting_for_tag_cycles
    0x88a8,  // NOLINT: waiting_for_axi_cycles
    0x88b0,  // NOLINT: simple_translations
    0x88b8,  // NOLINT: instruction_credits_per_cycle_sum
    0x88c0,  // NOLINT: input_actv_credits_per_cycle_sum
    0x88c8,  // NOLINT: param_credits_per_cycle_sum
    0x88d0,  // NOLINT: output_actv_credits_per_cycle_sum
    0x88d8,  // NOLINT: status_block_write_credits_per_cycle_sum
    0x88e0,  // NOLINT: queue_fetch_credits_per_cycle_sum
    0x88e8,  // NOLINT: page_table_request_credits_per_cycle_sum
    0x8000,  // NOLINT: output_actv_queue_control
    0x8008,  // NOLINT: output_actv_queue_status
    0x8010,  // NOLINT: output_actv_queue_descriptor_size
    0x8018,  // NOLINT: output_actv_queue_minimum_size
    0x8020,  // NOLINT: output_actv_queue_maximum_size
    0x8028,  // NOLINT: output_actv_queue_base
    0x8030,  // NOLINT: output_actv_queue_status_block_base
    0x8038,  // NOLINT: output_actv_queue_size
    0x8040,  // NOLINT: output_actv_queue_tail
    0x8048,  // NOLINT: output_actv_queue_fetched_head
    0x8050,  // NOLINT: output_actv_queue_completed_head
    0x8058,  // NOLINT: output_actv_queue_int_control
    0x8060,  // NOLINT: output_actv_queue_int_status
    0x8068,  // NOLINT: instruction_queue_control
    0x8070,  // NOLINT: instruction_queue_status
    0x8078,  // NOLINT: instruction_queue_descriptor_size
    0x8080,  // NOLINT: instruction_queue_minimum_size
    0x8088,  // NOLINT: instruction_queue_maximum_size
    0x8090,  // NOLINT: instruction_queue_base
    0x8098,  // NOLINT: instruction_queue_status_block_base
    0x80a0,  // NOLINT: instruction_queue_size
    0x80a8,  // NOLINT: instruction_queue_tail
    0x80b0,  // NOLINT: instruction_queue_fetched_head
    0x80b8,  // NOLINT: instruction_queue_completed_head
    0x80c0,  // NOLINT: instruction_queue_int_control
    0x80c8,  // NOLINT: instruction_queue_int_status
    0x80d0,  // NOLINT: input_actv_queue_control
    0x80d8,  // NOLINT: input_actv_queue_status
    0x80e0,  // NOLINT: input_actv_queue_descriptor_size
    0x80e8,  // NOLINT: input_actv_queue_minimum_size
    0x80f0,  // NOLINT: input_actv_queue_maximum_size
    0x80f8,  // NOLINT: input_actv_queue_base
    0x8100,  // NOLINT: input_actv_queue_status_block_base
    0x8108,  // NOLINT: input_actv_queue_size
    0x8110,  // NOLINT: input_actv_queue_tail
    0x8118,  // NOLINT: input_actv_queue_fetched_head
    0x8120,  // NOLINT: input_actv_queue_completed_head
    0x8128,  // NOLINT: input_actv_queue_int_control
    0x8130,  // NOLINT: input_actv_queue_int_status
    0x8138,  // NOLINT: param_queue_control
    0x8140,  // NOLINT: param_queue_status
    0x8148,  // NOLINT: param_queue_descriptor_size
    0x8150,  // NOLINT: param_queue_minimum_size
    0x8158,  // NOLINT: param_queue_maximum_size
    0x8160,  // NOLINT: param_queue_base
    0x8168,  // NOLINT: param_queue_status_block_base
    0x8170,  // NOLINT: param_queue_size
    0x8178,  // NOLINT: param_queue_tail
    0x8180,  // NOLINT: param_queue_fetched_head
    0x8188,  // NOLINT: param_queue_completed_head
    0x8190,  // NOLINT: param_queue_int_control
    0x8198,  // NOLINT: param_queue_int_status
    0x81a0,  // NOLINT: sc_host_int_control
    0x81a8,  // NOLINT: sc_host_int_status
    0x81b0,  // NOLINT: top_level_int_control
    0x81b8,  // NOLINT: top_level_int_status
    0x81c0,  // NOLINT: fatal_err_int_control
    0x81c8,  // NOLINT: fatal_err_int_status
    0x81d0,  // NOLINT: sc_host_int_count
    0x81d8,  // NOLINT: dma_pause
    0x81e0,  // NOLINT: dma_paused
    0x81e8,  // NOLINT: status_block_update
    0x8c20,  // NOLINT: hib_error_status
    0x8c28,  // NOLINT: hib_error_mask
    0x8c08,  // NOLINT: hib_first_error_status
    0x8c10,  // NOLINT: hib_first_error_timestamp
    0x8c30,  // NOLINT: hib_inject_error
    0x81f0,  // NOLINT: read_request_arbiter
    0x81f8,  // NOLINT: write_request_arbiter
    0x8200,  // NOLINT: address_translation_arbiter
    0x8208,  // NOLINT: sender_queue_threshold
    0x8210,  // NOLINT: page_fault_address
    0x8218,  // NOLINT: instruction_credits
    0x8220,  // NOLINT: input_actv_credits
    0x8228,  // NOLINT: param_credits
    0x8230,  // NOLINT: output_actv_credits
    0x8238,  // NOLINT: pause_state
    0x8240,  // NOLINT: snapshot
    0x8248,  // NOLINT: idle_assert
    0x8250,  // NOLINT: wire_int_pending_bit_array
    0x8260,  // NOLINT: tileconfig0
    0x8268,  // NOLINT: tileconfig1
};

const DebugScalarCoreCsrOffsets kShoalDebugScalarCoreCsrOffsets = {
    0x4000,  // NOLINT: topology
    0x4008,  // NOLINT: scMemoryCapacity
    0x4010,  // NOLINT: tileMemoryCapacity
    0x4018,  // NOLINT: scalarCoreRunControl
    0x4020,  // NOLINT: scalarCoreBreakPoint
    0x4028,  // NOLINT: currentPc
    0x4038,  // NOLINT: executeControl
    0x4040,  // NOLINT: scMemoryAccess
    0x4048,  // NOLINT: scMemoryData
    0x4050,  // NOLINT: SyncCounter_AVDATA_POP
    0x4058,  // NOLINT: SyncCounter_PARAMETER_POP
    0x4060,  // NOLINT: SyncCounter_AVDATA_INFEED
    0x4068,  // NOLINT: SyncCounter_PARAMETER_INFEED
    0x4070,  // NOLINT: SyncCounter_SCALAR_INFEED
    0x4078,  // NOLINT: SyncCounter_PRODUCER_A
    0x4080,  // NOLINT: SyncCounter_PRODUCER_B
    0x4088,  // NOLINT: SyncCounter_RING_OUTFEED
    0x40e0,  // NOLINT: avDataPopRunControl
    0x40e8,  // NOLINT: avDataPopBreakPoint
    0x40f0,  // NOLINT: avDataPopRunStatus
    0x40f8,  // NOLINT: avDataPopOverwriteMode
    0x4100,  // NOLINT: avDataPopEnableTracing
    0x4108,  // NOLINT: avDataPopStartCycle
    0x4110,  // NOLINT: avDataPopEndCycle
    0x4118,  // NOLINT: avDataPopProgramCounter
    0x4120,  // NOLINT: parameterPopRunControl
    0x4128,  // NOLINT: parameterPopBreakPoint
    0x4130,  // NOLINT: parameterPopRunStatus
    0x4138,  // NOLINT: parameterPopOverwriteMode
    0x4140,  // NOLINT: parameterPopEnableTracing
    0x4148,  // NOLINT: parameterPopStartCycle
    0x4150,  // NOLINT: parameterPopEndCycle
    0x4158,  // NOLINT: parameterPopProgramCounter
    0x4160,  // NOLINT: infeedRunControl
    0x4168,  // NOLINT: infeedRunStatus
    0x4170,  // NOLINT: infeedBreakPoint
    0x4178,  // NOLINT: infeedOverwriteMode
    0x4180,  // NOLINT: infeedEnableTracing
    0x4188,  // NOLINT: infeedStartCycle
    0x4190,  // NOLINT: infeedEndCycle
    0x4198,  // NOLINT: infeedProgramCounter
    0x41a0,  // NOLINT: outfeedRunControl
    0x41a8,  // NOLINT: outfeedRunStatus
    0x41b0,  // NOLINT: outfeedBreakPoint
    0x41b8,  // NOLINT: outfeedOverwriteMode
    0x41c0,  // NOLINT: outfeedEnableTracing
    0x41c8,  // NOLINT: outfeedStartCycle
    0x41d0,  // NOLINT: outfeedEndCycle
    0x41d8,  // NOLINT: outfeedProgramCounter
    0x41e0,  // NOLINT: scalarCoreRunStatus
    0x41e8,  // NOLINT: Error_ScalarCore
    0x41f0,  // NOLINT: Error_Mask_ScalarCore
    0x41f8,  // NOLINT: Error_Force_ScalarCore
    0x4200,  // NOLINT: Error_Timestamp_ScalarCore
    0x4208,  // NOLINT: Error_Info_ScalarCore
    0x4210,  // NOLINT: Timeout
    0x4220,  // NOLINT: avDataPopTtuStateRegFile
    0x4240,  // NOLINT: avDataPopTrace
    0x4260,  // NOLINT: parameterPopTtuStateRegFile
    0x4280,  // NOLINT: parameterPopTrace
    0x42a0,  // NOLINT: infeedTtuStateRegFile
    0x42e0,  // NOLINT: outfeedTtuStateRegFile
};

const DebugTileCsrOffsets kShoalDebugTileCsrOffsets = {
    0x2000,  // NOLINT: tileid
    0x2008,  // NOLINT: scratchpad
    0x2010,  // NOLINT: memoryAccess
    0x2018,  // NOLINT: memoryData
    0x2020,  // NOLINT: deepSleep
    0x20c0,  // NOLINT: SyncCounter_AVDATA
    0x20c8,  // NOLINT: SyncCounter_PARAMETERS
    0x20d0,  // NOLINT: SyncCounter_PARTIAL_SUMS
    0x20d8,  // NOLINT: SyncCounter_MESH_NORTH_IN
    0x20e0,  // NOLINT: SyncCounter_MESH_EAST_IN
    0x20e8,  // NOLINT: SyncCounter_MESH_SOUTH_IN
    0x20f0,  // NOLINT: SyncCounter_MESH_WEST_IN
    0x20f8,  // NOLINT: SyncCounter_MESH_NORTH_OUT
    0x2100,  // NOLINT: SyncCounter_MESH_EAST_OUT
    0x2108,  // NOLINT: SyncCounter_MESH_SOUTH_OUT
    0x2110,  // NOLINT: SyncCounter_MESH_WEST_OUT
    0x2118,  // NOLINT: SyncCounter_WIDE_TO_NARROW
    0x2120,  // NOLINT: SyncCounter_WIDE_TO_SCALING
    0x2128,  // NOLINT: SyncCounter_NARROW_TO_WIDE
    0x2130,  // NOLINT: SyncCounter_RING_READ_A
    0x2138,  // NOLINT: SyncCounter_RING_READ_B
    0x2140,  // NOLINT: SyncCounter_RING_WRITE
    0x2148,  // NOLINT: SyncCounter_RING_PRODUCER_A
    0x2150,  // NOLINT: SyncCounter_RING_PRODUCER_B
    0x2168,  // NOLINT: opRunControl
    0x2170,  // NOLINT: PowerSaveData
    0x2178,  // NOLINT: opBreakPoint
    0x2180,  // NOLINT: StallCounter
    0x2188,  // NOLINT: opRunStatus
    0x2190,  // NOLINT: OpOverwriteMode
    0x2198,  // NOLINT: OpEnableTracing
    0x21a0,  // NOLINT: OpStartCycle
    0x21a8,  // NOLINT: OpEndCycle
    0x21b0,  // NOLINT: OpProgramCounter
    0x21b8,  // NOLINT: wideToNarrowRunControl
    0x21c0,  // NOLINT: wideToNarrowRunStatus
    0x21c8,  // NOLINT: wideToNarrowBreakPoint
    0x21d0,  // NOLINT: dmaWideToNarrowOverwriteMode
    0x21d8,  // NOLINT: dmaWideToNarrowEnableTracing
    0x21e0,  // NOLINT: dmaWideToNarrowStartCycle
    0x21e8,  // NOLINT: dmaWideToNarrowEndCycle
    0x21f0,  // NOLINT: dmaWideToNarrowProgramCounter
    0x21f8,  // NOLINT: narrowToWideRunControl
    0x2200,  // NOLINT: narrowToWideRunStatus
    0x2208,  // NOLINT: narrowToWideBreakPoint
    0x2210,  // NOLINT: dmaNarrowToWideOverwriteMode
    0x2218,  // NOLINT: dmaNarrowToWideEnableTracing
    0x2220,  // NOLINT: dmaNarrowToWideStartCycle
    0x2228,  // NOLINT: dmaNarrowToWideEndCycle
    0x2230,  // NOLINT: dmaNarrowToWideProgramCounter
    0x2238,  // NOLINT: ringBusConsumer0RunControl
    0x2240,  // NOLINT: ringBusConsumer0RunStatus
    0x2248,  // NOLINT: ringBusConsumer0BreakPoint
    0x2250,  // NOLINT: dmaRingBusConsumer0OverwriteMode
    0x2258,  // NOLINT: dmaRingBusConsumer0EnableTracing
    0x2260,  // NOLINT: dmaRingBusConsumer0StartCycle
    0x2268,  // NOLINT: dmaRingBusConsumer0EndCycle
    0x2270,  // NOLINT: dmaRingBusConsumer0ProgramCounter
    0x2278,  // NOLINT: ringBusConsumer1RunControl
    0x2280,  // NOLINT: ringBusConsumer1RunStatus
    0x2288,  // NOLINT: ringBusConsumer1BreakPoint
    0x2290,  // NOLINT: dmaRingBusConsumer1OverwriteMode
    0x2298,  // NOLINT: dmaRingBusConsumer1EnableTracing
    0x22a0,  // NOLINT: dmaRingBusConsumer1StartCycle
    0x22a8,  // NOLINT: dmaRingBusConsumer1EndCycle
    0x22b0,  // NOLINT: dmaRingBusConsumer1ProgramCounter
    0x22b8,  // NOLINT: ringBusProducerRunControl
    0x22c0,  // NOLINT: ringBusProducerRunStatus
    0x22c8,  // NOLINT: ringBusProducerBreakPoint
    0x22d0,  // NOLINT: dmaRingBusProducerOverwriteMode
    0x22d8,  // NOLINT: dmaRingBusProducerEnableTracing
    0x22e0,  // NOLINT: dmaRingBusProducerStartCycle
    0x22e8,  // NOLINT: dmaRingBusProducerEndCycle
    0x22f0,  // NOLINT: dmaRingBusProducerProgramCounter
    0x22f8,  // NOLINT: meshBus0RunControl
    0x2300,  // NOLINT: meshBus0RunStatus
    0x2308,  // NOLINT: meshBus0BreakPoint
    0x2318,  // NOLINT: dmaMeshBus0OverwriteMode
    0x2320,  // NOLINT: dmaMeshBus0EnableTracing
    0x2328,  // NOLINT: dmaMeshBus0StartCycle
    0x2330,  // NOLINT: dmaMeshBus0EndCycle
    0x2338,  // NOLINT: dmaMeshBus0ProgramCounter
    0x2340,  // NOLINT: meshBus1RunControl
    0x2348,  // NOLINT: meshBus1RunStatus
    0x2350,  // NOLINT: meshBus1BreakPoint
    0x2360,  // NOLINT: dmaMeshBus1OverwriteMode
    0x2368,  // NOLINT: dmaMeshBus1EnableTracing
    0x2370,  // NOLINT: dmaMeshBus1StartCycle
    0x2378,  // NOLINT: dmaMeshBus1EndCycle
    0x2380,  // NOLINT: dmaMeshBus1ProgramCounter
    0x2388,  // NOLINT: meshBus2RunControl
    0x2390,  // NOLINT: meshBus2RunStatus
    0x2398,  // NOLINT: meshBus2BreakPoint
    0x23a8,  // NOLINT: dmaMeshBus2OverwriteMode
    0x23b0,  // NOLINT: dmaMeshBus2EnableTracing
    0x23b8,  // NOLINT: dmaMeshBus2StartCycle
    0x23c0,  // NOLINT: dmaMeshBus2EndCycle
    0x23c8,  // NOLINT: dmaMeshBus2ProgramCounter
    0x23d0,  // NOLINT: meshBus3RunControl
    0x23d8,  // NOLINT: meshBus3RunStatus
    0x23e0,  // NOLINT: meshBus3BreakPoint
    0x23f0,  // NOLINT: dmaMeshBus3OverwriteMode
    0x23f8,  // NOLINT: dmaMeshBus3EnableTracing
    0x2400,  // NOLINT: dmaMeshBus3StartCycle
    0x2408,  // NOLINT: dmaMeshBus3EndCycle
    0x2410,  // NOLINT: dmaMeshBus3ProgramCounter
    0x2460,  // NOLINT: Error_Tile
    0x2468,  // NOLINT: Error_Mask_Tile
    0x2470,  // NOLINT: Error_Force_Tile
    0x2478,  // NOLINT: Error_Timestamp_Tile
    0x2480,  // NOLINT: Error_Info_Tile
    0x2488,  // NOLINT: Timeout
    0x24c0,  // NOLINT: opTtuStateRegFile
    0x2500,  // NOLINT: OpTrace
    0x2580,  // NOLINT: wideToNarrowTtuStateRegFile
    0x2600,  // NOLINT: dmaWideToNarrowTrace
    0x2680,  // NOLINT: narrowToWideTtuStateRegFile
    0x2700,  // NOLINT: dmaNarrowToWideTrace
    0x2720,  // NOLINT: ringBusConsumer0TtuStateRegFile
    0x2740,  // NOLINT: dmaRingBusConsumer0Trace
    0x2760,  // NOLINT: ringBusConsumer1TtuStateRegFile
    0x2780,  // NOLINT: dmaRingBusConsumer1Trace
    0x27a0,  // NOLINT: ringBusProducerTtuStateRegFile
    0x27c0,  // NOLINT: dmaRingBusProducerTrace
    0x2800,  // NOLINT: meshBus0TtuStateRegFile
    0x2840,  // NOLINT: dmaMeshBus0Trace
    0x2880,  // NOLINT: meshBus1TtuStateRegFile
    0x28c0,  // NOLINT: dmaMeshBus1Trace
    0x2900,  // NOLINT: meshBus2TtuStateRegFile
    0x2940,  // NOLINT: dmaMeshBus2Trace
    0x2980,  // NOLINT: meshBus3TtuStateRegFile
    0x29c0,  // NOLINT: dmaMeshBus3Trace
};

const HibKernelCsrOffsets kShoalHibKernelCsrOffsets = {
    0x6000,   // NOLINT: page_table_size
    0x6008,   // NOLINT: extended_table
    0x6080,   // NOLINT: dma_pause
    0x60a8,   // NOLINT: page_table_init
    0x60b0,   // NOLINT: msix_table_init
    0x10000,  // NOLINT: page_table
};

const HibUserCsrOffsets kShoalHibUserCsrOffsets = {
    0x81b0,  // NOLINT: top_level_int_control
    0x81b8,  // NOLINT: top_level_int_status
    0x81d0,  // NOLINT: sc_host_int_count
    0x81d8,  // NOLINT: dma_pause
    0x81e0,  // NOLINT: dma_paused
    0x81e8,  // NOLINT: status_block_update
    0x8c20,  // NOLINT: hib_error_status
    0x8c28,  // NOLINT: hib_error_mask
    0x8c08,  // NOLINT: hib_first_error_status
    0x8c10,  // NOLINT: hib_first_error_timestamp
    0x8c30,  // NOLINT: hib_inject_error
    0x8280,  // NOLINT: dma_burst_limiter
};

const QueueCsrOffsets kShoalInstructionQueueCsrOffsets = {
    0x8068,  // NOLINT: instruction_queue_control
    0x8070,  // NOLINT: instruction_queue_status
    0x8078,  // NOLINT: instruction_queue_descriptor_size
    0x8090,  // NOLINT: instruction_queue_base
    0x8098,  // NOLINT: instruction_queue_status_block_base
    0x80a0,  // NOLINT: instruction_queue_size
    0x80a8,  // NOLINT: instruction_queue_tail
    0x80b0,  // NOLINT: instruction_queue_fetched_head
    0x80b8,  // NOLINT: instruction_queue_completed_head
    0x80c0,  // NOLINT: instruction_queue_int_control
    0x80c8,  // NOLINT: instruction_queue_int_status
    0x8080,  // NOLINT: instruction_queue_minimum_size
    0x8088,  // NOLINT: instruction_queue_maximum_size
    0x6048,  // NOLINT: instruction_queue_int_vector
};

const MemoryCsrOffsets kShoalMemoryMemoryCsrOffsets = {
    0x2010,  // NOLINT: memoryAccess
    0x2018,  // NOLINT: memoryData
};

const ScalarCoreCsrOffsets kShoalScalarCoreCsrOffsets = {
    0x4018,  // NOLINT: scalarCoreRunControl
    0x40e0,  // NOLINT: avDataPopRunControl
    0x4120,  // NOLINT: parameterPopRunControl
    0x4160,  // NOLINT: infeedRunControl
    0x41a0,  // NOLINT: outfeedRunControl
};

const MemoryCsrOffsets kShoalScmemoryMemoryCsrOffsets = {
    0x4040,  // NOLINT: scMemoryAccess
    0x4048,  // NOLINT: scMemoryData
};

const TileConfigCsrOffsets kShoalTileConfigCsrOffsets = {
    0x8260,  // NOLINT: tileconfig0
    0x8268,  // NOLINT: tileconfig1
};

const TileCsrOffsets kShoalTileCsrOffsets = {
    0x168,  // NOLINT: opRunControl
    0x418,  // NOLINT: narrowToNarrowRunControl
    0x1b8,  // NOLINT: wideToNarrowRunControl
    0x1f8,  // NOLINT: narrowToWideRunControl
    0x238,  // NOLINT: ringBusConsumer0RunControl
    0x278,  // NOLINT: ringBusConsumer1RunControl
    0x2b8,  // NOLINT: ringBusProducerRunControl
    0x2f8,  // NOLINT: meshBus0RunControl
    0x340,  // NOLINT: meshBus1RunControl
    0x388,  // NOLINT: meshBus2RunControl
    0x3d0,  // NOLINT: meshBus3RunControl
    0x20,   // NOLINT: deepSleep
};

const WireCsrOffsets kShoalWireCsrOffsets = {
    0x8250,  // NOLINT: wire_int_pending_bit_array
    0x8258,  // NOLINT: wire_int_mask_array
};

const AonResetCsrOffsets kShoalAonResetCsrOffsets = {
    0x20000,                  // NOLINT: resetReg
    0x20008,                  // NOLINT: clockEnableReg
    static_cast<uint64>(-1),  // UNUSED, logicShutdownReg
    static_cast<uint64>(-1),  // UNUSED, logicShutdownPreReg
    static_cast<uint64>(-1),  // UNUSED, logicShutdownAllReg
    static_cast<uint64>(-1),  // UNUSED, memoryShutdownReg
    static_cast<uint64>(-1),  // UNUSED, memoryShutdownAckReg
    static_cast<uint64>(-1),  // UNUSED, tileMemoryRetnReg
    static_cast<uint64>(-1),  // UNUSED, clampEnableReg
    0x20010,                  // NOLINT: forceQuiesceReg
};

}  // namespace config
}  // namespace driver
}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_DRIVER_CONFIG_SHOAL_SHOAL_CSR_OFFSETS_H_
