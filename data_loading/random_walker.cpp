#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <ctime>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <algorithm>
#include <cstring>
// Add socket programming headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <set>

// Replace the existing Graph definition with this enhanced version
struct Edge {
    std::string predicate;
    std::string target;
};

using Graph = std::unordered_map<std::string, std::vector<Edge>>;

bool parseTriple(const std::string& line, std::string& subject, std::string& predicate, std::string& object) {
    std::istringstream iss(line);
    if (!(iss >> subject >> predicate >> object))
        return false;
    if (!object.empty() && object.back() == '.')
        object.pop_back();
    return true;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string formatDuration(std::chrono::duration<double> duration) {
    double seconds = duration.count();
    if (seconds < 60) {
        return std::to_string(seconds) + " seconds";
    } else if (seconds < 3600) {
        int minutes = static_cast<int>(seconds) / 60;
        int remainingSeconds = static_cast<int>(seconds) % 60;
        return std::to_string(minutes) + " minutes " + std::to_string(remainingSeconds) + " seconds";
    } else {
        int hours = static_cast<int>(seconds) / 3600;
        int minutes = (static_cast<int>(seconds) % 3600) / 60;
        int remainingSeconds = static_cast<int>(seconds) % 60;
        return std::to_string(hours) + " hours " + std::to_string(minutes) + " minutes " + std::to_string(remainingSeconds) + " seconds";
    }
}

Graph loadGraph(const std::string& filename) {
    Graph graph;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::clog << "[" << getCurrentTimestamp() << "] Error opening file: " << filename << "\n";
        return graph;
    }
    std::clog << "[" << getCurrentTimestamp() << "] Parsing file: " << filename << "\n";
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::string line;
    int count = 0, lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#')
            continue;
        std::string subject, predicate, object;
        if (parseTriple(line, subject, predicate, object)) {
            // Store predicate and object
            graph[subject].push_back({predicate, object});
            count++;
        } else {
            std::clog << "[" << getCurrentTimestamp() << "] Failed to parse line " << lineNum << ": " << line << "\n";
        }
        if (lineNum % 10000000 == 0) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = currentTime - startTime;
            double rate = lineNum / elapsed.count();
            std::clog << "[" << getCurrentTimestamp() << "] Processed " << lineNum << " lines... (" 
                      << static_cast<int>(rate) << " lines/sec)\n";
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    double rate = count > 0 ? count / elapsed.count() : 0;
    
    std::clog << "[" << getCurrentTimestamp() << "] Parsed " << count << " triples from " << lineNum << " lines in " 
              << formatDuration(elapsed) << " (" << static_cast<int>(rate) << " triples/sec).\n";
    return graph;
}

// Enhanced randomWalk function with duplicate avoidance
std::vector<std::string> randomWalk(const Graph& graph, const std::string& start, int length, 
                                   std::mt19937& rng, 
                                   const std::set<std::string>* visitedWalkStrings = nullptr) {
    std::vector<std::string> walk{start};
    std::string current = start;
    
    // For tracking choices made at each step to allow backtracking
    std::vector<std::pair<int, std::vector<int>>> choicesHistory;
    
    // Walk length now refers to number of entities (excluding predicates)
    // We'll add predicates in between, so the final path length could be up to 2*length - 1
    for (int i = 0; i < length - 1; i++) {  // -1 because we already have the start node
        auto it = graph.find(current);
        if (it == graph.end() || it->second.empty())
            break;
            
        const auto &edges = it->second;
        
        // Record all possible choices at this step
        std::vector<int> availableChoices;
        for (int j = 0; j < static_cast<int>(edges.size()); j++) {
            availableChoices.push_back(j);
        }
        
        // If only one option, no need to shuffle
        if (availableChoices.size() > 1) {
            std::shuffle(availableChoices.begin(), availableChoices.end(), rng);
        }
        
        // Select the first available choice
        int edgeIndex = availableChoices[0];
        
        // Store the choice made and remaining options for potential backtracking
        choicesHistory.push_back({edgeIndex, availableChoices});
        
        // Add both predicate and target to the walk
        walk.push_back(edges[edgeIndex].predicate);
        walk.push_back(edges[edgeIndex].target);
        
        current = edges[edgeIndex].target;
    }
    
    return walk;
}

