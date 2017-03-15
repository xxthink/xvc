/******************************************************************************
* Copyright (C) 2017, Divideon. All rights reserved.
* No part of this code may be reproduced in any form
* without the written permission of the copyright holder.
******************************************************************************/

#include "xvc_common_lib/quantize.h"

#include <cstdlib>
#include <cmath>

#include "xvc_common_lib/utils.h"

namespace xvc {

const uint8_t QP::kChromaScale_[QP::kChromaQpMax_ + 1] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 29, 30, 31, 32, 33, 33, 34, 34, 35, 35, 36,
  36, 37, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

const int QP::kFwdQuantScales_[kNumScalingListRem_] = {
  26214, 23302, 20560, 18396, 16384, 14564
};

const int QP::kInvQuantScales_[kNumScalingListRem_] = {
  40, 45, 51, 57, 64, 72
};

QP::QP(int qp, ChromaFormat chroma_format, PicturePredictionType pic_type,
       int bitdepth, int sub_gop_length, int temporal_id, int chroma_offset)
  : qp_raw_({ qp,
            GetChromaQP(qp + chroma_offset, chroma_format, bitdepth),
            GetChromaQP(qp + chroma_offset, chroma_format, bitdepth) }),
  qp_bitdepth_({ qp_raw_[0] + kNumScalingListRem_ * (bitdepth - 8),
                 qp_raw_[1] + kNumScalingListRem_ * (bitdepth - 8),
                 qp_raw_[2] + kNumScalingListRem_ * (bitdepth - 8) }),
  distortion_weight_({ 1.0,
                     GetChromaDistWeight(qp + chroma_offset, chroma_format),
                     GetChromaDistWeight(qp + chroma_offset, chroma_format) }) {
  lambda_ = CalculateLambda(qp, pic_type, sub_gop_length, temporal_id);
  lambda_sqrt_ = std::sqrt(lambda_);
}

int QP::GetChromaQP(int qp, ChromaFormat chroma_format, int bitdepth) {
  int chroma_qp = util::Clip3(qp, 0, kChromaQpMax_);
  if (chroma_format == ChromaFormat::k420) {
    chroma_qp = kChromaScale_[chroma_qp];
  }
  return chroma_qp;
}

double QP::GetChromaDistWeight(int qp, ChromaFormat chroma_format) {
  int chroma_qp = util::Clip3(qp, 0, kChromaQpMax_);
  int comp_qp_offset = 0;
  if (chroma_format == ChromaFormat::k420) {
    comp_qp_offset = kChromaScale_[chroma_qp] - chroma_qp;
  }
  return pow(2.0, -comp_qp_offset / 3.0);
}

double QP::CalculateLambda(int qp, PicturePredictionType pic_type,
                           int sub_gop_length, int temporal_id) {
  int qp_temp = qp - 12;
  double bframe_factor =
    1.0 - util::Clip3(0.05 * (sub_gop_length - 1), 0.0, 0.5);
  double qp_factor =
    pic_type == PicturePredictionType::kIntra ? 0.57*bframe_factor : 1.0;
  double gop_factor =
    temporal_id == 0 ? 1 : util::Clip3(qp_temp / 6.0, 2.0, 4.0);
  return qp_factor * gop_factor * std::pow(2.0, qp_temp / 3.0);
}

int Quantize::Forward(YuvComponent comp, const QP &qp, int shift, int offset,
                      int width, int height, const Coeff *in,
                      ptrdiff_t in_stride, Coeff *out, ptrdiff_t out_stride) {
  int scale = qp.GetFwdScale(comp);
  int num_non_zero = 0;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int coeff = in[x];
      int sign = coeff < 0 ? -1 : 1;
      int level = std::abs(coeff);

      level = ((level * scale) + offset) >> shift;
      level *= sign;
      level = util::Clip3(level, constants::kInt16Min, constants::kInt16Max);

      out[x] = static_cast<Coeff>(level);
      if (level) {
        num_non_zero++;
      }
    }
    in += in_stride;
    out += out_stride;
  }
  return num_non_zero;
}

void Quantize::Inverse(YuvComponent comp, const QP &qp, int shift, int width,
                       int height, const Coeff *in, ptrdiff_t in_stride,
                       Coeff *out, ptrdiff_t out_stride) {
  int scale = qp.GetInvScale(comp);
  int offset = (1 << (shift - 1));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int coeff = ((in[x] * scale) + offset) >> shift;
      out[x] = util::Clip3(coeff, constants::kInt16Min, constants::kInt16Max);
    }
    in += in_stride;
    out += out_stride;
  }
}

}   // namespace xvc