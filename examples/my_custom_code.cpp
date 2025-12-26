#include <iostream>
#include "../backend/include/manager.h"
#include "../backend/include/B_tree.h"
#include "../backend/include/garbage_collector.h"

using namespace atomic_tree;

int main() {
    std::cout << R"({"type":"init","name":"MyAtomicTreeApp"})" << std::endl;
    
    // Create your database
    Manager manager("my_custom_db.dat", 256 * 1024 * 1024, 256, true);
    
    BTreeConfig config;
    config.max_keys = 16;
    config.min_keys = 8;
    config.leaf_capacity = 32;
    
    BTree tree(&manager, config);
    
    // YOUR CUSTOM CODE HERE
    std::cout << "Inserting 10,000 custom records..." << std::endl;
    
    for (int i = 0; i < 10000; i++) {
        tree.insert(i, i * 100);
        
        // Output metrics for extension visualization (every 100 ops)
        if (i % 100 == 0) {
            std::cout << R"({"type":"metric","name":"throughput","value":)" << (100000 / (i+1)) << "}" << std::endl;
            std::cout << R"({"type":"metric","name":"operations","value":)" << i << "}" << std::endl;
        }
    }
    
    // Test search
    int value;
    if (tree.search(5000, value)) {
        std::cout << "Found key 5000: " << value << std::endl;
        std::cout << R"({"type":"event","name":"search_success","key":5000,"value":)" << value << "}" << std::endl;
    }
    
    // Run garbage collection
    GarbageCollector gc(&manager);
    gc.collect(tree.root_offset());
    
    std::cout << R"({"type":"event","name":"gc_complete","marked":)" << gc.nodes_marked() 
              << R"(,"freed":)" << gc.blocks_freed() << "}" << std::endl;
    
    std::cout << R"({"type":"complete","status":"success"})" << std::endl;
    
    return 0;
}