// Generate a string representation of a walk for duplicate detection
std::string walkToString(const std::vector<std::string>& walk) {
    std::stringstream ss;
    for (const auto& node : walk) {
        ss << node << ",";
    }
    return ss.str();
}

// Create a new function that generates distinct walks
std::vector<std::vector<std::string>> generateDistinctWalks(
    const Graph& graph, const std::string& startNode, int numWalks, int walkLength, std::mt19937& rng) {
    
    std::vector<std::vector<std::string>> walks;
    std::set<std::string> walkStrings; // To track unique walks
    
    int attempts = 0;
    int duplicates = 0;
    const int maxAttemptsPerWalk = 10; // Maximum tries to generate a unique walk
    
    while (walks.size() < numWalks && attempts < numWalks * maxAttemptsPerWalk) {
        auto walk = randomWalk(graph, startNode, walkLength, rng);
        std::string walkStr = walkToString(walk);
        
        attempts++;
        
        // Check if this walk is already generated
        if (walkStrings.find(walkStr) == walkStrings.end()) {
            // New unique walk
            walkStrings.insert(walkStr);
            walks.push_back(walk);
        } else {
            // Duplicate walk found
            duplicates++;
            
            // If we have too many duplicates, we might have exhausted all possible unique paths
            if (duplicates > numWalks * 2) {
                std::clog << "[" << getCurrentTimestamp() << "] WARNING: High number of duplicate walks from node " 
                          << startNode << ". Possibly limited path diversity.\n";
                
                // Accept some duplicates if we can't find enough unique walks
                if (walks.size() < numWalks / 2) {
                    walks.push_back(walk);
                    std::clog << "[" << getCurrentTimestamp() << "] Accepting some duplicate walks to meet quota.\n";
                }
            }
        }
        
        // Log progress for excessive attempts
        if (attempts % (numWalks * 2) == 0) {
            std::clog << "[" << getCurrentTimestamp() << "] Generated " << walks.size() 
                      << " unique walks after " << attempts << " attempts. Duplicates: " << duplicates << "\n";
        }
    }
    
    if (walks.size() < numWalks) {
        std::clog << "[" << getCurrentTimestamp() << "] Could only generate " << walks.size() 
                  << " unique walks out of " << numWalks << " requested from node " << startNode << "\n";
    }
    
    return walks;
}


void writeWalkToCSV(std::ofstream& file, const std::vector<std::string>& walk, std::mutex& mtx) {
    std::stringstream ss;
    for (size_t i = 0; i < walk.size(); i++) {
        ss << walk[i];
        if (i < walk.size() - 1)
            ss << ",";
    }
    ss << "\n";
    
    std::lock_guard<std::mutex> lock(mtx);
    file << ss.str();
}

void processWalks(const Graph& graph, const std::vector<std::string>& nodes, int numWalks, int walkLength, 
                 std::mutex &mtx, int threadId, std::ofstream& outFile) {
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)) + threadId);
    for (const auto& node : nodes) {
        for (int i = 0; i < numWalks; i++) {
            auto walk = randomWalk(graph, node, walkLength, rng);
            
            // Write to CSV file
            writeWalkToCSV(outFile, walk, mtx);
            
            {
                std::lock_guard<std::mutex> lock(mtx);
                std::cout << "Random walk from " << node << ": ";
                for (const auto& n : walk)
                    std::cout << n << " ";
                std::cout << "\n";
            }
        }
    }
}

