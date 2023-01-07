/* Copyright (c) 2021 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License.
 */

#include "graph/planner/ngql/IsomorPlanner.h"

#include "graph/planner/plan/Query.h"
#include "graph/util/PlannerUtil.h"

namespace nebula {
namespace graph {

static const char kDefaultProp[] = "label";
static const char koutputCol[] = "count";

StatusOr<SubPlan> IsomorPlanner::transform(AstContext* astCtx) {
  isoCtx_ = static_cast<IsomorContext*>(astCtx);
  auto qctx = isoCtx_->qctx;
  auto dSpaceId = isoCtx_->dataSpace;
  auto qSpaceId = isoCtx_->querySpace;

  auto dScanVertics = createScanVerticesPlan(qctx, dSpaceId, nullptr);
  auto qScanVertics = createScanVerticesPlan(qctx, qSpaceId, dScanVertics);

  auto dScanEdges = createScanEdgesPlan(qctx, dSpaceId, qScanVertics);
  auto qScanEdges = createScanEdgesPlan(qctx, qSpaceId, dScanEdges);

  auto isomor = Isomor::make(qctx,
                             qScanEdges,
                             kDefaultProp,
                             dScanVertics->outputVar(),
                             qScanVertics->outputVar(),
                             dScanEdges->outputVar(),
                             qScanEdges->outputVar());

  isomor->setColNames({koutputCol});

  SubPlan subPlan;
  subPlan.root = isomor;
  subPlan.tail = dScanVertics;
  return subPlan;
}

PlanNode* IsomorPlanner::createScanVerticesPlan(QueryContext* qctx,
                                                GraphSpaceID spaceId,
                                                PlanNode* input) {
  // create plan node
  auto tags = qctx->schemaMng()->getAllVerTagSchema(spaceId);
  DCHECK(tags.ok());

  std::string tagName;
  TagID tagId;
  for (auto tag : tags.value()) {
    tagId = tag.first;
    auto tagNameRet = qctx->schemaMng()->toTagName(spaceId, tagId);
    DCHECK(tagNameRet.ok());
    tagName = std::move(tagNameRet.value());
    // LOG(INFO) << "spaceId: " << spaceId << " tag id: " << tagId << " tag name: " << tagName;
  }

  auto vProps = std::make_unique<std::vector<storage::cpp2::VertexProp>>();
  std::vector<std::string> colNames{kVid};

  storage::cpp2::VertexProp vProp;
  std::vector<std::string> props{kDefaultProp};
  vProp.tag_ref() = tagId;
  vProp.props_ref() = std::move(props);
  vProps->emplace_back(std::move(vProp));
  colNames.emplace_back(tagName + "." + std::string(kDefaultProp));

  auto* scanVertices =
      ScanVertices::make(qctx, input, spaceId, std::move(vProps), nullptr, false, {});
  scanVertices->setColNames(std::move(colNames));

  return scanVertices;
}

PlanNode* IsomorPlanner::createScanEdgesPlan(QueryContext* qctx,
                                             GraphSpaceID spaceId,
                                             PlanNode* input) {
  // create plan node
  auto edges = qctx->schemaMng()->getAllVerEdgeSchema(spaceId);
  DCHECK(edges.ok());

  std::string edgeName;
  EdgeType edgeType;
  for (auto edge : edges.value()) {
    edgeType = edge.first;
    auto edgeNameRet = qctx->schemaMng()->toEdgeName(spaceId, edgeType);
    DCHECK(edgeNameRet.ok());
    edgeName = std::move(edgeNameRet.value());
    LOG(INFO) << "edge type: " << edgeType << "edge name: " << edgeName;
  }

  auto eProps = std::make_unique<std::vector<storage::cpp2::EdgeProp>>();
  std::vector<std::string> colNames;

  storage::cpp2::EdgeProp eProp;
  std::vector<std::string> props = {kSrc, kType, kRank, kDst};

  // add column name
  for (auto prop : props) {
    colNames.emplace_back(edgeName + "." + prop);
  }

  eProp.type_ref() = edgeType;
  eProp.props_ref() = std::move(props);
  eProps->emplace_back(std::move(eProp));

  auto* scanEdges = ScanEdges::make(qctx, input, spaceId, std::move(eProps), nullptr, false);
  scanEdges->setColNames(std::move(colNames));

  return scanEdges;
}

}  // namespace graph
}  // namespace nebula
