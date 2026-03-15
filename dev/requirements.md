# Timeline Resource Engine
## Requirements Specification

## 1. Project Overview

This project implements a **timeline-based multimodal resource engine** that stores, manages, and queries system resources based on time.

The system provides a centralized resource service that allows agents to access historical and real-time data from multiple sources such as vehicle signals, camera streams, logs, and other runtime resources.

The project produces two deliverables:

1. **resource-server**
   - A standalone Linux process
   - Provides resource query and management services

2. **libtimelineclient**
   - A client library for agents
   - Distributed as:
     - C/C++ header files
     - Static or shared library

Agents link against the client library to communicate with the resource server.

---

# 2. System Architecture

The system follows a **client-server architecture**.

```
+--------------------+
|        Agent       |
| (links client lib) |
+---------+----------+
          |
          | Client API
          |
+---------v----------+
|  libtimelineclient |
+---------+----------+
          |
          | IPC / gRPC / HTTP
          |
+---------v----------+
|   resource-server  |
+---------+----------+
          |
   +------+------+
   |             |
Timeline Cache   Persistence
   |             |
Trigger Engine   Resource Adapters
```

---

# 3. Design Principles

### 3.1 Hardware Independence

The system must be **hardware independent**.

Core components must not depend on:

- specific SoC platforms
- hardware-specific SDKs
- vendor-specific drivers

All hardware-specific logic must be implemented through **adapter interfaces**.

Example adapters:

- Camera Adapter
- Signal Adapter
- Shared Memory Adapter
- Storage Adapter

Default implementations must rely on **standard Linux mechanisms** such as:

- POSIX APIs
- file systems
- mmap
- Unix domain sockets

---

### 3.2 Linux Platform

The system is designed to run on **Linux environments only**.

Supported toolchain requirements:

- GCC / Clang
- CMake build system
- POSIX APIs

Optional runtime environments:

- native host
- containerized (Docker)

---

### 3.3 Client-Server Decoupling

The system must allow agents to interact with the service without knowing the server deployment details.

The client library must:

- abstract communication protocols
- automatically select connection method

Preferred connection order:

1. Unix Domain Socket (local)
2. gRPC
3. HTTP/JSON

---

# 4. Deliverables

## 4.1 resource-server

Standalone executable process.

Responsibilities:

- resource ingestion
- timeline storage
- query processing
- trigger evaluation
- persistence management

Server provides:

- gRPC API
- HTTP/REST API
- Unix domain socket IPC

Example binary:

```
bin/resource-server
```

Configuration file example:

```
config/server.yaml
```

---

## 4.2 Client Library

Library distributed for agent integration.

Contents:

```
include/
    timeline_client.h

lib/
    libtimelineclient.a
    libtimelineclient.so
```

Responsibilities:

- connect to resource-server
- send query requests
- subscribe to events
- manage shared resource handles

---

# 5. Functional Requirements

## 5.1 Resource Types

The system must support multiple resource types:

- Vehicle signals
- Camera frames
- Audio segments
- Screen captures
- System logs
- Custom plugin resources

Each resource must include:

```
timestamp
resource_type
source_id
metadata
data_reference
```

Large binary data should be referenced using **handles**, not copied.

---

## 5.2 Timeline Cache

The system maintains a **timeline-based in-memory cache**.

Capabilities:

- time-indexed storage
- fast retrieval
- configurable retention window
- eviction policies

Supported queries:

- latest state
- point-in-time lookup
- time-range query
- filtered query

---

## 5.3 Query Interfaces

The system must support the following query methods:

### 5.3.1 Time Range Query

Retrieve resources within a time interval.

Example:

```
query(start_ts, end_ts)
```

### 5.3.2 Latest State Query

Retrieve the most recent value for a resource.

### 5.3.3 Filtered Query

Filter by:

- resource type
- resource id
- metadata

---

## 5.4 Trigger System

The system supports **event triggers**.

Triggers are activated when conditions are satisfied.

Examples:

- signal value exceeds threshold
- specific event detected
- resource capture occurs

Triggers notify subscribed agents.

---

## 5.5 Checkpoints

Agents can create **checkpoints**.

Checkpoint features:

- mark important timestamps
- attach metadata
- enable later query

Example:

```
create_checkpoint("lane_change_event")
```

---

## 5.6 Persistence

Cache data may be persisted to disk.

Persistence triggers:

- cache size limit
- time interval
- checkpoint event

Persistence strategies:

- append-only storage
- periodic snapshot

Large resources (e.g., video frames) may use **sampling strategies**.

---

# 6. Client API Requirements

The client library must provide APIs for:

- server connection
- querying resources
- subscribing to triggers
- checkpoint creation
- memory handle management

Example API functions:

```
tl_init()
tl_shutdown()

tl_query_range()
tl_query_latest()

tl_subscribe()

tl_create_checkpoint()

tl_map_handle()
tl_unmap_handle()
```

---

# 7. Performance Goals

Performance targets are **reference goals**, not strict guarantees.

Under light workload:

- ≤ 5 resource types
- ≤ 100 signals

Expected latency:

| Operation | Target |
|--------|--------|
| Cache Query | ~10 ms |
| Persistence Query | ~100 ms |
| Signal Write | ~1 ms |
| Complex Resource Write | ~5 ms |

Actual performance depends on hardware.

Benchmark tests must document test environment.

---

# 8. Testing Strategy

The system must include:

### Unit Tests

Coverage targets:

- timeline cache
- query engine
- adapters
- client API

### Integration Tests

Tests must validate:

- client-server communication
- trigger propagation
- persistence workflow

### Benchmark Tests

Micro-benchmarks must measure:

- query latency
- write throughput
- memory usage

---

# 9. Build System

The project uses **CMake**.

Expected repository layout:

```
project/
│
├─ server/
├─ client/
├─ include/
├─ adapters/
├─ tests/
├─ benchmarks/
├─ config/
└─ docs/
```

Build artifacts:

```
resource-server
libtimelineclient.so
libtimelineclient.a
timeline_client.h
```

---

# 10. Security Considerations

The system must:

- avoid unnecessary data copying
- protect raw resource data
- allow access control in server configuration

Sensitive telemetry data should not be exposed by default.

---

# 11. Extensibility

The architecture must support:

- new resource types
- custom persistence backends
- new communication protocols
- additional trigger rules

All extensions must be implemented through **plugin or adapter interfaces**.

---

# 12. Acceptance Criteria

The project is considered complete when:

1. `resource-server` can run on Linux and accept client requests
2. `libtimelineclient` can connect to the server and perform queries
3. at least one resource provider is implemented
4. trigger subscription works end-to-end
5. persistence functionality is demonstrated
6. unit and integration tests run successfully
7. build and run instructions are documented