void benchmarkRandomWalks(const Graph& graph, const std::string& startNode, int numWalks, int walkLength) {
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    
    std::clog << "[" << getCurrentTimestamp() << "] Starting benchmark: " << numWalks 
              << " random walks of length " << walkLength << " from node " << startNode << "\n";
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    int walksCompleted = 0;
    for (int i = 0; i < numWalks; i++) {
        auto walk = randomWalk(graph, startNode, walkLength, rng);
        walksCompleted++;
        
        if (walksCompleted % 1000 == 0) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = currentTime - startTime;
            double rate = walksCompleted / elapsed.count();
            std::clog << "[" << getCurrentTimestamp() << "] Generated " << walksCompleted 
                      << " walks... (" << static_cast<int>(rate) << " walks/sec)\n";
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalTime = endTime - startTime;
    double rate = numWalks / totalTime.count();
    
    std::clog << "[" << getCurrentTimestamp() << "] Benchmark complete: Generated " << numWalks 
              << " random walks in " << formatDuration(totalTime) << "\n";
    std::clog << "[" << getCurrentTimestamp() << "] Performance: " << static_cast<int>(rate) 
              << " walks/sec\n";
}

// Buffer for efficient writing
class WalkBuffer {
private:
    std::vector<std::string> buffer;
    std::ofstream& outFile;
    std::mutex& mtx;
    size_t maxSize;

public:
    WalkBuffer(std::ofstream& file, std::mutex& mutex, size_t bufferSize = 10000) 
        : outFile(file), mtx(mutex), maxSize(bufferSize) {
        buffer.reserve(maxSize);
    }

    void add(const std::string& line) {
        buffer.push_back(line);
        if (buffer.size() >= maxSize) {
            flush();
        }
    }

    void flush() {
        if (buffer.empty()) return;
        
        std::string data;
        for (const auto& line : buffer) {
            data += line;
        }
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            outFile << data;
        }
        
        buffer.clear();
    }

    ~WalkBuffer() {
        flush();
    }
};

std::string walkToCSV(const std::vector<std::string>& walk) {
    std::stringstream ss;
    for (size_t i = 0; i < walk.size(); i++) {
        ss << walk[i];
        if (i < walk.size() - 1)
            ss << ",";
    }
    ss << "\n";
    return ss.str();
}

void generateRandomWalks(const Graph& graph, const std::vector<std::string>& startNodes, 
                         int numWalksPerNode, int walkLength, std::ofstream& outFile,
                         int threadId, std::mutex& fileMutex, std::atomic<int>& walkCounter) {
    
    // Create RNG with unique seed per thread
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)) + threadId);
    
    // Create buffer for efficient writing
    WalkBuffer buffer(outFile, fileMutex);
    
    int localWalks = 0;
    for (const auto& node : startNodes) {
        for (int i = 0; i < numWalksPerNode; i++) {
            auto walk = randomWalk(graph, node, walkLength, rng);
            buffer.add(walkToCSV(walk));
            
            localWalks++;
            walkCounter++;
            
            // Periodic status update
            if (walkCounter.load() % 10000 == 0) {
                std::lock_guard<std::mutex> lock(fileMutex);
                std::clog << "[" << getCurrentTimestamp() << "] Generated " << walkCounter.load() << " walks\n";
            }
        }
    }
    
    // Make sure to flush remaining walks
    buffer.flush();
}

// Helper function to check if a node is a predicate
// bool isPredicate(const std::string& node) {
//     // You may need to customize this based on your knowledge of predicates in your graph
//     // For example, if predicates always start with a specific prefix:
//     return node.substr(0, 4) == "http" && node.find("property") != std::string::npos;
//     // Or if you have a set of known predicates:
//     // static const std::unordered_set<std::string> predicates = {"type", "subClassOf", ...};
//     // return predicates.find(node) != predicates.end();
// }

std::vector<std::string> getStartNodes(const Graph& graph, float sampleRate = 1.0) {
    std::vector<std::string> nodes;
    nodes.reserve(graph.size() * sampleRate);
    
    for (const auto& pair : graph) {
        // Only include nodes that have neighbors and are not predicates
        if (!pair.second.empty()) {
            nodes.push_back(pair.first);
        }
    }
    
    // If sampling, shuffle and take a subset
    if (sampleRate < 1.0) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(nodes.begin(), nodes.end(), g);
        
        size_t sampleSize = static_cast<size_t>(nodes.size() * sampleRate);
        nodes.resize(sampleSize);
    }
    
    return nodes;
}

