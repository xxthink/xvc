/******************************************************************************
* Copyright (C) 2017, Divideon.
*
* Redistribution and use in source and binary form, with or without
* modifications is permitted only under the terms and conditions set forward
* in the xvc License Agreement. For commercial redistribution and use, you are
* required to send a signed copy of the xvc License Agreement to Divideon.
*
* Redistribution and use in source and binary form is permitted free of charge
* for non-commercial purposes. See definition of non-commercial in the xvc
* License Agreement.
*
* All redistribution of source code must retain this copyright notice
* unmodified.
*
* The xvc License Agreement is available at https://xvc.io/license/.
******************************************************************************/

#include "xvc_dec_lib/segment_header_reader.h"

#include "xvc_common_lib/restrictions.h"

namespace xvc {

Decoder::State SegmentHeaderReader::Read(SegmentHeader* segment_header,
                                         BitReader *bit_reader,
                                         SegmentNum segment_counter) {
  segment_header->codec_identifier = bit_reader->ReadBits(24);
  if (segment_header->codec_identifier != constants::kXvcCodecIdentifier) {
    return Decoder::State::kNoSegmentHeader;
  }
  segment_header->major_version = bit_reader->ReadBits(16);
  if (segment_header->major_version > constants::kXvcMajorVersion) {
    return Decoder::State::kDecoderVersionTooLow;
  }
  segment_header->minor_version = bit_reader->ReadBits(16);
  segment_header->SetWidth(bit_reader->ReadBits(16));
  segment_header->SetHeight(bit_reader->ReadBits(16));
  segment_header->chroma_format = ChromaFormat(bit_reader->ReadBits(4));
  segment_header->internal_bitdepth = bit_reader->ReadBits(4) + 8;
  if (segment_header->internal_bitdepth >
      static_cast<int>(8 * sizeof(Sample))) {
    return Decoder::State::kBitstreamBitdepthTooHigh;
  }
  segment_header->bitstream_ticks = bit_reader->ReadBits(24);
  segment_header->max_sub_gop_length = bit_reader->ReadBits(8);

  segment_header->color_matrix = ColorMatrix(bit_reader->ReadBits(3));
  segment_header->open_gop = bit_reader->ReadBit() != 0;
  segment_header->num_ref_pics = bit_reader->ReadBits(4);
  static_assert(constants::kMaxBinarySplitDepth < (1 << 2),
                "max binary split depth signaling");
  segment_header->max_binary_split_depth = bit_reader->ReadBits(2);
  segment_header->checksum_mode = Checksum::Mode(bit_reader->ReadBits(1));
  segment_header->adaptive_qp = bit_reader->ReadBits(2);
  segment_header->chroma_qp_offset_table = bit_reader->ReadBits(2);
  int chroma_qp_offsets = bit_reader->ReadBit();
  if (chroma_qp_offsets) {
    int d = constants::kChromaOffsetBits;
    segment_header->chroma_qp_offset_u =
      bit_reader->ReadBits(d) - (1 << (d - 1));
    segment_header->chroma_qp_offset_v =
      bit_reader->ReadBits(d) - (1 << (d - 1));
  }
  segment_header->deblock = bit_reader->ReadBits(2);
  if (segment_header->deblock == 3) {
    int d = constants::kDeblockOffsetBits;
    segment_header->beta_offset = bit_reader->ReadBits(d) - (1 << (d - 1));
    segment_header->tc_offset = bit_reader->ReadBits(d) - (1 << (d - 1));
  }

  auto &restr = segment_header->restrictions;
  restr = Restrictions();

  // Note! Override the value of the restriction flags only if the flag is
  // set to true in the bitstream.

  int intra_restrictions = bit_reader->ReadBit();
  if (intra_restrictions) {
    if (bit_reader->ReadBit()) {
      restr.disable_intra_ref_padding = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_intra_ref_sample_filter = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_intra_dc_post_filter = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_intra_ver_hor_post_filter = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_intra_planar = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_intra_mpm_prediction = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_intra_chroma_predictor = true;
    }
  }

  int inter_restrictions = bit_reader->ReadBit();
  if (inter_restrictions) {
    if (bit_reader->ReadBit()) {
      restr.disable_inter_mvp = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_scaling_mvp = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_tmvp_mvp = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_tmvp_merge = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_tmvp_ref_list_derivation = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_merge_candidates = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_merge_mode = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_merge_bipred = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_skip_mode = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_chroma_subpel = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_mvd_greater_than_flags = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_inter_bipred = true;
    }
  }

  int transform_restrictions = bit_reader->ReadBit();
  if (transform_restrictions) {
    if (bit_reader->ReadBit()) {
      restr.disable_transform_adaptive_scan_order = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_residual_greater_than_flags = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_residual_greater2 = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_last_position = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_root_cbf = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_cbf = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_subblock_csbf = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_sign_hiding = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_transform_adaptive_exp_golomb = true;
    }
  }

  int cabac_restrictions = bit_reader->ReadBit();
  if (cabac_restrictions) {
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_ctx_update = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_split_flag_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_skip_flag_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_inter_dir_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_subblock_csbf_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_coeff_sig_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_coeff_greater1_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_coeff_greater2_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_coeff_last_pos_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_init_per_pic_type = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_cabac_init_per_qp = true;
    }
  }

  int deblock_restrictions = bit_reader->ReadBit();
  if (deblock_restrictions) {
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_strong_filter = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_weak_filter = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_chroma_filter = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_boundary_strength_zero = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_boundary_strength_one = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_initial_sample_decision = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_weak_sample_decision = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_two_samples_weak_filter = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_deblock_depending_on_qp = true;
    }
  }

  int high_level_restrictions = bit_reader->ReadBit();
  if (high_level_restrictions) {
    if (bit_reader->ReadBit()) {
      restr.disable_high_level_default_checksum_method = true;
    }
  }

  int ext_restrictions = bit_reader->ReadBit();
  if (ext_restrictions) {
    if (bit_reader->ReadBit()) {
      restr.disable_ext_sink = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_implicit_last_ctu = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_tmvp_full_resolution = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_tmvp_exclude_intra_from_ref_list = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_ref_list_l0_trim = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_implicit_partition_type = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_cabac_alt_split_flag_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_cabac_alt_inter_dir_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_cabac_alt_last_pos_ctx = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_two_cu_trees = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_transform_size_64 = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_intra_unrestricted_predictor = true;
    }
    if (bit_reader->ReadBit()) {
      restr.disable_ext_deblock_subblock_size_4 = true;
    }
  }

  Restrictions::GetRW() = restr;

  segment_header->soc = segment_counter;
  return Decoder::State::kSegmentHeaderDecoded;
}

}   // namespace xvc
