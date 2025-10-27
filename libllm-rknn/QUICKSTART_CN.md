# 快速入门指南 - libllm-rknn

在5分钟内开始使用libllm-rknn！

## 前提条件

您需要:
- 带有NPU的瑞芯微开发板(RK3588、RK3576等)
- 板上运行Linux系统
- 已安装RKNN运行时
- 已转换的`.rknn`模型文件
- 分词器模型文件

## 步骤1: 安装依赖

```bash
# 安装构建工具
sudo apt-get update
sudo apt-get install build-essential cmake git

# 安装SentencePiece
sudo apt-get install libsentencepiece-dev

# 验证RKNN运行时已安装
ldconfig -p | grep rknnrt
```

## 步骤2: 编译库

```bash
# 克隆仓库
git clone https://github.com/yourusername/libllm-rknn.git
cd libllm-rknn

# 编译
chmod +x build.sh
./build.sh

# 安装
cd build
sudo make install
sudo ldconfig
```

## 步骤3: 获取模型

您需要一个RKNN转换的LLM模型。如果没有:

```bash
# 示例: 下载预转换模型(如果可用)
# 或使用RKNN-Toolkit2在主机PC上转换您自己的模型

# 您应该有:
# - model.rknn (转换后的模型)
# - tokenizer.model (SentencePiece分词器)
```

## 步骤4: 运行首次推理

```bash
# 交互模式
./build/bin/llm_demo -m /path/to/model.rknn -t /path/to/tokenizer.model

# 单次提示模式
./build/bin/llm_demo \
    -m /path/to/model.rknn \
    -t /path/to/tokenizer.model \
    -p "给我讲个笑话"
```

## 步骤5: 编写您自己的应用

创建 `my_app.cpp`:

```cpp
#include "llm_rknn.h"
#include <stdio.h>

// 回调函数接收每个生成的token
int token_callback(const char* text, int32_t id, void* data) {
    printf("%s", text);
    fflush(stdout);
    return 0;  // 返回0继续，非零值停止
}

int main() {
    // 1. 初始化
    llm_rknn_handle_t llm = llm_rknn_init(
        "model.rknn",
        "tokenizer.model",
        NULL  // 使用默认值
    );
    
    if (!llm) {
        fprintf(stderr, "初始化失败!\n");
        return 1;
    }
    
    printf("初始化成功!\n\n");
    
    // 2. 生成文本
    printf("助手: ");
    int ret = llm_rknn_generate(
        llm,
        "什么是人工智能?",
        token_callback,
        NULL
    );
    
    printf("\n\n");
    
    // 3. 检查结果
    if (ret == LLM_RKNN_SUCCESS) {
        llm_rknn_perf_stats_t stats;
        llm_rknn_get_perf_stats(llm, &stats);
        printf("在%.2f毫秒内生成了%u个token (%.2f token/秒)\n",
               stats.total_time_ms,
               stats.total_tokens_generated,
               stats.tokens_per_second);
    } else {
        fprintf(stderr, "错误: %s\n", llm_rknn_get_error_string(ret));
    }
    
    // 4. 清理
    llm_rknn_release(llm);
    
    return 0;
}
```

编译并运行:

```bash
# 编译
g++ -o my_app my_app.cpp -lllm-rknn

# 运行
./my_app
```

## 高级示例: 自定义配置

```cpp
#include "llm_rknn.h"
#include <stdio.h>

int callback(const char* text, int32_t id, void* data) {
    printf("%s", text);
    fflush(stdout);
    return 0;
}

int main() {
    // 创建自定义配置
    llm_rknn_config_t config;
    llm_rknn_init_default_config(&config);
    
    // 自定义生成参数
    config.max_new_tokens = 200;              // 生成最多200个token
    config.sampling_method = LLM_RKNN_SAMPLING_TOP_K;  // 使用Top-K
    config.temperature = 0.8f;                // 更有创意
    config.top_k = 40;                        // Top-40采样
    config.rknn_core_mask = 3;                // 使用所有NPU核心
    
    // 使用自定义配置初始化
    llm_rknn_handle_t llm = llm_rknn_init(
        "model.rknn",
        "tokenizer.model",
        &config
    );
    
    if (!llm) {
        fprintf(stderr, "初始化失败!\n");
        return 1;
    }
    
    // 使用创意设置生成
    llm_rknn_generate(llm, "写一个创意故事:", callback, NULL);
    
    llm_rknn_release(llm);
    return 0;
}
```

## 常见采样配置

### 用于事实问答(确定性)

```c
config.sampling_method = LLM_RKNN_SAMPLING_GREEDY;
config.temperature = 1.0f;
```

### 用于创意写作(多样化)

```c
config.sampling_method = LLM_RKNN_SAMPLING_TOP_K_TOP_P;
config.temperature = 0.8f;
config.top_k = 40;
config.top_p = 0.95f;
```

### 用于平衡输出

```c
config.sampling_method = LLM_RKNN_SAMPLING_TOP_P;
config.temperature = 0.7f;
config.top_p = 0.9f;
```

## 故障排除

**问题**: 运行时找不到库

```bash
# 解决方案:
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
# 或使用ldconfig正确安装
```

**问题**: RKNN运行时错误

```bash
# 检查NPU驱动是否加载:
dmesg | grep rknpu

# 检查RKNN库:
ldconfig -p | grep rknnrt
```

**问题**: 推理慢

- 启用KV缓存: `config.enable_kv_cache = true;`
- 使用贪婪采样以提高速度
- 确保模型已量化(int8或fp16)

## 下一步

- **完整文档**: 阅读 [README_CN.md](README_CN.md)
- **构建指南**: 查看 [docs/BUILD_CN.md](docs/BUILD_CN.md)
- **API参考**: 检查 [include/llm_rknn.h](include/llm_rknn.h)
- **示例**: 探索 [examples/](examples/)

## 需要帮助?

- 在GitHub上提出issue
- 查看现有文档
- 查看示例代码

---

**祝编码愉快! 🚀**