void runParallelRandomWalks(const Graph& graph, const std::string& outputFile, 
                           int numWalksPerNode, int walkLength, float nodeSampleRate, int numThreads) {
    
    std::clog << "[" << getCurrentTimestamp() << "] Starting parallel random walks generation\n";
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Get nodes to start walks from
    std::vector<std::string> startNodes = getStartNodes(graph, nodeSampleRate);
    
    std::clog << "[" << getCurrentTimestamp() << "] Selected " << startNodes.size() 
              << " start nodes (sampling rate: " << nodeSampleRate << ")\n";
    
    // Open output file
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        std::clog << "[" << getCurrentTimestamp() << "] Error opening output file: " << outputFile << "\n";
        return;
    }
    
    // Prepare for parallel execution
    std::vector<std::thread> threads;
    std::mutex fileMutex;
    std::atomic<int> totalWalks{0};
    
    // Divide work among threads
    size_t nodesPerThread = (startNodes.size() + numThreads - 1) / numThreads;
    
    for (int i = 0; i < numThreads; i++) {
        size_t startIdx = i * nodesPerThread;
        size_t endIdx = std::min(startIdx + nodesPerThread, startNodes.size());
        
        if (startIdx >= startNodes.size()) break;
        
        std::vector<std::string> threadNodes(startNodes.begin() + startIdx, startNodes.begin() + endIdx);
        
        threads.emplace_back(generateRandomWalks, std::ref(graph), std::ref(threadNodes),
                            numWalksPerNode, walkLength, std::ref(outFile),
                            i, std::ref(fileMutex), std::ref(totalWalks));
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalTime = endTime - startTime;
    
    std::clog << "[" << getCurrentTimestamp() << "] Random walks generation complete: Generated " 
              << totalWalks << " walks in " << formatDuration(totalTime) << "\n";
    
    double rate = totalWalks / totalTime.count();
    std::clog << "[" << getCurrentTimestamp() << "] Performance: " << static_cast<int>(rate) 
              << " walks/sec\n";
}

// Class to manage used start nodes to ensure variety in responses
class NodeManager {
private:
    std::vector<std::string> allNodes;
    std::set<std::string> usedNodes;
    std::mutex mtx;
    std::mt19937 rng;
    size_t batchSize;
    float sampleRate;

public:
    NodeManager(const Graph& graph, float sampleRate = 1.0, size_t batchSize = 100) 
        : batchSize(batchSize), sampleRate(sampleRate) {
        
        // Initialize with all nodes from graph
        for (const auto& pair : graph) {
            if (!pair.second.empty()) {
                allNodes.push_back(pair.first);
            }
        }
        
        // Seed the random number generator
        std::random_device rd;
        rng = std::mt19937(rd());
        
        std::clog << "[" << getCurrentTimestamp() << "] NodeManager initialized with " 
                  << allNodes.size() << " potential start nodes\n";
    }
    
    std::vector<std::string> getNextBatch() {
        std::lock_guard<std::mutex> lock(mtx);
        
        // If we've used all nodes or close to it, reset the used nodes
        if (usedNodes.size() >= allNodes.size() * 0.9) {
            std::clog << "[" << getCurrentTimestamp() << "] Resetting used nodes list\n";
            usedNodes.clear();
        }
        
        // Create a pool of unused nodes to sample from
        std::vector<std::string> unusedNodes;
        for (const auto& node : allNodes) {
            if (usedNodes.find(node) == usedNodes.end()) {
                unusedNodes.push_back(node);
            }
        }
        
        // Apply sampling rate to the unused nodes
        if (sampleRate < 1.0 && !unusedNodes.empty()) {
            std::shuffle(unusedNodes.begin(), unusedNodes.end(), rng);
            size_t sampleSize = std::max(size_t(1), static_cast<size_t>(unusedNodes.size() * sampleRate));
            unusedNodes.resize(sampleSize);
        }
        
        // Select up to batchSize nodes
        std::vector<std::string> batch;
        size_t count = std::min(batchSize, unusedNodes.size());
        
        if (!unusedNodes.empty()) {
            // Shuffle and take the first 'count' elements
            std::shuffle(unusedNodes.begin(), unusedNodes.end(), rng);
            batch.assign(unusedNodes.begin(), unusedNodes.begin() + count);
            
            // Mark these nodes as used
            for (const auto& node : batch) {
                usedNodes.insert(node);
            }
        }
        
        std::clog << "[" << getCurrentTimestamp() << "] Returning batch of " 
                  << batch.size() << " nodes (" << usedNodes.size() << "/" 
                  << allNodes.size() << " used so far)\n";
                  
        return batch;
    }
};

