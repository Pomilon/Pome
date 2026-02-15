#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "nlohmann/json.hpp" // Adjusted path

using json = nlohmann::json;

// Simplified LSP
class PomeLSP {
public:
    void run() {
        while (true) {
            // Read Header
            std::string line;
            int contentLength = 0;
            while (std::getline(std::cin, line) && line != "\r") {
                if (line.rfind("Content-Length: ", 0) == 0) {
                    contentLength = std::stoi(line.substr(16));
                }
            }
            if (std::cin.eof()) break;

            // Read Body
            std::vector<char> buffer(contentLength);
            std::cin.read(buffer.data(), contentLength);
            std::string body(buffer.begin(), buffer.end());

            try {
                json request = json::parse(body);
                handleRequest(request);
            } catch (...) {
                // Ignore malformed JSON
            }
        }
    }

private:
    void handleRequest(const json& request) {
        if (!request.contains("method")) return;
        std::string method = request["method"];
        
        json response;
        response["jsonrpc"] = "2.0";
        if (request.contains("id")) response["id"] = request["id"];

        if (method == "initialize") {
            response["result"] = {
                {"capabilities", {
                    {"textDocumentSync", 1}, // Full sync
                    {"completionProvider", {
                        {"resolveProvider", false},
                        {"triggerCharacters", {"."}}
                    }}
                }}
            };
        }
        else if (method == "textDocument/completion") {
            // Basic completion
            response["result"] = {
                {
                    {"label", "print"},
                    {"kind", 3}, // Function
                    {"detail", "Standard Output"},
                    {"insertText", "print($1)"},
                    {"insertTextFormat", 2} // Snippet
                },
                {
                    {"label", "strict"},
                    {"kind", 14}, // Keyword
                    {"detail", "Enable strict mode"}
                },
                {
                    {"label", "var"},
                    {"kind", 14}, 
                    {"detail", "Declare variable"}
                }
            };
        }
        else if (method == "shutdown") {
            response["result"] = nullptr;
        }
        else if (method == "exit") {
            exit(0);
        }
        
        if (response.contains("id")) { // Only reply to requests
            sendResponse(response);
        }
    }

    void sendResponse(const json& response) {
        std::string str = response.dump();
        std::cout << "Content-Length: " << str.length() << "\r\n\r\n" << str << std::flush;
    }
};

int main() {
    PomeLSP lsp;
    lsp.run();
    return 0;
}
