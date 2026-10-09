#include "BenchCommon.hpp"

#include "GenerateBiconnectedTriangulations.hpp"

namespace tb
{
    namespace
    {
        class WithoutVGSRun final : public GeneratorRun
        {
            GenerateBiconnectedTriangulationsWithoutVGS generator_;

        public:
            WithoutVGSRun(long long vertexCount, const std::vector<std::vector<long long>> &adjacency, long long limit)
                : generator_(vertexCount, adjacency, limit) {}

            void generateAllTriangulations() override { generator_.generateAllTriangulations(); }

            GeneratorStats stats() const override
            {
                return {generator_.totalTriangulations, true, generator_.totalChecks,
                        generator_.totalChecks - generator_.unsuccessfulChecks};
            }
        };

        class WithVGSRun final : public GeneratorRun
        {
            GenerateBiconnectedTriangulationsWithVGS generator_;

        public:
            WithVGSRun(long long vertexCount, const std::vector<std::vector<long long>> &adjacency, long long limit)
                : generator_(vertexCount, adjacency, limit) {}

            void generateAllTriangulations() override { generator_.generateAllTriangulations(); }

            GeneratorStats stats() const override
            {
                return {generator_.totalTriangulations, false, 0, 0};
            }
        };
    } // namespace

    std::unique_ptr<GeneratorRun> makeBiconnectedWithoutVGS(
        long long vertexCount, const std::vector<std::vector<long long>> &adjacency, long long limit)
    {
        return std::make_unique<WithoutVGSRun>(vertexCount, adjacency, limit);
    }

    std::unique_ptr<GeneratorRun> makeBiconnectedWithVGS(
        long long vertexCount, const std::vector<std::vector<long long>> &adjacency, long long limit)
    {
        return std::make_unique<WithVGSRun>(vertexCount, adjacency, limit);
    }
} // namespace tb
