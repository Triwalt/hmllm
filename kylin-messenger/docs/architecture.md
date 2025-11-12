# Kylin Messenger 架构文档

本文档使用Mermaid图表描述Kylin Messenger的程序结构和架构设计。

重要更新：自 1.2 起，协议层已统一为 IPMSG/FeiQ 文本协议；历史自定义二进制协议与 ProtoBuf 编解码路径已移除，仅保留 IPMSG 语义与实现。

## 系统整体架构

```mermaid
graph TB
    subgraph "应用层 (Application Layer)"
        Main[main.cpp<br/>应用入口]
        MainWindow[MainWindow<br/>主窗口]
        ChatWindow[ChatWindow<br/>聊天窗口]
    end
    
    subgraph "核心层 (Core Layer)"
        Models[Models<br/>数据模型]
        Logging[Logging<br/>日志系统]
        ServiceLocator[ServiceLocator<br/>服务定位器]
        MessageRepo[MessageRepository<br/>消息仓储]
    end
    
    subgraph "网络层 (Network Layer)"
        NetworkManager[NetworkManager<br/>网络管理器]
        Protocol[Protocol<br/>协议定义]
        IPMsg[IPMsg<br/>IP Messenger协议]
        AsyncTransport[AsyncTransport<br/>异步传输]
        GatewayClient[GatewayClient<br/>网关客户端]
    end
    
    subgraph "AI服务层 (AI Service Layer)"
        AIFactory[AIServiceFactory<br/>服务工厂]
        AIService[IAIService<br/>AI服务接口]
        EchoService[AIEchoService<br/>Echo服务]
        ComplianceService[ComplianceService<br/>合规服务]
        NSFWService[NSFWComplianceService<br/>NSFW检测]
        RKNNService[RKNNNSFWService<br/>RKNN加速检测]
    end
    
    subgraph "外部依赖 (External Dependencies)"
        Qt[Qt Framework<br/>Qt6/Qt5]
        RKNN[RKNN Runtime<br/>NPU运行时]
        LibLLM[libllm-rknn<br/>LLM推理库]
        Python[Python Runtime<br/>NSFW模型推理]
    end
    
    Main --> MainWindow
    MainWindow --> ChatWindow
    MainWindow --> NetworkManager
    MainWindow --> ServiceLocator
    ChatWindow --> NetworkManager
    ChatWindow --> AIFactory
    
    NetworkManager --> Protocol
    NetworkManager --> IPMsg
    NetworkManager --> AsyncTransport
    NetworkManager --> GatewayClient
    NetworkManager --> Models
    
    Protocol --> Models
    IPMsg --> Protocol
    AsyncTransport --> Protocol
    GatewayClient --> Protocol
    
    ServiceLocator --> AIFactory
    ServiceLocator --> MessageRepo
    
    AIFactory --> AIService
    EchoService -.实现.-> AIService
    ComplianceService -.实现.-> AIService
    NSFWService --> ComplianceService
    RKNNService --> ComplianceService
    
    MainWindow --> Qt
    ChatWindow --> Qt
    NetworkManager --> Qt
    
    RKNNService --> RKNN
    AIService --> LibLLM
    NSFWService --> Python
    
    MessageRepo --> Models
    Logging --> Models
    
    style Main fill:#e1f5ff
    style AIService fill:#fff9c4
    style Protocol fill:#c8e6c9
    style Models fill:#f8bbd0
```

## 分层架构详细视图

```mermaid
graph LR
    subgraph "表示层 (Presentation)"
        UI1[MainWindow]
        UI2[ChatWindow]
        UI3[SettingsDialog]
    end
    
    subgraph "业务逻辑层 (Business Logic)"
        BL1[NetworkManager]
        BL2[AIServiceFactory]
        BL3[ServiceLocator]
        BL4[MessageRepository]
    end
    
    subgraph "数据访问层 (Data Access)"
        DA1[InMemoryMessageRepository]
        DA2[UserInfoCache]
        DA3[FileTransferManager]
    end
    
    subgraph "基础设施层 (Infrastructure)"
        INF1[Protocol Layer]
        INF2[Transport Layer]
        INF3[AI Service Interface]
        INF4[Logging System]
    end
    
    UI1 --> BL1
    UI1 --> BL3
    UI2 --> BL1
    UI2 --> BL2
    
    BL1 --> DA1
    BL1 --> DA2
    BL1 --> DA3
    BL2 --> INF3
    BL3 --> BL2
    BL3 --> BL4
    BL4 --> DA1
    
    BL1 --> INF1
    BL1 --> INF2
    INF1 --> INF2
    
    INF4 -.-> BL1
    INF4 -.-> BL2
    INF4 -.-> UI1
```

