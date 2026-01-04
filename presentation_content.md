# 面向智能座舱的多模态实时数据引擎设计与实现
## Design and Implementation of a Multimodal Real-Time Data Engine for Intelligent Cockpits

---

# Slide 1: 封面 (Title Slide)

**标题**: 面向智能座舱的多模态实时数据引擎设计与实现
**副标题**: 基于时间线的高性能车载数据中间件
**汇报人**: 邓杰 (Deng Jie)
**学号**: SA24225109
**日期**: 2025年12月
**合作方**: 博世 (Bosch) XC-CP

---

# Slide 2: 课题背景与痛点 (Background & Pain Points)

**行业趋势**:
- **E/E架构演进**: 从分布式向域集中式(Domain Centralized)及中央计算(Vehicle Computer)转变。
- **数据驱动**: AI Agent和大模型引入座舱，需求从"瞬时状态读取"转变为"历史时序推理"。

**现有痛点**:
1.  **数据碎片化**: Camera、Mic、CAN信号等异构数据缺乏统一的时间基准，难以对齐。
2.  **性能瓶颈**: 传统数据库(TSDB/SQL)在嵌入式环境(RAM/UFS受限)下无法满足高频写入(<5ms)和低延迟查询(<10ms)需求。
3.  **被动交互**: 缺乏主动感知机制，上层应用只能轮询查询。

---

# Slide 3: 项目目标 (Project Objectives)

**核心目标**:
构建一套**基于时间线(Timeline-based)**的多模态资源引擎，作为座舱软件架构的底层核心组件。

**关键指标**:
- **多模态融合**: 统一管理视频流、音频、车辆信号等异构数据。
- **高性能**: 
    - 写入延迟 < 1ms (信号) / 5ms (视频)
    - 查询响应 < 10ms
- **零拷贝**: 跨进程共享内存传输，避免内核态/用户态拷贝。
- **分级存储**: 内存(Hot Data) -> 持久化存储(Cold Data) 自动流转。

---

# Slide 4: 系统总体架构 (System Architecture)

*(建议插入 doc/for_ai.pdf 图10 或 doc/Bosch...pdf 中的架构图)*

**分层设计 (Layering)**:
1.  **Resources Layer (数据源)**: Camera, Audio, Vehicle Signals.
2.  **Core Engine (核心引擎)**: 
    - **TimelineCache**: 核心内存索引与缓存。
    - **SharedMemoryManager**: 零拷贝内存池管理。
    - **PersistenceEngine**: 负责刷盘与冷热数据交换。
3.  **Service Interface (服务接口)**: 提供 Query (查询) 和 Trigger (触发器) 接口。
4.  **Control Plane (控制面)**: 策略配置、生命周期管理。

---

# Slide 5: 核心技术 I - 时间线数据模型 (Timeline Data Model)

**创新点**: 专为时序数据设计的混合索引结构。

**结构设计**:
- **一级索引**: 红黑树 (Red-Black Tree) / AVL树
    - *作用*: 实现 O(log N) 的快速时间戳查找。
- **二级索引**: 双向链表 (Doubly Linked List)
    - *作用*: 优化范围查询 (Range Query) 和时序遍历，复杂度 O(K)。

**优势**:
- 相比传统 Hash Map，支持高效的范围检索 (如: "查询过去10秒的视频帧")。
- 相比纯数组，支持动态插入与内存碎片管理。

---

# Slide 6: 核心技术 II - 零拷贝与共享内存 (Zero-Copy & Shared Memory)

**机制**:
1.  **预分配内存池 (Slab Allocation)**: 针对视频帧等大对象，启动时预分配共享内存块。
2.  **句柄传递 (Handle Passing)**: 
    - 数据生产者(Provider)写入共享内存。
    - 仅将内存句柄(Handle/Offset)通过 IPC 发送给引擎。
    - 引擎建立索引，不移动物理数据。
    - 消费者(Agent)通过句柄直接读取。

**效果**:
- 消除大数据量下的 `memcpy` 开销，CPU 占用显著降低。

---

# Slide 7: 核心技术 III - 分级存储与持久化 (Hierarchical Storage)

**冷热数据分离**:
- **Hot Data (内存)**: 最近 N 秒的数据驻留 Ring Buffer，支持极速随机访问。
- **Cold Data (UFS)**: 
    - **Checkpoint (检查点)**: 触发关键事件(如急刹车、闭眼检测)时，自动标记关键时间窗数据。
    - **Sync Policy**: 异步线程将冷数据压缩并刷入磁盘，释放内存。

**智能抽帧**:
- 对非关键时段视频数据进行降采样(Downsampling)存储，节省存储空间。

---

# Slide 8: 核心技术 IV - 主动感知触发器 (Trigger Mechanism)

**从 "被动查询" 到 "主动通知"**:

**工作流**:
1.  **注册规则**: Agent 注册感兴趣的事件 (例: `Speed > 100` AND `Face == Drowsy`)。
2.  **状态监测**: 引擎在数据摄入时实时匹配规则。
3.  **事件触发**: 满足条件时，立即回调通知 Agent，并自动锁定相关历史数据(Checkpoint)。

**价值**:
- 降低上层应用轮询开销，提升系统响应实时性。

---

# Slide 9: 详细实现 (Implementation Details)

**技术栈**:
- **语言**: C++ 17/20 (高性能，RAII资源管理)
- **并发模型**: Reactor 模式 (I/O 线程与工作线程分离)
- **锁机制**: 
    - 核心路径采用 **无锁队列 (Lock-free Ring Buffer)** (SPSC)。
    - 时间桶分段锁 (Segmented Lock) 减少竞争。

**代码模块**:
- `TimelineCache`: 核心索引逻辑。
- `SharedMemoryManager`: `mmap` 封装与引用计数管理。
- `DataIngestionManager`: 数据摄入流水线。

---

# Slide 10: 性能评估 (Performance Evaluation)

*(基于 benchmark 数据)*

**测试环境**: Qualcomm QC8295 / Linux / C++ Benchmark

**结果摘要**:
1.  **写入延迟**: 
    - 信号数据: < 0.5ms
    - 4K视频帧: < 2ms (仅传递句柄)
2.  **查询响应**:
    - 单点查询: < 10µs
    - 范围查询(100ms窗口): < 1ms
3.  **并发能力**:
    - 支持 100+ 路 CAN 信号与 4 路高清视频并发写入，无阻塞。

---

# Slide 11: 总结与展望 (Conclusion & Future Work)

**总结**:
- 成功设计并实现了一个**高内聚、低耦合**的车载多模态数据引擎。
- 解决了异构数据对齐、高频写入瓶颈及跨进程高效传输三大难题。

**未来工作**:
- **数据压缩优化**: 引入更高效的时序数据压缩算法 (如 Gorilla)。
- **云端协同**: 实现边缘端数据与云端训练平台的无缝同步。
- **生态扩展**: 支持更多标准协议 (如 ROS2, DDS) 接入。

---

# Slide 12: 致谢 (Thank You)

**Q & A**

感谢导师指导与博世团队支持。
