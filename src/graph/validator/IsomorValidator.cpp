/* Copyright (c) 2021 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License.
 */
#include "graph/validator/IsomorValidator.h"

#include "graph/planner/plan/Query.h"
#include "graph/util/ExpressionUtils.h"
#include "graph/util/ValidateUtil.h"
#include "graph/visitor/DeducePropsVisitor.h"
namespace nebula {
namespace graph {
Status IsomorValidator::validateImpl() {
  auto *fSentence = static_cast<IsomorSentence *>(sentence_);
  fetchCtx_ = getContext<IsomorContext>();
  NG_RETURN_IF_ERROR(validateTag(fSentence->graphs()));
  return Status::OK();
}
// Check validity of tags specified in sentence
Status IsomorValidator::validateTag(const NameLabelList *nameLabels) {
  auto graphs = nameLabels->labels();

  LOG(INFO) << "query space name: " << *graphs[0];
  LOG(INFO) << "data space name: " << *graphs[1];
  // The first graph is query graph and the second graph is the data graph
  auto querySpaceRet = qctx_->schemaMng()->toGraphSpaceID(*graphs[0]);
  NG_RETURN_IF_ERROR(querySpaceRet);
  auto dataSpaceRet = qctx_->schemaMng()->toGraphSpaceID(*graphs[1]);
  NG_RETURN_IF_ERROR(dataSpaceRet);

  fetchCtx_->querySpace = std::move(querySpaceRet).value();
  fetchCtx_->dataSpace = std::move(dataSpaceRet).value();
  return Status::OK();
}
}  // namespace graph
}  // namespace nebula