## 核心模块类图

```mermaid
classDiagram
    class UserInfo {
        +QString user_id
        +QString username
        +QString ip_address
        +quint16 port
        +UserStatus status
        +QByteArray serialize()
        +bool deserialize()
    }
    
    class ChatMessage {
        +QString message_id
        +QString sender_id
        +QString receiver_id
        +MessageContentType type
        +QString content
        +QDateTime timestamp
        +QList~FileAttachment~ attachments
        +QByteArray serialize()
        +bool deserialize()
    }
    
    class FileAttachment {
        +QString filename
        +QString filepath
        +quint64 filesize
        +QString file_hash
    }
    
    class NetworkPacket {
        -PacketHeader header_
        -QByteArray payload_
        +setPayload()
        +QByteArray serialize()
        +bool deserialize()
        +bool isValid()
    }
    
    class PacketHeader {
        +quint32 magic_number
        +quint16 version
        +MessageType type
        +quint32 payload_size
        +quint32 checksum
        +QByteArray serialize()
        +bool deserialize()
    }
    
    class NetworkManager {
        -QUdpSocket udp_socket
        -QTcpServer tcp_server
        -QMap~QString,UserInfo~ users
        +sendMessage()
        +broadcastPresence()
        +handleIncomingPacket()
        +discoverUsers()
    }
    
    ChatMessage --> FileAttachment : contains
    NetworkPacket --> PacketHeader : has
    NetworkPacket --> ChatMessage : encapsulates
    NetworkPacket --> UserInfo : encapsulates
    NetworkManager --> NetworkPacket : uses
    NetworkManager --> UserInfo : manages
```

## AI服务架构

```mermaid
classDiagram
    class IAIService {
        <<interface>>
        +initialize(model_path) bool
        +shutdown() void
        +isReady() bool
        +getName() string
        +getCapabilities() AICapability
        +processText(input) AIResult
        +processTextStream(input, callback) AIResult
        +processImage(image) AIResult
        +generateSmartReplies(history) AIResult
        +analyzeContent(content) AIResult
    }
    
    class AIResult {
        +bool success
        +float confidence
        +string error_message
        +string text_output
        +vector~string~ suggestions
        +QVariantMap metadata
    }
    
    class AIServiceFactory {
        +createService(type) unique_ptr~IAIService~
        +registerService(name, creator) void
        +listAvailableServices() vector~string~
    }
    
    class AIEchoService {
        +initialize(path) bool
        +processText(input) AIResult
        +getName() string
    }
    
    class ComplianceService {
        <<interface>>
        +checkText(content) ComplianceResult
        +checkImage(image) ComplianceResult
        +getBackend() string
    }
    
    class NSFWComplianceService {
        -QString python_path
        -QString model_path
        +initialize(path) bool
        +processImage(image) AIResult
        -invokePythonChecker() ComplianceResult
    }
    
    class RKNNNSFWComplianceService {
        -rknn_context ctx
        -void* model_data
        +initialize(path) bool
        +processImage(image) AIResult
        -loadRKNNModel() bool
        -runInference() ComplianceResult
    }
    
    IAIService <|.. AIEchoService : implements
    IAIService <|.. ComplianceService : implements
    ComplianceService <|.. NSFWComplianceService : implements
    ComplianceService <|.. RKNNNSFWComplianceService : implements
    
    AIServiceFactory --> IAIService : creates
    IAIService --> AIResult : returns
```

## 网络通信时序图

```mermaid
sequenceDiagram
    participant User1 as 用户1 客户端
    participant NM1 as NetworkManager1
    participant UDP as UDP广播
    participant NM2 as NetworkManager2
    participant User2 as 用户2 客户端
    participant TCP as TCP连接
    
    Note over User1,User2: 用户发现阶段
    User1->>NM1: 启动应用
    NM1->>UDP: 广播 UserPresence
    UDP->>NM2: 接收广播
    NM2->>User2: 显示新用户
    NM2->>UDP: 回复 UserPresence
    UDP->>NM1: 接收回复
    NM1->>User1: 显示用户2
    
    Note over User1,User2: 消息发送阶段
    User1->>NM1: 发送消息给用户2
    NM1->>NM1: 创建 ChatMessage
    NM1->>NM1: 封装为 NetworkPacket
    NM1->>TCP: 建立TCP连接
    TCP->>NM2: 连接成功
    NM1->>TCP: 发送数据包
    TCP->>NM2: 接收数据包
    NM2->>NM2: 解析 NetworkPacket
    NM2->>NM2: 验证校验和
    NM2->>User2: 显示消息
    NM2->>TCP: 发送ReadReceipt
    TCP->>NM1: 接收回执
    NM1->>User1: 标记已读
```

