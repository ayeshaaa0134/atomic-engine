# AtomicTree API Documentation

Complete API reference for the Atomic Tree Engine.

---

## Table of Contents

1. [Manager API](#manager-api)
2. [BTree API](#btree-api)
3. [GarbageCollector API](#garbagecollector-api)
4. [Primitives API](#primitives-api)

---

## Manager API

**Header**: `manager.h`  
**Purpose**: Persistent memory allocation and management

### Constructor

```cpp
Manager(const std::string& filename, 
        std::size_t region_size, 
        std::size_t block_size, 
        bool create_new = false)
```

**Parameters**:
- `filename`: Path to backing file (e.g., `"mydb.dat"`)
- `region_size`: Total memory region size in bytes (e.g., `1GB`)
- `block_size`: Fixed block size in bytes (recommended: `256`)
- `create_new`: If `true`, overwrites existing file; if `false`, opens existing

**Example**:
```cpp
// Create new 1GB database
Manager mgr("db.dat", 1024*1024*1024, 256, true);

// Reopen existing database
Manager mgr("db.dat", 1024*1024*1024, 256, false);
```

---

### alloc_block()

```cpp
std::uint64_t alloc_block()
```

**Returns**: Offset to newly allocated block  
**Throws**: `std::runtime_error` if out of memory

**Example**:
```cpp
uint64_t offset = mgr.alloc_block();
void* ptr = mgr.offset_to_ptr(offset);
```

---

### free_block()

```cpp
void free_block(std::uint64_t offset)
```

**Parameters**:
- `offset`: Offset returned by `alloc_block()`

**Example**:
```cpp
mgr.free_block(offset);
```

---

### offset_to_ptr()

```cpp
void* offset_to_ptr(std::uint64_t offset)
```

**Returns**: Absolute pointer to memory location  
**Note**: Use for read/write operations, but store offsets for persistence

---

### Accessors

```cpp
void* base() const;                // Base address of mapped region
std::size_t region_size() const;   // Total size in bytes
std::size_t block_size() const;    // Block size in bytes
std::size_t block_count() const;   // Number of blocks
std::uint64_t* get_bitmap();       // Direct bitmap access (advanced)
```

---

## BTree API

**Header**: `B_tree.h`  
**Purpose**: NV-Tree B+ Tree operations

### Configuration

```cpp
struct BTreeConfig {
    int max_keys;        // Maximum keys per internal node (default: 16)
    int min_keys;        // Minimum keys per node (default: 8)
    int leaf_capacity;   // Leaf entry slots (default: 32)
};
```

---

### Constructor

```cpp
BTree(Manager* manager, const BTreeConfig& config)
```

**Parameters**:
- `manager`: Pointer to initialized Manager
- `config`: Tree configuration

**Example**:
```cpp
BTreeConfig config;
config.max_keys = 16;
config.min_keys = 8;
config.leaf_capacity = 32;

BTree tree(&manager, config);
```

---

### insert()

```cpp
void insert(int key, int value)
```

**Parameters**:
- `key`: Integer key (32-bit)
- `value`: Integer value (32-bit)

**Behavior**:
- Appends to unsorted leaf node
- Triggers atomic split if leaf is full
- Guarantees crash consistency

**Example**:
```cpp
tree.insert(42, 100);
tree.insert(17, 200);
```

---

### search()

```cpp
bool search(int key, int& out_value) const
```

**Parameters**:
- `key`: Key to search
- `out_value`: Output parameter for value

**Returns**: `true` if found, `false` otherwise

**Example**:
```cpp
int value;
if (tree.search(42, value)) {
    std::cout << "Found: " << value << std::endl;
} else {
    std::cout << "Not found" << std::endl;
}
```

---

### root_offset()

```cpp
std::uint64_t root_offset() const
```

**Returns**: Offset of root node (for GC and recovery)

---

## GarbageCollector API

**Header**: `garbage_collector.h`  
**Purpose**: Crash recovery and memory leak detection

### Constructor

```cpp
GarbageCollector(Manager* manager)
```

**Parameters**:
- `manager`: Pointer to Manager instance

---

### collect()

```cpp
void collect(std::uint64_t root_offset)
```

**Parameters**:
- `root_offset`: Root node offset from `tree.root_offset()`

**Behavior**:
- **Mark Phase**: DFS traversal from root, marks reachable nodes
- **Sweep Phase**: Frees allocated but unreachable blocks (shadow nodes)

**Example**:
```cpp
GarbageCollector gc(&manager);
gc.collect(tree.root_offset());

std::cout << "Marked: " << gc.nodes_marked() << std::endl;
std::cout << "Freed: " << gc.blocks_freed() << std::endl;
```

---

### Statistics

```cpp
int nodes_marked() const;  // Number of reachable nodes
int blocks_freed() const;  // Number of leaked blocks recovered
```

---

## Primitives API

**Header**: `primitives.h`  
**Purpose**: Cache flush and persistence operations

### pmem_flush()

```cpp
void pmem_flush(void* addr, std::size_t len)
```

**Parameters**:
- `addr`: Start address
- `len`: Length in bytes

**Behavior**: Flushes cache lines covering `[addr, addr+len)` to NVM

---

### pmem_fence()

```cpp
void pmem_fence()
```

**Behavior**: Memory barrier (`SFENCE`), ensures all prior flushes complete

---

### persist()

```cpp
void persist(void* addr, std::size_t len)
```

**Behavior**: `pmem_flush()` + `pmem_fence()` (recommended for most use cases)

**Example**:
```cpp
Node* node = allocate_node();
// ... populate node ...
persist(node, sizeof(Node));  // Ensure durability
```

---

### atomic_pointer_swap()

```cpp
void atomic_pointer_swap(std::uint64_t* addr, 
                         std::uint64_t new_value, 
                         std::uint64_t* out_old_value)
```

**Parameters**:
- `addr`: Pointer location to update
- `new_value`: New offset value
- `out_old_value`: Receives old value

**Behavior**:
- Atomically exchanges 8-byte value
- Automatically persists the update

**Example**:
```cpp
uint64_t old_child;
atomic_pointer_swap(&parent->child_ptr, new_child_offset, &old_child);
```

---

## Complete Example: Database with Recovery

```cpp
#include "manager.h"
#include "B_tree.h"
#include "garbage_collector.h"
#include <iostream>

void create_database() {
    // Create new database
    Manager mgr("production.dat", 2ULL*1024*1024*1024, 256, true);
    
    BTreeConfig config;
    config.max_keys = 32;
    config.min_keys = 16;
    config.leaf_capacity = 64;
    
    BTree tree(&mgr, config);
    
    // Insert 1 million records
    for (int i = 0; i < 1000000; i++) {
        tree.insert(i, i * 100);
    }
    
    std::cout << "Database created with 1M records" << std::endl;
}

void recover_and_query() {
    // Reopen after crash
    Manager mgr("production.dat", 2ULL*1024*1024*1024, 256, false);
    
    BTreeConfig config;
    config.max_keys = 32;
    config.min_keys = 16;
    config.leaf_capacity = 64;
    
    BTree tree(&mgr, config);
    
    // Run GC for crash recovery
    GarbageCollector gc(&mgr);
    gc.collect(tree.root_offset());
    
    if (gc.blocks_freed() > 0) {
        std::cout << "Recovered " << gc.blocks_freed() 
                  << " leaked blocks from crash" << std::endl;
    }
    
    // Verify data
    int value;
    if (tree.search(500000, value)) {
        std::cout << "Data intact: key 500000 -> " << value << std::endl;
    }
}

int main() {
    create_database();
    
    // Simulate crash...
    
    recover_and_query();
    return 0;
}
```

---

## Best Practices

### 1. Always Run GC on Startup

```cpp
Manager mgr("db.dat", SIZE, BLOCK_SIZE, false);
BTree tree(&mgr, config);

GarbageCollector gc(&mgr);
gc.collect(tree.root_offset());  // Clean up crashed transactions
```

### 2. Use Appropriate Block Size

| Block Size | Use Case |
|-----------|----------|
| 128 bytes | Small nodes, minimal overhead |
| 256 bytes | **Recommended** (balances waste and efficiency) |
| 512 bytes | Large leaf nodes |

### 3. Tune Tree Configuration

```cpp
// For sequential inserts (high locality)
BTreeConfig seq_config;
seq_config.max_keys = 32;        // Larger nodes
seq_config.leaf_capacity = 128;  // More entries per leaf

// For random inserts (low locality)
BTreeConfig rand_config;
rand_config.max_keys = 16;       // Smaller nodes
rand_config.leaf_capacity = 32;  // Fewer entries
```

### 4. Persist Critical Updates

```cpp
// After modifying metadata
persist(&root_ptr, sizeof(root_ptr));

// After bulk operations
tree.insert(key, value);
persist(&tree, sizeof(tree));  // Optional: explicit flush
```

---

## Thread Safety

⚠️ **Current Version**: Not thread-safe  
**Recommendation**: Use external locking if accessing from multiple threads

**Future Enhancement**: Lock-free operations using CAS (Compare-And-Swap)

---

## Error Handling

All APIs throw `std::runtime_error` on fatal errors:

```cpp
try {
    uint64_t offset = mgr.alloc_block();
} catch (const std::runtime_error& e) {
    std::cerr << "Allocation failed: " << e.what() << std::endl;
}
```

**Common Errors**:
- `"Out of memory"`: Bitmap exhausted
- `"Failed to create/open file"`: Invalid path or permissions
- `"Failed to map file"`: Insufficient virtual address space

---

## Performance Tips

1. **Batch Inserts**: Insert sequentially when possible
2. **Pre-allocate**: Use larger `region_size` upfront to avoid resizing
3. **Tune `leaf_capacity`**: Larger = fewer splits, but more waste
4. **Monitor GC**: If `blocks_freed()` is high, investigate crash causes
5. **Use Sequential Keys**: Better cache locality and fewer splits
