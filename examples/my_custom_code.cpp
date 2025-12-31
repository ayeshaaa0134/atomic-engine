#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>


// Simple JSON printing helper
void log_json(std::string key, int value) {
  std::cout << "{\"" << key << "\": " << value << "}" << std::endl;
}

int main() {
  // 1. Disable buffering to ensure VS Code sees output immediately
  setvbuf(stdout, NULL, _IONBF, 0);

  std::cerr << "My Custom Backend Started!" << std::endl;

  int counter = 0;

  while (true) {
    // 2. Print JSON telemetry to stdout
    // The extension listens to this and updates the dashboard
    std::cout << "{\"iops\": " << (1000 + rand() % 500)
              << ", \"latency\": " << (10 + rand() % 5)
              << ", \"custom_msg\": \"Hello from C++ " << counter++ << "\"}"
              << std::endl;

    // 3. Sleep to simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  return 0;
}