## 消息处理流程

```mermaid
flowchart TD
    Start([用户发送消息]) --> Input[获取输入内容]
    Input --> CheckAI{启用AI功能?}
    
    CheckAI -->|是| AICheck[内容合规检查]
    CheckAI -->|否| CreateMsg[创建ChatMessage]
    
    AICheck --> ComplianceOK{通过审核?}
    ComplianceOK -->|是| CreateMsg
    ComplianceOK -->|否| Reject[拒绝发送并提示]
    Reject --> End([结束])
    
    CreateMsg --> Serialize[序列化为NetworkPacket]
    Serialize --> CalcChecksum[计算CRC32校验和]
    CalcChecksum --> SelectTransport{目标类型?}
    
    SelectTransport -->|单聊| TCP[TCP点对点传输]
    SelectTransport -->|群聊| MultiTCP[多个TCP连接]
    SelectTransport -->|广播| UDP[UDP广播]
    
    TCP --> SendPacket[发送数据包]
    MultiTCP --> SendPacket
    UDP --> SendPacket
    
    SendPacket --> WaitReceipt{需要回执?}
    WaitReceipt -->|是| WaitAck[等待确认]
    WaitReceipt -->|否| UpdateUI[更新UI]
    
    WaitAck --> Timeout{超时?}
    Timeout -->|是| Retry{重试次数<3?}
    Timeout -->|否| UpdateUI
    
    Retry -->|是| SendPacket
    Retry -->|否| Error[标记发送失败]
    Error --> End
    
    UpdateUI --> SaveRepo[保存到消息仓储]
    SaveRepo --> End
```

## 文件传输流程

```mermaid
flowchart TD
    Start([用户选择文件]) --> CalcHash[计算文件哈希]
    CalcHash --> CreateOffer[创建FileOffer包]
    CreateOffer --> SendOffer[发送到接收方]
    
    SendOffer --> WaitResponse{等待响应}
    WaitResponse -->|接受| StartTransfer[开始传输]
    WaitResponse -->|拒绝| Cancel1[取消传输]
    WaitResponse -->|超时| Cancel1
    
    Cancel1 --> End([结束])
    
    StartTransfer --> OpenFile[打开文件]
    OpenFile --> ReadChunk[读取64KB数据块]
    
    ReadChunk --> HasMore{还有数据?}
    HasMore -->|是| CreateDataPacket[创建FileTransferData包]
    HasMore -->|否| SendComplete[发送完成标记]
    
    CreateDataPacket --> SendChunk[发送数据块]
    SendChunk --> WaitAck[等待ACK]
    
    WaitAck --> AckOK{收到确认?}
    AckOK -->|是| Progress[更新进度]
    AckOK -->|否| Retry{重试?}
    
    Retry -->|是| SendChunk
    Retry -->|否| ErrorHandle[错误处理]
    
    ErrorHandle --> End
    Progress --> ReadChunk
    
    SendComplete --> Verify[接收方验证哈希]
    Verify --> VerifyOK{哈希匹配?}
    
    VerifyOK -->|是| Success[传输成功]
    VerifyOK -->|否| Error[传输失败]
    
    Success --> End
    Error --> End
```

## 依赖注入架构

```mermaid
graph TD
    subgraph "ServiceLocator (服务容器)"
        SL[ServiceLocator Instance]
    end
    
    subgraph "已注册服务"
        NM[NetworkManager]
        AF[AIServiceFactory]
        MR[MessageRepository]
        CS[ComplianceService]
        Logger[LoggingService]
    end
    
    subgraph "服务消费者"
        MW[MainWindow]
        CW[ChatWindow]
        NP[NetworkProtocol]
    end
    
    SL --> NM
    SL --> AF
    SL --> MR
    SL --> CS
    SL --> Logger
    
    MW -.获取.-> SL
    CW -.获取.-> SL
    NP -.获取.-> SL
    
    MW --> NM
    MW --> MR
    CW --> AF
    CW --> NM
    NP --> CS
    
    style SL fill:#4fc3f7
    style MW fill:#81c784
    style CW fill:#81c784
```

