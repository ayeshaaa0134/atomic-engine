#include <iostream>
#include "../backend/include/manager.h"
#include "../backend/include/B_tree.h"

using namespace atomic_tree;
using namespace std;

int main() {
    // Simple example - just insert numbers in a loop
    cout << "Starting AtomicTree..." << endl;
    
    // Create database (small 64MB for quick demo)
    Manager mgr("simple_demo.dat", 64*1024*1024, 256, true);
    
    BTreeConfig cfg{16, 8, 32};
    BTree tree(&mgr, cfg);
    
    // Simple loop - insert 1000 numbers
    for(int i = 0; i < 1000; i++) {
        tree.insert(i, i * 2);
        
        // Show progress every 100
        if(i % 100 == 0) {
            cout << "Inserted " << i << " keys..." << endl;
        }
    }
    
    // Search for a value
    int value;
    if(tree.search(500, value)) {
        cout << "Found key 500: value = " << value << endl;
    }
    
    cout << "Done!" << endl;
    return 0;
}
