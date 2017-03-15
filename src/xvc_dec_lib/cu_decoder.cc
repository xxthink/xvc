/******************************************************************************
* Copyright (C) 2017, Divideon. All rights reserved.
* No part of this code may be reproduced in any form
* without the written permission of the copyright holder.
******************************************************************************/

#include "xvc_dec_lib/cu_decoder.h"

#include <cassert>

#include "xvc_common_lib/restrictions.h"
#include "xvc_common_lib/utils.h"

namespace xvc {

CuDecoder::CuDecoder(const QP &pic_qp, YuvPicture *decoded_pic,
                     PictureData *pic_data)
  : min_pel_(0),
  max_pel_((1 << decoded_pic->GetBitdepth()) - 1),
  pic_qp_(pic_qp),
  decoded_pic_(*decoded_pic),
  pic_data_(*pic_data),
  inter_pred_(decoded_pic->GetBitdepth()),
  intra_pred_(decoded_pic->GetBitdepth()),
  inv_transform_(decoded_pic->GetBitdepth()),
  quantize_(),
  cu_reader_(pic_data, intra_pred_),
  temp_pred_(kBufferStride_, constants::kMaxBlockSize),
  temp_resi_(kBufferStride_, constants::kMaxBlockSize),
  temp_coeff_(kBufferStride_, constants::kMaxBlockSize) {
}

void CuDecoder::DecodeCtu(int rsaddr, SyntaxReader *reader) {
  CodingUnit *cu = pic_data_.GetCtu(rsaddr);

  cu_reader_.ReadCu(cu, reader);
#if HM_STRICT
  if (reader->ReadEndOfSlice()) {
    assert(0);
  }
#endif

  pic_data_.ClearMarkCuInPic(cu);
  DecompressCu(cu);
}

void CuDecoder::DecompressCu(CodingUnit *cu) {
  if (cu->IsSplit()) {
    for (CodingUnit *sub_cu : cu->GetSubCu()) {
      if (sub_cu) {
        DecompressCu(sub_cu);
      }
    }
  } else {
    pic_data_.MarkUsedInPic(cu);
    cu->SetQp(pic_qp_);
    for (int c = 0; c < pic_data_.GetNumComponents(); c++) {
      const YuvComponent comp = YuvComponent(c);
      DecompressComponent(cu, comp, cu->GetQp());
    }
  }
}

void CuDecoder::DecompressComponent(CodingUnit *cu, YuvComponent comp,
                                    const QP &qp) {
  int cu_x = cu->GetPosX(comp);
  int cu_y = cu->GetPosY(comp);
  int width = cu->GetWidth(comp);
  int height = cu->GetHeight(comp);
  bool cbf = cu->GetCbf(comp);
  SampleBuffer dec_buffer = decoded_pic_.GetSampleBuffer(comp, cu_x, cu_y);
  SampleBuffer &pred_buffer = cbf ? temp_pred_ : dec_buffer;

  // Predict
  if (cu->IsIntra()) {
    Sample *dec = decoded_pic_.GetSamplePtr(comp, cu_x, cu_y);
    ptrdiff_t dec_stride = decoded_pic_.GetStride(comp);
    IntraMode intra_mode = cu->GetIntraMode(comp);
    intra_pred_.Predict(intra_mode, *cu, comp, dec, dec_stride,
                        pred_buffer.GetDataPtr(), pred_buffer.GetStride());
  } else {
    inter_pred_.CalculateMV(cu);
    inter_pred_.MotionCompensation(*cu, comp, pred_buffer.GetDataPtr(),
                                   pred_buffer.GetStride());
  }
  if (!cbf) {
    return;
  }

  // Dequant
  Coeff *cu_coeff_buf = cu->GetCoeff(comp);
  ptrdiff_t cu_coeff_stride = cu->GetCoeffStride();
  const int transform_shift = constants::kMaxTrDynamicRange
    - decoded_pic_.GetBitdepth() - util::SizeToLog2(width);
  const int shift_iquant = constants::kIQuantShift
    - constants::kQuantShift - transform_shift;
  quantize_.Inverse(comp, qp, shift_iquant, width, height, cu_coeff_buf,
                    cu_coeff_stride, temp_coeff_.GetDataPtr(),
                    temp_coeff_.GetStride());

  // Inverse transform
  inv_transform_.Transform(width, temp_coeff_.GetDataPtr(),
                           temp_coeff_.GetStride(), temp_resi_.GetDataPtr(),
                           temp_resi_.GetStride());

  // Reconstruct
  dec_buffer.AddClip(width, height, temp_pred_, temp_resi_, min_pel_, max_pel_);
}

}   // namespace xvc