## 数据流图

```mermaid
graph LR
    subgraph "用户输入"
        UI[用户界面]
    end
    
    subgraph "数据处理"
        Validation[输入验证]
        AIProcess[AI处理]
        Serialization[序列化]
    end
    
    subgraph "传输层"
        Protocol[协议封装]
        Network[网络传输]
    end
    
    subgraph "存储层"
        Cache[内存缓存]
        Repo[消息仓储]
    end
    
    subgraph "对端"
        Remote[远程客户端]
    end
    
    UI -->|原始输入| Validation
    Validation -->|验证通过| AIProcess
    AIProcess -->|AI增强| Serialization
    Serialization -->|二进制数据| Protocol
    Protocol -->|网络包| Network
    
    Network -->|发送| Remote
    Network -->|本地副本| Cache
    Cache -->|持久化| Repo
    
    Remote -->|回执| Network
    Network -->|状态更新| Cache
    Cache -->|同步| Repo
    Repo -->|查询| UI
```

## 编译构建流程

```mermaid
graph TD
    Start([CMake配置]) --> DetectQt{检测Qt版本}
    DetectQt -->|Qt6| UseQt6[使用Qt6]
    DetectQt -->|Qt5| UseQt5[回退Qt5]
    
    UseQt6 --> CheckWebSocket{WebSocket可用?}
    UseQt5 --> CheckWebSocket
    
    CheckWebSocket -->|是| EnableWS[启用网关功能]
    CheckWebSocket -->|否| DisableWS[禁用网关功能]
    
    EnableWS --> CheckAI{AI功能启用?}
    DisableWS --> CheckAI
    
    CheckAI -->|否| BuildCore[构建核心库]
    CheckAI -->|是| DetectPlatform{检测平台}
    
    DetectPlatform -->|ARM/Linux| FindRKNN[查找RKNN运行时]
    DetectPlatform -->|x86/Windows| SkipRKNN[跳过RKNN]
    
    FindRKNN -->|找到| LinkRKNN[链接RKNN库]
    FindRKNN -->|未找到| Warn1[警告: AI功能受限]
    
    LinkRKNN --> FindLLM{查找libllm-rknn?}
    Warn1 --> FindLLM
    SkipRKNN --> BuildCore
    
    FindLLM -->|找到| LinkLLM[链接LLM库]
    FindLLM -->|未找到| Warn2[警告: LLM不可用]
    
    LinkLLM --> BuildCore
    Warn2 --> BuildCore
    
    BuildCore --> AutoMOC[Qt MOC自动生成]
    AutoMOC --> AutoRCC[资源文件编译]
    AutoRCC --> AutoUIC[UI文件编译]
    
    AutoUIC --> CompileCore[编译核心库]
    CompileCore --> CompileMain[编译主程序]
    CompileMain --> LinkExe[链接可执行文件]
    
    LinkExe --> BuildTests{构建测试?}
    BuildTests -->|是| CompileTests[编译测试]
    BuildTests -->|否| Package
    
    CompileTests --> RunTests[运行测试]
    RunTests --> Package[生成安装包]
    
    Package --> End([构建完成])
```

## 部署架构

```mermaid
graph TB
    subgraph "开发板硬件"
        CPU[ARM CPU<br/>4核心]
        NPU[RKNN NPU<br/>AI加速器]
        RAM[内存<br/>2GB+]
        Storage[存储<br/>eMMC/SD卡]
    end
    
    subgraph "操作系统层"
        Kernel[Linux Kernel<br/>麒麟操作系统]
        Drivers[驱动程序<br/>RKNN Driver]
    end
    
    subgraph "运行时环境"
        Qt6Runtime[Qt6 Runtime<br/>libQt6*.so]
        RKNNRuntime[RKNN Runtime<br/>librknnrt.so]
        LLMLib[LLM库<br/>libllm-rknn.so]
        PythonRuntime[Python 3.x<br/>NSFW检测]
    end
    
    subgraph "应用层"
        KylinMessenger[kylin-messenger<br/>可执行文件]
        CoreLib[libkylin-messenger-core.a<br/>核心库]
        Models[AI模型文件<br/>*.rknn, *.h5]
        Resources[资源文件<br/>图标/主题]
    end
    
    subgraph "用户界面"
        Desktop[桌面环境<br/>UKUI/DDE]
        AppIcon[应用图标<br/>.desktop]
    end
    
    CPU --> Kernel
    NPU --> Kernel
    RAM --> Kernel
    Storage --> Kernel
    
    Kernel --> Drivers
    Drivers --> RKNNRuntime
    
    Kernel --> Qt6Runtime
    Kernel --> PythonRuntime
    
    RKNNRuntime --> LLMLib
    
    Qt6Runtime --> KylinMessenger
    RKNNRuntime --> KylinMessenger
    LLMLib --> KylinMessenger
    PythonRuntime --> KylinMessenger
    
    CoreLib --> KylinMessenger
    Models --> KylinMessenger
    Resources --> KylinMessenger
    
    KylinMessenger --> Desktop
    AppIcon --> Desktop
    
    style NPU fill:#ffeb3b
    style RKNNRuntime fill:#ffeb3b
    style KylinMessenger fill:#4caf50
```

