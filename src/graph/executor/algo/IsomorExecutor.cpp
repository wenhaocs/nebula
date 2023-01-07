// Copyright (c) 2020 vesoft inc. All rights reserved.
//
// This source code is licensed under Apache 2.0 License.
#include "graph/executor/algo/IsomorExecutor.h"

#include "graph/planner/plan/Algo.h"

namespace nebula {
namespace graph {

static const char kDefaultProp[] = "label";  //

StatusOr<std::unique_ptr<Graph>> IsomorExecutor::generateGraph(PropIter* vIter,
                                                               PropIter* eIter,
                                                               std::string labelName) {
  uint32_t vCount = vIter->size();
  uint32_t lCount = vIter->size();
  uint32_t eCount = eIter->size();

  LOG(INFO) << "vertex count: " << vCount;
  LOG(INFO) << "label count: " << lCount;
  LOG(INFO) << "edge count: " << eCount;

  // auto vertexColMap = vIter->getColIndices();
  // auto edgeColMap = eIter->getColIndices();
  // if (vertexColMap.size() < 1 || edgeColMap.size() < 1) {
  //   return Status::Error("vertex or edge column smaller than 1");
  // }

  // std::string tagName;
  // std::string edgeName;
  // for (auto entry : vertexColMap) {
  //   if (entry.first.find(labelName)) {
  //     tagName = entry.first.substr(0, entry.first.find("."));
  //     break;
  //   }
  // }
  // for (auto entry : edgeColMap) {
  //   if (entry.first.find(labelName)) {
  //     edgeName = entry.first.substr(0, entry.first.find("."));
  //   }
  // }

  // Example:
  // Vetices 3: 0, 1, 2, 3
  // Edges:
  // 0 1
  // 1 2
  // 2 3
  // 3 0
  // To store the out degree of each vertex
  // degree[0] = 2
  // degree[1] = 2
  // degree[2] = 2
  // degree[3] = 2
  std::vector<uint32_t> degree(vCount, 0);

  // To store the starting position of each vertex in neighborhood array.
  unsigned int* offset = new unsigned int[vCount + 1];
  // offset[0] = 0
  // offset[1] = 2
  // offset[2] = 4
  // offset[3] = 6
  // offset[4] = 8 // End of the neighborhood array

  // Array of the neighborhood can be initialized by 2 dimension of the matrix,
  // However, here we use 2*edge count as we have in edge and out edges.
  // Neighbors stores all the dest vId.
  // The format is [dst0 of src0, dst1 of src0,..., dst0 of src1, dst1 of src1,...]
  // neighbors = 1, 3, 0, 2, 1, 3, 2, 0
  unsigned int* neighbors = new unsigned int[eCount * 2];

  unsigned int* labels = new unsigned int[lCount];

  // load data vertices id and label
  while (vIter->valid()) {
    const auto vertex = vIter->getColumn(nebula::kVid);  // check if v is a vertex
    auto vId = vertex.getInt();
    const auto label = vIter->getTagProp("*", labelName);  // get label by index
    auto lId = label.getInt();
    labels[vId] = lId;
    vIter->next();
  }

  auto eIterCopy = eIter->copy();
  // calculate out degree
  while (eIter->valid()) {
    auto s = eIter->getEdgeProp("*", kSrc);
    auto src = s.getInt();  // vid starts from 0
    degree[src]++;
    eIter->next();
  }

  // caldulate the start position of each vertex in the neighborhood array
  offset[0] = 0;
  for (uint32_t i = 0; i < vCount; i++) {
    offset[i + 1] = degree[i] + offset[i];
  }

  std::vector<uint32_t> offsetCurr(offset, offset + vCount + 1);  // make a copy of the offset array

  // load data edges
  while (eIterCopy->valid()) {
    unsigned int src = eIterCopy->getEdgeProp("*", kSrc).getInt();
    unsigned int dst = eIterCopy->getEdgeProp("*", kDst).getInt();

    neighbors[offsetCurr[src]] = dst;
    offsetCurr[src]++;
    eIterCopy->next();
  }

  auto graph = std::make_unique<Graph>();
  graph->loadGraphFromExecutor(vCount, lCount, eCount, offset, neighbors, labels, degree);

  delete[] offset;
  delete[] neighbors;
  delete[] labels;

  return graph;
}

folly::Future<Status> IsomorExecutor::execute() {
  LOG(INFO) << "In Isomor executor";
  SCOPED_TIMER(&execTime_);
  auto* isomor = asNode<Isomor>(node());
  DataSet ds;
  ds.colNames = isomor->colNames();
  auto iterDV = ectx_->getResult(isomor->getdScanVOut()).iter();
  auto iterQV = ectx_->getResult(isomor->getqScanVOut()).iter();
  auto iterDE = ectx_->getResult(isomor->getdScanEOut()).iter();
  auto iterQE = ectx_->getResult(isomor->getqScanEOut()).iter();

  LOG(INFO) << "iter dv: " << iterDV->size() << " iterQV: " << iterQV->size();
  LOG(INFO) << "iter de: " << iterDE->size() << " iterQE: " << iterQE->size();

  // auto dataGraphRet = generateGraph(static_cast<PropIter*>(iterDV.get()),
  //                                   static_cast<PropIter*>(iterDE.get()),
  //                                   isomor->getLabel());
  auto queryGraphRet = generateGraph(static_cast<PropIter*>(iterQV.get()),
                                     static_cast<PropIter*>(iterQE.get()),
                                     isomor->getLabel());

  // if (!dataGraphRet.ok() || !queryGraphRet.ok()) {
  //   return Status::Error("Generating graph error!");
  // }

  // auto dataGraph = std::move(dataGraphRet).value();
  auto queryGraph = std::move(queryGraphRet).value();

  // dataGraph->printGraph();
  queryGraph->printGraph();

  // ui** candidates = nullptr;
  // ui* candidates_count = nullptr;

  // TreeNode* ceci_tree = nullptr;
  // ui* ceci_order = nullptr;
  // ui* provenance = nullptr;

  // //  Parent, first branch, second branch.
  // std::vector<std::unordered_map<V_ID, std::vector<V_ID>>> P_Candidates;
  // std::vector<std::unordered_map<V_ID, std::vector<V_ID>>> P_Provenance;

  // auto result = CECIFunction(dataGraph.get(),
  //                            queryGraph.get(),
  //                            candidates,
  //                            candidates_count,
  //                            ceci_order,
  //                            provenance,
  //                            ceci_tree,
  //                            P_Candidates,
  //                            P_Provenance);

  // ds.emplace_back(nebula::Row({result}));

  // delete[] ceci_order;
  // delete[] provenance;
  // delete[] candidates_count;
  // delete[] candidates;
  // delete ceci_tree;

  // Set result in the ds and set the new column name for the (isomor matching 's) result.
  auto status = finish(ResultBuilder().value(std::move(ds)).build());

  return status;
}
}  // namespace graph
}  // namespace nebula