// Structure for socket-based server
void serveRandomWalks(const Graph& graph, int port, int defaultNumWalksPerNode, 
                      int defaultWalkLength, float nodeSampleRate, int numThreads) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    
    // Create the node manager
    NodeManager nodeManager(graph, nodeSampleRate);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::clog << "[" << getCurrentTimestamp() << "] Socket creation failed\n";
        return;
    }
    
    // Set socket options to reuse address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::clog << "[" << getCurrentTimestamp() << "] setsockopt failed\n";
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::clog << "[" << getCurrentTimestamp() << "] Bind failed on port " << port << "\n";
        return;
    }
    
    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        std::clog << "[" << getCurrentTimestamp() << "] Listen failed\n";
        return;
    }
    
    std::clog << "[" << getCurrentTimestamp() << "] Server started on port " << port 
              << ", waiting for connections...\n";
    
    while (true) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::clog << "[" << getCurrentTimestamp() << "] Accept failed\n";
            continue;
        }
        
        std::clog << "[" << getCurrentTimestamp() << "] New connection accepted\n";
        
        // Read request
        int valread = read(new_socket, buffer, 1024);
        if (valread <= 0) {
            close(new_socket);
            continue;
        }
        
        // Parse request (expected format: "GET_RANDOM_WALKS numWalks walkLength")
        std::string request(buffer);
        std::istringstream requestStream(request);
        std::string command;
        int numWalks = defaultNumWalksPerNode;
        int walkLength = defaultWalkLength;
        
        requestStream >> command;
        if (command == "GET_RANDOM_WALKS") {
            // Try to parse numWalks and walkLength if provided
            if (requestStream >> numWalks) {
                if (!(requestStream >> walkLength)) {
                    walkLength = defaultWalkLength;
                }
            } else {
                numWalks = defaultNumWalksPerNode;
            }
        }
        
        std::clog << "[" << getCurrentTimestamp() << "] Received request with parameters: "
                  << "numWalks=" << numWalks << ", walkLength=" << walkLength << "\n";
        
        // Create unique filename based on timestamp
        auto now = std::chrono::system_clock::now();
        auto nowMs = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto epoch = nowMs.time_since_epoch();
        auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
        std::string timestamp = std::to_string(value.count());
        
        std::string outputDir = "walks_output";
        system(("mkdir -p " + outputDir).c_str());
        std::string outputFile = outputDir + "/walks_" + timestamp + ".csv";
        std::clog << "[" << getCurrentTimestamp() << "] Creating output file: " << outputFile << "\n";
        
        // Get a new batch of start nodes
        std::vector<std::string> startNodes = nodeManager.getNextBatch();
        
        // Open output file
        std::ofstream outFile(outputFile);
        if (!outFile.is_open()) {
            std::clog << "[" << getCurrentTimestamp() << "] Error opening output file: " << outputFile << "\n";
            std::string errorMsg = "ERROR: Could not create output file";
            send(new_socket, errorMsg.c_str(), errorMsg.size(), 0);
            close(new_socket);
            continue;
        }
        
        // Start timing the walk generation
        auto walkGenStart = std::chrono::high_resolution_clock::now();
        
        // Generate walks for this batch and write to file
        std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
        int walkCount = 0;
        int duplicateCount = 0;
        
        for (const auto& node : startNodes) {
            // Track distinct walks for this node
            std::set<std::string> nodeWalks;
            
            // Generate distinct walks for this node
            auto walks = generateDistinctWalks(graph, node, numWalks, walkLength, rng);
            
            // Write walks to file
            for (const auto& walk : walks) {
                // Write walk to CSV file
                for (size_t j = 0; j < walk.size(); j++) {
                    outFile << walk[j];
                    if (j < walk.size() - 1) outFile << ",";
                }
                outFile << "\n";
                walkCount++;
            }
            
            // Count how many duplicate walks we had to handle
            duplicateCount += (numWalks - walks.size());
        }
        
        outFile.close();
        
        // End timing and calculate duration
        auto walkGenEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> walkGenTime = walkGenEnd - walkGenStart;
        double walkRate = walkCount / walkGenTime.count();
        
        // Send the file path as response
        std::string absolutePath = std::string("/gpfs/workdir/mortadii/GraphTwin-ai/") + outputFile;
        send(new_socket, absolutePath.c_str(), absolutePath.size(), 0);
        
        std::clog << "[" << getCurrentTimestamp() << "] Generated and saved " 
                  << walkCount << " walks to " << outputFile << " in " 
                  << formatDuration(walkGenTime) << " (" 
                  << static_cast<int>(walkRate) << " walks/sec)\n";
                  
        if (duplicateCount > 0) {
            std::clog << "[" << getCurrentTimestamp() << "] Handled " << duplicateCount 
                      << " potential duplicate walks during generation\n";
        }
                  
        close(new_socket);
    }
    
    // Close the socket (this is unreachable in the current implementation)
    close(server_fd);
}

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -f, --file FILE       Input graph file (default: data/dbpedia_ml.nt)\n"
              << "  -o, --output FILE     Output file for walks (default: walks.csv)\n"
              << "  -w, --walks N         Number of walks per node (default: 10)\n"
              << "  -l, --length N        Length of each walk (default: 15)\n"
              << "  -s, --sample RATE     Sampling rate for start nodes (0.0-1.0, default: 1.0)\n"
              << "  -t, --threads N       Number of threads (default: 4)\n"
              << "  -S, --server          Run as a server serving random walks over a socket\n"
              << "  -p, --port N          Port number for server mode (default: 8080)\n"
              << "  -h, --help            Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::clog << "[" << getCurrentTimestamp() << "] Starting graph processing\n";
    
    // Default parameters
    std::string inputFile = "data/dbpedia_ml.nt";
    std::string outputFile = "walks.csv";
    int numWalksPerNode = 10;
    int walkLength = 15;
    float nodeSampleRate = 1.0;
    int numThreads = 4;
    bool serverMode = false;
    int port = 8080;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if ((arg == "-f" || arg == "--file") && i + 1 < argc) {
            inputFile = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputFile = argv[++i];
        } else if ((arg == "-w" || arg == "--walks") && i + 1 < argc) {
            numWalksPerNode = std::atoi(argv[++i]);
        } else if ((arg == "-l" || arg == "--length") && i + 1 < argc) {
            walkLength = std::atoi(argv[++i]);
        } else if ((arg == "-s" || arg == "--sample") && i + 1 < argc) {
            nodeSampleRate = std::atof(argv[++i]);
        } else if ((arg == "-t" || arg == "--threads") && i + 1 < argc) {
            numThreads = std::atoi(argv[++i]);
        } else if (arg == "-S" || arg == "--server") {
            serverMode = true;
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Load the graph
    auto graphLoadStart = std::chrono::high_resolution_clock::now();
    Graph graph = loadGraph(inputFile);
    auto graphLoadEnd = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> graphLoadTime = graphLoadEnd - graphLoadStart;
    std::clog << "[" << getCurrentTimestamp() << "] Graph loading completed in " << formatDuration(graphLoadTime) << "\n";
    
    if (graph.empty()) {
        std::clog << "[" << getCurrentTimestamp() << "] Graph is empty. Exiting.\n";
        return 1;
    }
    
    std::clog << "[" << getCurrentTimestamp() << "] Graph has " << graph.size() << " nodes\n";
    
    if (serverMode) {
        // Run in server mode
        std::clog << "[" << getCurrentTimestamp() << "] Starting in server mode on port " << port << "\n";
        serveRandomWalks(graph, port, numWalksPerNode, walkLength, nodeSampleRate, numThreads);
    } else {
        // Generate walks in parallel and write to file
        runParallelRandomWalks(graph, outputFile, numWalksPerNode, walkLength, nodeSampleRate, numThreads);
    }
    
    return 0;
}