## 项目目录结构

```mermaid
graph TD
    Root[kylin-messenger/] --> Include[include/<br/>头文件]
    Root --> Src[src/<br/>源代码]
    Root --> Tests[tests/<br/>单元测试]
    Root --> Resources[resources/<br/>资源文件]
    Root --> UI[ui/<br/>UI定义文件]
    Root --> Debian[debian/<br/>打包配置]
    Root --> CMake[CMakeLists.txt<br/>构建配置]
    
    Include --> IncCore[core/<br/>核心模块]
    Include --> IncNetwork[network/<br/>网络模块]
    Include --> IncAI[ai_service.h等<br/>AI接口]
    Include --> IncUtils[utils/<br/>工具类]
    
    Src --> SrcCore[core/<br/>核心实现]
    Src --> SrcNetwork[network/<br/>网络实现]
    Src --> SrcUI[ui/<br/>UI实现]
    Src --> SrcServices[services/<br/>服务实现]
    Src --> SrcMain[main.cpp<br/>程序入口]
    
    SrcCore --> Models[models.cpp<br/>数据模型]
    SrcCore --> Logging[logging.cpp<br/>日志系统]
    SrcCore --> DI[di/service_locator.cpp<br/>依赖注入]
    SrcCore --> Repo[repositories/<br/>仓储层]
    
    SrcNetwork --> IPMsg[ipmsg.cpp<br/>协议实现]
    SrcNetwork --> NetMgr[network_manager.cpp<br/>网络管理]
    SrcNetwork --> Transport[async_transport.cpp<br/>传输层]
    SrcNetwork --> Gateway[gateway_client.cpp<br/>网关客户端]
    
    SrcUI --> MainWin[main_window.cpp<br/>主窗口]
    SrcUI --> ChatWin[chat_window.cpp<br/>聊天窗口]
    
    SrcServices --> Compliance[compliance服务<br/>内容审核]
    
    Resources --> Icons[icons/<br/>图标资源]
    Resources --> Emojis[emojis/<br/>表情包]
    Resources --> QSS[qss/<br/>样式表]
    Resources --> Scripts[scripts/<br/>脚本资源]
    
    style Root fill:#e3f2fd
    style Include fill:#fff3e0
    style Src fill:#f1f8e9
    style Resources fill:#fce4ec
```

---

## 说明

以上Mermaid图表从多个维度展示了Kylin Messenger的架构设计：

1. **系统整体架构**: 展示应用、核心、网络、AI四大层次及其依赖关系
2. **分层架构**: 体现表示层、业务逻辑层、数据访问层、基础设施层的分层设计
3. **核心模块类图**: 详细展示关键数据结构和类的关系
4. **AI服务架构**: 展示AI服务的抽象接口和具体实现的继承关系
5. **网络通信时序图**: 描述用户发现和消息传递的完整流程
6. **消息处理流程**: 展示消息从输入到发送的完整处理流程
7. **文件传输流程**: 描述文件传输的详细步骤
8. **依赖注入架构**: 展示ServiceLocator模式的应用
9. **数据流图**: 展示数据在各层之间的流动
10. **编译构建流程**: 描述CMake构建过程的决策树
11. **部署架构**: 展示从硬件到应用的完整部署栈
12. **项目目录结构**: 展示源代码的组织结构

这些图表可以使用支持Mermaid的Markdown查看器（如GitHub、GitLab、Typora、VSCode等）查看。

