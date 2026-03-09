# 楼梯检测算法文档

## 概述

这是一个用于 Unreal Engine 5 的楼梯检测算法，可以自动检测车辆前方是否存在楼梯，并返回楼梯的几何参数（高度、宽度、台阶数量等）。

## 核心功能

### 检测结果 (FStairDetectionResult)

当调用检测算法时，会返回以下信息：

- `bIsStair`: 是否检测到楼梯（true/false）
- `StepHeight`: 台阶高度（厘米）
- `StepWidth`: 台阶宽度/深度（厘米）
- `StepCount`: 检测到的台阶数量
- `SlopeAngle`: 楼梯倾斜角度（度）
- `StartLocation`: 楼梯起始位置（世界坐标）
- `EndLocation`: 楼梯结束位置（世界坐标）
- `Confidence`: 检测置信度（0.0 - 1.0）

### 检测配置 (FStairDetectionConfig)

可以通过配置参数调整检测行为：

- `DetectionRange`: 检测距离（默认 300cm）
- `TraceCount`: 射线数量（默认 7）
- `TraceSpacing`: 射线间距（默认 30cm）
- `MinStepHeight`: 最小台阶高度（默认 10cm）
- `MaxStepHeight`: 最大台阶高度（默认 50cm）
- `MinStepWidth`: 最小台阶宽度（默认 15cm）
- `MaxStepWidth`: 最大台阶宽度（默认 100cm）
- `HeightTolerance`: 高度容差（默认 5cm）
- `WidthTolerance`: 宽度容差（默认 10cm）
- `MinStepCount`: 最小台阶数量（默认 3）

## 使用方法

### 方法 1: 直接调用静态函数

```cpp
#include "StairDetection.h"

// 在你的代码中
FStairDetectionConfig Config;
Config.DetectionRange = 300.0f;
Config.MinStepCount = 3;

FVector StartLocation = GetActorLocation();
FVector ForwardDirection = GetActorForwardVector();

FStairDetectionResult Result = UStairDetection::DetectStairs(
    GetWorld(),
    StartLocation,
    ForwardDirection,
    Config
);

if (Result.bIsStair)
{
    UE_LOG(LogTemp, Log, TEXT("检测到楼梯！"));
    UE_LOG(LogTemp, Log, TEXT("台阶高度: %.1f cm"), Result.StepHeight);
    UE_LOG(LogTemp, Log, TEXT("台阶宽度: %.1f cm"), Result.StepWidth);
    UE_LOG(LogTemp, Log, TEXT("台阶数量: %d"), Result.StepCount);
}
else
{
    UE_LOG(LogTemp, Log, TEXT("未检测到楼梯"));
}
```

### 方法 2: 使用示例组件

1. 在你的 Actor 上添加 `UStairDetectionExample` 组件
2. 在编辑器中配置检测参数
3. 组件会自动定期检测并显示调试信息

```cpp
// 在 Blueprint 或 C++ 中获取检测结果
UStairDetectionExample* Detector = FindComponentByClass<UStairDetectionExample>();
if (Detector && Detector->IsStairDetected())
{
    FStairDetectionResult Result = Detector->GetLastDetectionResult();
    // 使用检测结果...
}
```

### 方法 3: Blueprint 使用

1. 在 Blueprint 中添加 `StairDetectionExample` 组件
2. 调用 `Perform Stair Detection` 节点
3. 从返回的结构体中读取 `Is Stair`、`Step Height`、`Step Width` 等信息

## 算法原理

### 1. 射线检测阶段

算法从车辆前方发射多条垂直向下的射线，扫描地面几何：

```
车辆位置
    |
    v
    ↓  ↓  ↓  ↓  ↓  ↓  ↓  (多条射线)
    ▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔
       ▔▔▔▔▔▔▔▔▔▔▔▔
          ▔▔▔▔▔▔▔▔
```

### 2. 台阶边缘提取

通过分析射线碰撞点的高度变化，识别台阶边缘：

- 如果相邻两个碰撞点的高度差 > 5cm，认为是台阶边缘
- 记录所有边缘点的位置

### 3. 几何分析

计算每个台阶的：
- 高度：相邻边缘点的垂直距离
- 宽度：相邻边缘点的水平距离

### 4. 模式验证

验证是否符合楼梯特征：
- 台阶高度是否一致（在容差范围内）
- 台阶宽度是否一致（在容差范围内）
- 台阶数量是否足够（≥ MinStepCount）
- 至少 70% 的台阶符合规律

### 5. 置信度计算

基于以下因素计算置信度：
- 台阶数量（30%）
- 一致性得分（50%）
- 几何合理性（20%）

## 返回值说明

### 如果不是楼梯

```cpp
Result.bIsStair = false;
Result.StepHeight = 0.0f;
Result.StepWidth = 0.0f;
Result.StepCount = 0;
Result.Confidence = 0.0f;
```

### 如果是楼梯

```cpp
Result.bIsStair = true;
Result.StepHeight = 25.0f;      // 例如：25cm
Result.StepWidth = 30.0f;       // 例如：30cm
Result.StepCount = 5;           // 例如：5个台阶
Result.SlopeAngle = 39.8f;      // 例如：39.8度
Result.Confidence = 0.85f;      // 例如：85% 置信度
```

## 调试可视化

启用 `bEnableDebugVisualization` 后，会在场景中绘制：

- 绿色球体：楼梯起始和结束位置
- 绿色线条：楼梯范围
- 黄色线条：检测范围
- 屏幕文字：检测结果详细信息

## 性能考虑

- 射线检测数量：默认 7 条，可根据需要调整
- 检测频率：建议 0.1-0.2 秒一次
- 碰撞通道：使用 `ECC_Visibility` 通道
- 复杂碰撞：启用 `bTraceComplex` 以获得准确结果

## 集成到现有系统

可以将此算法集成到 `FObstacleDetector` 中：

```cpp
// 在 C_TankMovementComponent 或 StairClimbingComponent 中
void UpdateObstacleDetection()
{
    FStairDetectionConfig Config;
    // 配置参数...
    
    FStairDetectionResult Result = UStairDetection::DetectStairs(
        GetWorld(),
        GetActorLocation(),
        GetActorForwardVector(),
        Config
    );
    
    if (Result.bIsStair)
    {
        // 触发爬楼梯逻辑
        OnStairDetected(Result);
    }
}
```

## 限制和注意事项

1. 算法假设楼梯是规则的（台阶高度和宽度基本一致）
2. 对于螺旋楼梯或不规则楼梯，检测效果可能不佳
3. 需要确保场景中的楼梯有正确的碰撞设置
4. 检测距离和射线数量会影响性能和准确度

## 示例场景

建议的测试场景设置：

1. 创建一个简单的楼梯（3-5 个台阶）
2. 台阶高度：20-30cm
3. 台阶宽度：25-35cm
4. 在车辆上添加 `StairDetectionExample` 组件
5. 启用调试可视化
6. 驾驶车辆接近楼梯，观察检测结果

## 未来改进方向

- 支持不规则楼梯检测
- 添加楼梯方向检测（上行/下行）
- 优化射线检测策略
- 支持曲线楼梯
- 添加更多调试可视化选项
