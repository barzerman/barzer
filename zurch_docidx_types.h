#pragma once

#include <vector>

/// ATTENTION! keep the types in this files very simple. no class/struct definitions please - only typedefs etc
namespace zurch {
typedef std::pair< uint32_t, double > DocWithScore_t;
typedef std::vector< DocWithScore_t > DocWithScoreVec_t;

struct SimpleIdx;
class DocDataIndex;
class DocFeatureIndex;

class ZurchSettings;    
class DocFeatureLoader;
class DocIndexLoaderNamedDocs;
class DocIndexAndLoader;
struct ReqFilterCascade;

struct DocFeatureIndexHeuristics;
struct DocFeature;
struct FeatureDocPosition;
struct ExtractedDocFeature;
struct DocFeatureLink;
} // namespace zurch
