#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Nova {

// LOD (Level of Detail) configuration
struct LODLevel {
    float distance;          // Distance from camera
    float reductionRatio;    // 0.0-1.0, percentage of vertices to keep
    int targetTriangles;     // Target triangle count
    std::string meshPath;    // Path to pre-generated LOD mesh (optional)
};

struct LODConfig {
    std::vector<LODLevel> levels;
    bool autoGenerate = true;
    bool useContinuousLOD = false;  // Smooth transitions
    float transitionTime = 0.3f;    // Seconds for LOD fade
};

// Mesh optimization settings
struct MeshOptimizationConfig {
    bool optimizeVertexCache = true;
    bool optimizeOverdraw = true;
    bool optimizeVertexFetch = true;
    bool stripifyTriangles = true;
    bool generateNormals = true;
    bool generateTangents = false;
    bool weldVertices = true;
    float weldThreshold = 0.0001f;
    bool removeUnusedVertices = true;
    bool removeDegenerateTriangles = true;
};

// Mesh statistics
struct MeshStats {
    std::string path;
    size_t vertexCount = 0;
    size_t triangleCount = 0;
    size_t indexCount = 0;
    size_t memoryBytes = 0;
    float boundingRadius = 0.0f;
    bool hasNormals = false;
    bool hasTangents = false;
    bool hasUVs = false;
    bool hasColors = false;
    int lodLevels = 0;
};

class MeshOptimizer {
public:
    static MeshOptimizer& GetInstance() {
        static MeshOptimizer instance;
        return instance;
    }

    // === MESH OPTIMIZATION ===
    bool OptimizeMesh(const std::string& inputPath, const std::string& outputPath,
                     const MeshOptimizationConfig& config);
    bool OptimizeMeshInPlace(const std::string& meshPath, const MeshOptimizationConfig& config);
    void SetDefaultOptimizationConfig(const MeshOptimizationConfig& config);
    MeshOptimizationConfig GetDefaultOptimizationConfig() const { return defaultConfig_; }
    
    // === VERTEX CACHE OPTIMIZATION ===
    bool OptimizeVertexCache(std::vector<unsigned int>& indices);
    float CalculateACMR(const std::vector<unsigned int>& indices, size_t vertexCount, int cacheSize = 32);
    void ReorderTriangles(std::vector<unsigned int>& indices);
    
    // === OVERDRAW OPTIMIZATION ===
    bool OptimizeOverdraw(std::vector<unsigned int>& indices, 
                         const std::vector<float>& vertices, int vertexStride);
    float CalculateOverdrawScore(const std::vector<unsigned int>& indices,
                                const std::vector<float>& vertices, int vertexStride);
    
    // === VERTEX FETCH OPTIMIZATION ===
    bool OptimizeVertexFetch(std::vector<float>& vertices, std::vector<unsigned int>& indices, 
                            int vertexStride);
    bool RemoveDuplicateVertices(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                                int vertexStride, float threshold = 0.0001f);
    
    // === MESH SIMPLIFICATION ===
    bool SimplifyMesh(const std::string& inputPath, const std::string& outputPath,
                     float reductionRatio, float targetError = 0.01f);
    bool SimplifyMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                     int vertexStride, float reductionRatio, float targetError = 0.01f);
    int CalculateTargetTriangleCount(int currentCount, float reductionRatio);
    
    // === LOD GENERATION ===
    bool GenerateLODChain(const std::string& meshPath, const LODConfig& config);
    bool GenerateLODLevel(const std::string& baseMeshPath, const std::string& outputPath,
                         float reductionRatio);
    void SetLODConfig(const std::string& meshPath, const LODConfig& config);
    LODConfig GetLODConfig(const std::string& meshPath) const;
    int SelectLODLevel(const std::string& meshPath, float distance) const;
    
    // === MESH ANALYSIS ===
    MeshStats AnalyzeMesh(const std::string& meshPath);
    void CacheMeshStats(const std::string& meshPath, const MeshStats& stats);
    size_t CalculateMemoryUsage(const MeshStats& stats);
    std::vector<std::string> FindLargeMeshes(size_t minTriangles = 10000);
    
    // === GEOMETRY PROCESSING ===
    bool GenerateNormals(std::vector<float>& vertices, const std::vector<unsigned int>& indices,
                        int vertexStride, bool smooth = true);
    bool GenerateTangents(std::vector<float>& vertices, const std::vector<unsigned int>& indices,
                         int vertexStride);
    bool CalculateBounds(const std::vector<float>& vertices, int vertexStride,
                        float& minX, float& minY, float& minZ,
                        float& maxX, float& maxY, float& maxZ);
    
    // === MESH COMPRESSION ===
    bool CompressMesh(const std::string& inputPath, const std::string& outputPath,
                     int positionPrecision = 16, int uvPrecision = 12);
    bool DecompressMesh(const std::string& inputPath, const std::string& outputPath);
    size_t EstimateCompressedSize(const MeshStats& stats);
    
    // === BATCH OPERATIONS ===
    void OptimizeDirectory(const std::string& directory, bool recursive = true);
    void GenerateLODsForDirectory(const std::string& directory, const LODConfig& config,
                                 bool recursive = true);
    int BatchOptimize(const std::vector<std::string>& meshes,
                     const MeshOptimizationConfig& config);
    
    // === INSTANCING SUPPORT ===
    bool CanUseInstancing(const std::string& meshPath, int minInstances = 10);
    void MarkForInstancing(const std::string& meshPath, bool enable = true);
    std::vector<std::string> GetInstanceableMeshes() const;
    
    // === DIAGNOSTICS ===
    void DumpMeshReport(const std::string& outputPath);
    size_t GetTotalMeshMemory() const;
    int GetMeshCount() const;
    void ClearCache();

private:
    MeshOptimizer() = default;
    ~MeshOptimizer() = default;
    
    MeshOptimizer(const MeshOptimizer&) = delete;
    MeshOptimizer& operator=(const MeshOptimizer&) = delete;

    // Internal optimization helpers
    void BuildAdjacencyList(const std::vector<unsigned int>& indices,
                           std::vector<std::vector<unsigned int>>& adjacency);
    float CalculateTriangleScore(int cachePosition, int vertexUsage);
    void SortTrianglesByDepth(std::vector<unsigned int>& indices,
                             const std::vector<float>& vertices, int vertexStride);
    
    MeshOptimizationConfig defaultConfig_;
    std::unordered_map<std::string, MeshStats> statsCache_;
    std::unordered_map<std::string, LODConfig> lodConfigs_;
    std::unordered_map<std::string, bool> instanceableMeshes_;
};

} // namespace Nova
