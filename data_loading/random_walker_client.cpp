#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

int main(int argc, char* argv[]) {
    // Default values
    std::string serverHost = "127.0.0.1";
    int serverPort = 8080;
    int numWalks = 10;
    int walkLength = 15;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-h" || arg == "--host") && i + 1 < argc) {
            serverHost = argv[++i];
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            serverPort = std::atoi(argv[++i]);
        } else if ((arg == "-w" || arg == "--walks") && i + 1 < argc) {
            numWalks = std::atoi(argv[++i]);
        } else if ((arg == "-l" || arg == "--length") && i + 1 < argc) {
            walkLength = std::atoi(argv[++i]);
        } else {
            std::cerr << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  -h, --host HOST      Server host (default: 127.0.0.1)\n"
                      << "  -p, --port PORT      Server port (default: 8080)\n"
                      << "  -w, --walks N        Number of walks per node (default: 10)\n"
                      << "  -l, --length N       Length of each walk (default: 15)\n";
            return 1;
        }
    }
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }
    
    // Set up server address
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(serverPort);
    
    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, serverHost.c_str(), &servAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported\n";
        return 1;
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        std::cerr << "Connection failed\n";
        return 1;
    }
    
    std::cout << "Connected to server at " << serverHost << ":" << serverPort << "\n";
    
    // Create request with parameters
    std::stringstream requestStream;
    requestStream << "GET_RANDOM_WALKS" << " " << numWalks << " " << walkLength;
    std::string requestStr = requestStream.str();
    
    std::cout << "Sending request: " << requestStr << "\n";
    
    // Send the request
    send(sock, requestStr.c_str(), requestStr.length(), 0);
    
    // Receive response (file path)
    char buffer[4096];
    int bytesRead = read(sock, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string filePath(buffer);
        
        std::cout << "Random walks have been generated and saved to: " << filePath << "\n";
    } else {
        std::cerr << "Error receiving response from server\n";
    }
    
    // Close socket
    close(sock);
    
    return 0;
}