// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "sw/device/lib/base/mmio.h"
#include "sw/device/lib/dif/dif_csrng.h"
#include "sw/device/lib/dif/dif_entropy_src.h"
#include "sw/device/lib/testing/test_framework/check.h"

#include "edn_regs.h"  // Generated
#include "hw/top_earlgrey/sw/autogen/top_earlgrey.h"

static void setup_entropy_src(void) {
  dif_entropy_src_t entropy_src;
  CHECK_DIF_OK(dif_entropy_src_init(
      mmio_region_from_addr(TOP_EARLGREY_ENTROPY_SRC_BASE_ADDR), &entropy_src));

  // Disable entropy for test purpose, as it has been turned on by ROM
  CHECK_DIF_OK(dif_entropy_src_set_enabled(&entropy_src, kDifToggleDisabled));

  const dif_entropy_src_config_t config = {
      .fips_enable = true,
      .route_to_firmware = false,
      .single_bit_mode = kDifEntropySrcSingleBitModeDisabled,
  };
  CHECK_DIF_OK(
      dif_entropy_src_configure(&entropy_src, config, kDifToggleEnabled));
}

static void setup_csrng(void) {
  dif_csrng_t csrng;
  CHECK_DIF_OK(dif_csrng_init(
      mmio_region_from_addr(TOP_EARLGREY_CSRNG_BASE_ADDR), &csrng));
  CHECK_DIF_OK(dif_csrng_configure(&csrng));
}

static void setup_edn(void) {
  // Temporary solution to configure/enable the EDN and CSRNG to allow OTBN to
  // run before a DIF is available,
  // https://github.com/lowRISC/opentitan/issues/6082
  uint32_t reg =
      bitfield_field32_write(0, EDN_CTRL_EDN_ENABLE_FIELD, kMultiBitBool4True);
  reg = bitfield_field32_write(reg, EDN_CTRL_BOOT_REQ_MODE_FIELD,
                               kMultiBitBool4True);
  reg = bitfield_field32_write(reg, EDN_CTRL_AUTO_REQ_MODE_FIELD,
                               kMultiBitBool4False);
  reg = bitfield_field32_write(reg, EDN_CTRL_CMD_FIFO_RST_FIELD,
                               kMultiBitBool4False);
  mmio_region_write32(mmio_region_from_addr(TOP_EARLGREY_EDN0_BASE_ADDR),
                      EDN_CTRL_REG_OFFSET, reg);
  mmio_region_write32(mmio_region_from_addr(TOP_EARLGREY_EDN1_BASE_ADDR),
                      EDN_CTRL_REG_OFFSET, reg);
}

void entropy_testutils_boot_mode_init(void) {
  setup_entropy_src();
  setup_csrng();
  setup_edn();
}

void entropy_testutils_wait_for_state(const dif_entropy_src_t *entropy_src,
                                      dif_entropy_src_main_fsm_t state) {
  dif_entropy_src_main_fsm_t cur_state;

  do {
    CHECK_DIF_OK(dif_entropy_src_get_main_fsm_state(entropy_src, &cur_state));
  } while (cur_state != state);
}
