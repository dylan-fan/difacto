/**
 *  Copyright (c) 2015 by Contributors
 */
#include <gtest/gtest.h>
#include "./utils.h"
#include "loss/fm_loss.h"
#include "data/localizer.h"
#include "loss/bin_class_metric.h"

using namespace difacto;

TEST(FMLoss, NoV) {
  SArray<real_t> weight(47149);
  for (size_t i = 0; i < weight.size(); ++i) {
    weight[i] = i / 5e4;
  }

  dmlc::data::RowBlockContainer<unsigned> rowblk;
  std::vector<feaid_t> uidx;
  load_data(&rowblk, &uidx);
  SArray<real_t> w(uidx.size());
  for (size_t i = 0; i < uidx.size(); ++i) {
    w[i] = weight[uidx[i]];
  }

  KWArgs args = {{"V_dim", "0"}};
  FMLoss loss; loss.Init(args);
  auto data = rowblk.GetBlock();
  SArray<real_t> pred(data.size);
  loss.Predict(data, w, {}, {}, &pred);

  BinClassMetric eval(data.label, pred.data(), data.size);

  // Progress prog;
  EXPECT_LT(fabs(eval.LogitObjv() - 147.4672), 1e-3);

  SArray<real_t> grad(w.size());
  loss.CalcGrad(data, w, {}, {}, pred, &grad);
  EXPECT_LT(fabs(norm2(grad) - 90.5817), 1e-3);
}

TEST(FMLoss, HasV) {
  int V_dim = 5;
  int n = 47149;
  std::vector<real_t> weight(n*(V_dim+1));
  for (int i = 0; i < n; ++i) {
    weight[i*(V_dim+1)] = i / 5e4;
    for (int j = 1; j <= V_dim; ++j) {
      weight[i*(V_dim+1)+j] = i * j / 5e5;
    }
  }

  dmlc::data::RowBlockContainer<unsigned> rowblk;
  std::vector<feaid_t> uidx;
  load_data(&rowblk, &uidx);

  SArray<int> w_pos(uidx.size());
  SArray<int> V_pos(uidx.size());
  int p = 0;
  SArray<real_t> w(uidx.size()*(V_dim+1));
  for (size_t i = 0; i < uidx.size(); ++i) {
    for (int j = 0; j < V_dim+1; ++j) {
      w[i*(V_dim+1)+j] = weight[uidx[i]*(V_dim+1)+j];
    }
    w_pos[i] = p;
    V_pos[i] = p+1;
    p += V_dim + 1;
  }

  KWArgs args = {{"V_dim", std::to_string(V_dim)}};
  FMLoss loss; loss.Init(args);
  auto data = rowblk.GetBlock();
  SArray<real_t> pred(data.size);
  loss.Predict(data, w, w_pos, V_pos, &pred);

  // Progress prog;
  BinClassMetric eval(data.label, pred.data(), data.size);
  EXPECT_LT(fabs(eval.LogitObjv() - 330.628), 1e-3);

  SArray<real_t> grad(w.size());
  loss.CalcGrad(data, w, w_pos, V_pos, pred, &grad);
  EXPECT_LT(fabs(norm2(grad) - 1.2378e+03), 1e-1);
}
