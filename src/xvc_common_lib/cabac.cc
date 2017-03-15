/******************************************************************************
* Copyright (C) 2017, Divideon. All rights reserved.
* No part of this code may be reproduced in any form
* without the written permission of the copyright holder.
******************************************************************************/

#include "xvc_common_lib/cabac.h"

#include "xvc_common_lib/restrictions.h"
#include "xvc_common_lib/utils.h"

namespace xvc {

const uint8_t Cabac::kRenormTable_[32] = {
  6, 5, 4, 4, 3, 3, 3, 3,
  2, 2, 2, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
};

const std::array<std::array<const uint8_t, 4>, 1 <<
  ContextModel::CONTEXT_STATE_BITS>
  Cabac::kRangeTable_ = { {
{ 128, 176, 208, 240 },
{ 128, 167, 197, 227 },
{ 128, 158, 187, 216 },
{ 123, 150, 178, 205 },
{ 116, 142, 169, 195 },
{ 111, 135, 160, 185 },
{ 105, 128, 152, 175 },
{ 100, 122, 144, 166 },
{ 95, 116, 137, 158 },
{ 90, 110, 130, 150 },
{ 85, 104, 123, 142 },
{ 81, 99, 117, 135 },
{ 77, 94, 111, 128 },
{ 73, 89, 105, 122 },
{ 69, 85, 100, 116 },
{ 66, 80, 95, 110 },
{ 62, 76, 90, 104 },
{ 59, 72, 86, 99 },
{ 56, 69, 81, 94 },
{ 53, 65, 77, 89 },
{ 51, 62, 73, 85 },
{ 48, 59, 69, 80 },
{ 46, 56, 66, 76 },
{ 43, 53, 63, 72 },
{ 41, 50, 59, 69 },
{ 39, 48, 56, 65 },
{ 37, 45, 54, 62 },
{ 35, 43, 51, 59 },
{ 33, 41, 48, 56 },
{ 32, 39, 46, 53 },
{ 30, 37, 43, 50 },
{ 29, 35, 41, 48 },
{ 27, 33, 39, 45 },
{ 26, 31, 37, 43 },
{ 24, 30, 35, 41 },
{ 23, 28, 33, 39 },
{ 22, 27, 32, 37 },
{ 21, 26, 30, 35 },
{ 20, 24, 29, 33 },
{ 19, 23, 27, 31 },
{ 18, 22, 26, 30 },
{ 17, 21, 25, 28 },
{ 16, 20, 23, 27 },
{ 15, 19, 22, 25 },
{ 14, 18, 21, 24 },
{ 14, 17, 20, 23 },
{ 13, 16, 19, 22 },
{ 12, 15, 18, 21 },
{ 12, 14, 17, 20 },
{ 11, 14, 16, 19 },
{ 11, 13, 15, 18 },
{ 10, 12, 15, 17 },
{ 10, 12, 14, 16 },
{ 9, 11, 13, 15 },
{ 9, 11, 12, 14 },
{ 8, 10, 12, 14 },
{ 8, 9, 11, 13 },
{ 7, 9, 11, 12 },
{ 7, 9, 10, 12 },
{ 7, 8, 10, 11 },
{ 6, 8, 9, 11 },
{ 6, 7, 9, 10 },
{ 6, 7, 8, 9 },
{ 2, 2, 2, 2 }
} };

static const uint8_t kNotUsed = 0;

static const uint8_t
kInitCuTransquantBypassFlag[3][CabacContexts::kNumTquantBypassFlagCtx] = {
  { 154 },
  { 154 },
  { 154 },
};

static const uint8_t
kInitSplitFlag[3][CabacContexts::kNumSplitFlagCtx] = {
  { 107,  139,  126, },
  { 107,  139,  126, },
  { 139,  141,  157, },
};

static const uint8_t
kInitSkipFlag[3][CabacContexts::kNumSkipFlagCtx] = {
  { 197,  185,  201, },
  { 197,  185,  201, },
  { kNotUsed,  kNotUsed,  kNotUsed, },
};

static const uint8_t
kInitMergeFlag[3][CabacContexts::kNumMergeFlagCtx] = {
  { 154, },
  { 110, },
  { kNotUsed, },
};

static const uint8_t
kInitMergeIdx[3][CabacContexts::kNumMergeIdxCtx] = {
  { 137, },
  { 122, },
  { kNotUsed, },
};

static const uint8_t
kInitPartSize[3][CabacContexts::kNumPartSizeCtx] = {
  { 154,  139,  154, 154 },
  { 154,  139,  154, 154 },
  { 184,  kNotUsed,  kNotUsed, kNotUsed },
};

static const uint8_t
kInitPredMode[3][CabacContexts::kNumPredModeCtx] = {
  { 134, },
  { 149, },
  { kNotUsed, },
};

static const uint8_t
kInitIntraPredMode[3][CabacContexts::kNumIntraPredCtx] = {
  { 183, 152, },
  { 154, 152, },
  { 184, 63, },
};

static const uint8_t
kInitInterDir[3][CabacContexts::kNumInterDirCtx] = {
  { 95,   79,   63,   31,  31, },
  { 95,   79,   63,   31,  31, },
  { kNotUsed,  kNotUsed,  kNotUsed,  kNotUsed, kNotUsed, },
};

static const uint8_t
kInitMvd[3][CabacContexts::kNumMvdCtx] = {
  { 169,  198, },
  { 140,  198, },
  { kNotUsed,  kNotUsed, },
};

static const uint8_t
kInitRefIdx[3][CabacContexts::kNumRefIdxCtx] = {
  { 153,  153 },
  { 153,  153 },
  { kNotUsed,  kNotUsed },
};

static const uint8_t
kInitDqp[3][CabacContexts::kNumDeltaQpCtx] = {
  { 154,  154,  154, },
  { 154,  154,  154, },
  { 154,  154,  154, },
};

static const uint8_t
kInitCuCbf[3][CabacContexts::kNumCuCbfCtx] = {
  { 111,  149 },
  { 111,  149 },
  { 141,   94 },
};

static const uint8_t
kInitCuRootCbf[3][CabacContexts::kNumCuRootCbfCtx] = {
  { 79, },
  { 79, },
  { kNotUsed, },
};

static const uint8_t
kInitLastPos[3][CabacContexts::kNumCoeffLastPosCtx] = {
  { 125, 110, 124, 110, 95, 94, 125, 111, 111, 79, 125, 126, 111, 111, 79,
  108, 123, 93 },
  { 125, 110, 94, 110, 95, 79, 125, 111, 110, 78, 110, 111, 111, 95, 94,
  108, 123, 108 },
  { 110, 110, 124, 125, 140, 153, 125, 127, 140, 109, 111, 143, 127, 111, 79,
  108, 123, 63 },
};

static const uint8_t
kInitSubblockCsbf[3][CabacContexts::kNumSubblockCsbfCtx] = {
  { 121, 140, 61, 154, },
  { 121, 140, 61, 154, },
  { 91, 171, 134, 141, },
};

static const uint8_t
kInitCoeffSig[3][CabacContexts::kNumCoeffSigCtx] = {
  { 170, 154, 139, 153, 139, 123, 123, 63, 124, 166, 183, 140, 136, 153, 154,
  166, 183, 140, 136, 153, 154, 166, 183, 140, 136, 153, 154, 170, 153, 138,
  138, 122, 121, 122, 121, 167, 151, 183, 140, 151, 183, 140, },
  { 155, 154, 139, 153, 139, 123, 123, 63, 153, 166, 183, 140, 136, 153, 154,
  166, 183, 140, 136, 153, 154, 166, 183, 140, 136, 153, 154, 170, 153, 123,
  123, 107, 121, 107, 121, 167, 151, 183, 140, 151, 183, 140, },
  { 111, 111, 125, 110, 110, 94, 124, 108, 124, 107, 125, 141, 179, 153, 125,
  107, 125, 141, 179, 153, 125, 107, 125, 141, 179, 153, 125, 140, 139, 182,
  182, 152, 136, 152, 136, 153, 136, 139, 111, 136, 139, 111, },
};

static const uint8_t
kInitCoeffGreater1[3][CabacContexts::kNumCoeffGreater1Ctx] = {
  { 154, 196, 167, 167, 154, 152, 167, 182, 182, 134, 149, 136, 153, 121, 136,
  122, 169, 208, 166, 167, 154, 152, 167, 182, },
  { 154, 196, 196, 167, 154, 152, 167, 182, 182, 134, 149, 136, 153, 121, 136,
  137, 169, 194, 166, 167, 154, 167, 137, 182, },
  { 140, 92, 137, 138, 140, 152, 138, 139, 153, 74, 149, 92, 139, 107, 122,
  152, 140, 179, 166, 182, 140, 227, 122, 197, },
};

static const uint8_t
kInitCoeffGreater2[3][CabacContexts::kNumCoeffGreater2Ctx] = {
  { 107, 167, 91, 107, 107, 167, },
  { 107, 167, 91, 122, 107, 167, },
  { 138, 153, 136, 167, 152, 152, },
};

static const uint8_t
kInitMvpIdx[3][CabacContexts::kNumMvpIdxCtx] = {
  { 168 },
  { 168 },
  { kNotUsed },
};

static const uint8_t
kInitSaoMergeFlag[3][CabacContexts::kNumSaoMergeFlagCtx] = {
  { 153, },
  { 153, },
  { 153, },
};

static const uint8_t
InitSaoTypeIdx[3][CabacContexts::kNumSaoTypeIdxCtx] = {
  { 160, },
  { 185, },
  { 200, },
};

static const uint8_t
kInitTransSubdivFlag[3][CabacContexts::kNumTransSubdivFlagCtx] = {
  { 224,  167,  122, },
  { 124,  138,   94, },
  { 153,  138,  138, },
};

static const uint8_t
kInitTransformskipFlag[3][2 * CabacContexts::kNumTransformskipFlagCtx] = {
  { 139,  139 },
  { 139,  139 },
  { 139,  139 },
};

template <size_t N>
inline static void Init(int qp, int slice_type,
                           std::array<ContextModel, N> *ctx,
                           const uint8_t init_state[3][N]) {
  for (size_t i = 0; i < N; i++) {
    (*ctx)[i].Init(qp, init_state[slice_type][i]);
  }
}

template <size_t N, size_t M>
inline static void Init(int qp, int slice_type,
                           std::array<ContextModel, N> *ctx_luma,
                           std::array<ContextModel, M> *ctx_chroma,
                           const uint8_t init_state[3][N + M]) {
  for (size_t i = 0; i < N; i++) {
    (*ctx_luma)[i].Init(qp, init_state[slice_type][i]);
  }
  for (size_t i = 0; i < M; i++) {
    (*ctx_chroma)[i].Init(qp, init_state[slice_type][N + i]);
  }
}

void CabacContexts::ResetStates(const QP &qp, PicturePredictionType pic_type) {
  int q = qp.GetQpRaw(YuvComponent::kY);
  int s = static_cast<int>(pic_type);
  Init(q, s, &cu_cbf_luma, &cu_cbf_chroma, kInitCuCbf);
  Init(q, s, &cu_part_size, kInitPartSize);
  Init(q, s, &cu_pred_mode, kInitPredMode);
  Init(q, s, &cu_root_cbf, kInitCuRootCbf);
  Init(q, s, &cu_skip_flag, kInitSkipFlag);
  Init(q, s, &cu_split_flag, kInitSplitFlag);
  Init(q, s, &inter_dir, kInitInterDir);
  Init(q, s, &inter_merge_flag, kInitMergeFlag);
  Init(q, s, &inter_merge_idx, kInitMergeIdx);
  Init(q, s, &inter_mvd, kInitMvd);
  Init(q, s, &inter_mvp_idx, kInitMvpIdx);
  Init(q, s, &inter_ref_idx, kInitRefIdx);
  Init(q, s, &intra_pred_luma, &intra_pred_chroma, kInitIntraPredMode);
  Init(q, s, &subblock_csbf_luma, &subblock_csbf_chroma, kInitSubblockCsbf);
  Init(q, s, &coeff_sig_luma, &coeff_sig_chroma, kInitCoeffSig);
  Init(q, s, &coeff_greater1_luma, &coeff_greater1_chroma, kInitCoeffGreater1);
  Init(q, s, &coeff_greater2_luma, &coeff_greater2_chroma, kInitCoeffGreater2);
  Init(q, s, &coeff_last_pos_x_luma, &coeff_last_pos_x_chroma, kInitLastPos);
  Init(q, s, &coeff_last_pos_y_luma, &coeff_last_pos_y_chroma, kInitLastPos);
}

ContextModel &CabacContexts::GetSubblockCsbfCtx(YuvComponent comp,
                                                const uint8_t *sublock_csbf,
                                                int posx, int posy, int width,
                                                int height,
                                                int *pattern_sig_ctx) {
  int below = false;
  int right = false;
  ContextModel *ctx_base =
    util::IsLuma(comp) ? &subblock_csbf_luma[0] : &subblock_csbf_chroma[0];
  if (posx < width - 1) {
    right = sublock_csbf[posy * width + posx + 1] != 0;
  }
  if (posy < height - 1) {
    below = sublock_csbf[(posy + 1) * width + posx] != 0;
  }
  *pattern_sig_ctx = right + (below << 1);
  if (Restrictions::Get().disable_cabac_subblock_csbf_ctx) {
    return ctx_base[0];
  }
  return ctx_base[right | below];
}

ContextModel &CabacContexts::GetCoeffSigCtx(YuvComponent comp,
                                            int pattern_sig_ctx,
                                            ScanOrder scan_order, int posx,
                                            int posy, int log2size) {
  const uint8_t kCtxIndexMap[16] = {
    0, 1, 4, 5, 2, 3, 4, 5, 6, 6, 8, 8, 7, 7, 8, 8
  };
  ContextModel *ctx_base =
    util::IsLuma(comp) ? &coeff_sig_luma[0] : &coeff_sig_chroma[0];
  if ((!posx && !posy) || Restrictions::Get().disable_cabac_coeff_sig_ctx) {
    return ctx_base[0];
  }
  if (log2size == 2) {
    return ctx_base[kCtxIndexMap[4 * posy + posx]];
  }
  int offset = log2size == 3 ? (scan_order == ScanOrder::kDiagonal ? 9 : 15) :
    (util::IsLuma(comp) ? 21 : 12);
  int pos_x_in_subset = posx - ((posx >> 2) << 2);
  int pos_y_in_subset = posy - ((posy >> 2) << 2);
  int cnt = 0;
  if (pattern_sig_ctx == 0) {
    cnt = pos_x_in_subset + pos_y_in_subset <= 2 ?
      (pos_x_in_subset + pos_y_in_subset == 0 ? 2 : 1) : 0;
  } else if (pattern_sig_ctx == 1) {
    cnt = pos_y_in_subset <= 1 ? (pos_y_in_subset == 0 ? 2 : 1) : 0;
  } else if (pattern_sig_ctx == 2) {
    cnt = pos_x_in_subset <= 1 ? (pos_x_in_subset == 0 ? 2 : 1) : 0;
  } else {
    cnt = 2;
  }
  int comp_offset =
    util::IsLuma(comp) && ((posx >> 2) + (posy >> 2)) > 0 ? 3 : 0;
  return ctx_base[comp_offset + offset + cnt];
}

ContextModel &CabacContexts::GetCoeffGreaterThan1Ctx(YuvComponent comp,
                                                     int ctx_set, int c1) {
  ContextModel *ctx_base =
    util::IsLuma(comp) ? &coeff_greater1_luma[0] : &coeff_greater1_chroma[0];
  if (Restrictions::Get().disable_cabac_coeff_greater1_ctx) {
    return ctx_base[0];
  }
  return ctx_base[4 * ctx_set + c1];
}

ContextModel &CabacContexts::GetCoeffGreaterThan2Ctx(YuvComponent comp,
                                                     int ctx_set) {
  ContextModel *ctx_base =
    util::IsLuma(comp) ? &coeff_greater2_luma[0] : &coeff_greater2_chroma[0];
  if (Restrictions::Get().disable_cabac_coeff_greater2_ctx) {
    return ctx_base[0];
  }
  return ctx_base[ctx_set];
}

ContextModel &CabacContexts::GetCoeffLastPosCtx(YuvComponent comp, int width,
                                                int height, int pos,
                                                bool is_pos_x) {
  if (util::IsLuma(comp)) {
    auto &ctx_base = is_pos_x ? coeff_last_pos_x_luma : coeff_last_pos_y_luma;
    if (Restrictions::Get().disable_cabac_coeff_last_pos_ctx) {
      return ctx_base[0];
    }
    int offset =
      util::SizeLog2Bits(width) * 3 + ((util::SizeLog2Bits(width) + 1) >> 2);
    int shift = (util::SizeLog2Bits(width) + 3) >> 2;
    return ctx_base[offset + (pos >> shift)];
  } else {
    auto &ctx_base =
      is_pos_x ? coeff_last_pos_x_chroma : coeff_last_pos_y_chroma;
    if (Restrictions::Get().disable_cabac_coeff_last_pos_ctx) {
      return ctx_base[0];
    }
    int offset = 0;
    int shift = util::SizeLog2Bits(width);
    return ctx_base[offset + (pos >> shift)];
  }
}

}   // namespace xvc