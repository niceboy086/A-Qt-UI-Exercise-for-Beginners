# 嵌入式滤波与 RMS 计算最佳实践

在嵌入式场景下计算采样点序列的有效电压（RMS），**滤波是必要的，但需根据实际场景选择合适的滤波方式**—— 未滤波的 ADC 采样值会包含硬件噪声、电源纹波、电磁干扰等干扰，直接计算会导致 RMS 值波动大、精度低；但过度滤波又会引入延迟，甚至失真（如滤掉有效信号）。

以下从「为什么要滤波」「嵌入式适配的滤波方案」「滤波与 RMS 计算的结合方式」「实操注意事项」四个维度详细说明：

### 一、为什么嵌入式计算 RMS 必须考虑滤波？

嵌入式系统中 ADC 采样的原始数据存在典型干扰：



| 干扰类型     | 表现形式               | 对 RMS 的影响                      |
| -------- | ------------------ | ------------------------------ |
| 高频噪声     | 采样值随机跳变 ±1\~±5 LSB | RMS 值频繁波动（如 220V 测成 215\~225V） |
| 电源纹波     | 50/100Hz 低频波动      | RMS 值整体偏移（如偏置 + 2V）            |
| 尖峰干扰     | 单次超大 / 超小采样值       | 单次异常值导致 RMS 瞬间跳变               |
| ADC 量化噪声 | 低位随机抖动（如 12 位 ADC） | 小电压下 RMS 精度显著下降                |

**示例**：未滤波时，12 位 ADC 采样 220V 交流电（分压后 22V），原始值可能在`2760~2770`（对应 21.9\~22.1V）随机跳变，直接计算 RMS 会出现 ±0.5V 以上的波动；滤波后可稳定在 ±0.1V 内。

### 二、嵌入式适配的滤波方案（轻量化、低延迟）

嵌入式 MCU 资源有限（无大内存、算力弱），需选择**计算量小、内存占用 O (1)、延迟低**的滤波算法，优先以下 3 种：

#### 1. 一阶低通数字滤波（最常用）



* **原理**：用当前采样值和上一次滤波值加权平均，平滑噪声：

  $V_{filter} = \alpha \times V_{raw} + (1-\alpha) \times V_{filter\_last}$

  其中：$\alpha$为滤波系数（0<α<1，α 越小滤波越强，延迟越大），建议取`0.1~0.3`（兼顾平滑与实时）。

* **优势**：仅需保存上一次滤波值，计算仅 2 次乘法 + 1 次加法，耗时＜1μs，适配所有 MCU。

* **嵌入式代码实现**：



```
// 一阶低通滤波（全局/结构体保存上一次滤波值，避免重复初始化）

float first\_order\_filter(float raw\_volt, float alpha) {

&#x20;   static float last\_filtered = 0.0f; // 静态变量保存上一次结果

&#x20;   // 首次采样直接赋值，避免初始0值影响

&#x20;   if (fabs(last\_filtered) < 0.001f) {

&#x20;       last\_filtered = raw\_volt;

&#x20;       return raw\_volt;

&#x20;   }

&#x20;   // 滤波计算

&#x20;   float filtered = alpha \* raw\_volt + (1 - alpha) \* last\_filtered;

&#x20;   last\_filtered = filtered;

&#x20;   return filtered;

}
```

#### 2. 滑动平均滤波（适合高频噪声）



* **原理**：取最近 N 个采样值的平均值（N=4\~16，不宜过大），滤除随机高频噪声。

* **嵌入式优化**：用「环形缓冲区」实现，无需移位操作，内存占用仅 N 个 float（如 N=8 时占 32 字节）：



```
\#define AVG\_FILTER\_LEN 8  // 滑动平均长度（2的幂次，便于除法优化）

typedef struct {

&#x20;   float buf\[AVG\_FILTER\_LEN];

&#x20;   uint8\_t idx;    // 当前缓冲区索引

&#x20;   uint8\_t count;  // 已填充的采样数

&#x20;   float sum;      // 缓冲区总和（避免每次求和，减少计算）

} AvgFilter\_t;

// 初始化滑动平均滤波器

void avg\_filter\_init(AvgFilter\_t \*filter) {

&#x20;   if (filter == NULL) return;

&#x20;   for (uint8\_t i=0; i\<AVG\_FILTER\_LEN; i++) filter->buf\[i] = 0.0f;

&#x20;   filter->idx = 0;

&#x20;   filter->count = 0;

&#x20;   filter->sum = 0.0f;

}

// 滑动平均滤波计算

float avg\_filter\_update(AvgFilter\_t \*filter, float raw\_volt) {

&#x20;   if (filter == NULL) return raw\_volt;

&#x20;   // 减去即将被覆盖的旧值

&#x20;   filter->sum -= filter->buf\[filter->idx];

&#x20;   // 存入新值

&#x20;   filter->buf\[filter->idx] = raw\_volt;

&#x20;   filter->sum += raw\_volt;

&#x20;   // 更新索引（环形）

&#x20;   filter->idx = (filter->idx + 1) % AVG\_FILTER\_LEN;

&#x20;   // 未填满时按实际数量平均，填满后按固定长度平均

&#x20;   if (filter->count < AVG\_FILTER\_LEN) filter->count++;

&#x20;   return filter->sum / filter->count;

}
```



* **优势**：对高频随机噪声滤波效果优于一阶低通；

* **注意**：N 不宜超过 16（否则延迟增大，如 N=32 时，50Hz 采样会引入 0.64ms 延迟）。

#### 3. 限幅滤波（先滤尖峰，再平滑）



* **原理**：先判断当前采样值与上一次的差值是否超过阈值（如 ±5%），超过则视为尖峰干扰，丢弃并沿用上次值；未超过则正常参与后续滤波。

* **嵌入式代码实现**：



```
// 限幅滤波（阈值为电压最大值的5%，如50V量程则阈值=2.5V）

float limit\_filter(float raw\_volt, float last\_volt, float limit\_thresh) {

&#x20;   if (fabs(raw\_volt - last\_volt) > limit\_thresh) {

&#x20;       return last\_volt; // 超阈值，沿用上次值

&#x20;   }

&#x20;   return raw\_volt; // 正常，返回原始值

}
```



* **适用场景**：工业现场（如电机、变频器干扰导致的尖峰采样值），建议与一阶低通 / 滑动平均组合使用（先限幅，后平滑）。

### 三、滤波与 RMS 计算的结合方式（嵌入式最佳实践）

#### 1. 推荐流程（兼顾效率与精度）



```
ADC原始值 → 限幅滤波（滤尖峰） → 一阶低通滤波（平滑噪声） → 递推式RMS计算
```



* **代码整合示例**：



```
// 全局/结构体保存滤波器上下文

RmsCalculator\_t rms\_calc;

AvgFilter\_t avg\_filter;

float last\_volt = 0.0f; // 限幅滤波用的上次值

// 单次采样+滤波+RMS计算

float adc\_sample\_and\_rms(uint16\_t adc\_raw) {

&#x20;   // 1. ADC原始值转实际电压

&#x20;   float raw\_volt = (float)adc\_raw \* ADC\_REF\_VOLTAGE / ADC\_MAX\_VALUE \* VOLTAGE\_SCALE;

&#x20;  &#x20;

&#x20;   // 2. 限幅滤波（阈值=最大量程的5%，如50V则2.5V）

&#x20;   float limit\_volt = limit\_filter(raw\_volt, last\_volt, 2.5f);

&#x20;   last\_volt = limit\_volt;

&#x20;  &#x20;

&#x20;   // 3. 一阶低通滤波（α=0.2，兼顾平滑与实时）

&#x20;   float filtered\_volt = first\_order\_filter(limit\_volt, 0.2f);

&#x20;  &#x20;

&#x20;   // 4. 递推式RMS计算

&#x20;   float rms\_volt = rms\_calc\_update(\&rms\_calc, filtered\_volt);

&#x20;  &#x20;

&#x20;   return rms\_volt;

}
```

#### 2. 特殊场景的简化 / 增强



| 场景          | 滤波方案                 | 说明                 |
| ----------- | -------------------- | ------------------ |
| 低噪声场景（实验室）  | 仅一阶低通（α=0.3）         | 减少计算量，满足精度         |
| 高干扰场景（工业）   | 限幅 + 滑动平均（N=8）+ 一阶低通 | 先滤尖峰，再平高频噪声，最后平滑波动 |
| 低速采样（<10Hz） | 无需滤波（或仅轻度过一阶低通）      | 采样频率低，噪声占比小        |
| 高速采样（>1kHz） | 滑动平均（N=4）+ 一阶低通      | 优先滤高频噪声，再平滑低频波动    |

### 四、嵌入式滤波的关键注意事项

#### 1. 避免过度滤波



* 滤波系数 α 过小（如 α=0.05）或滑动平均长度 N 过大（如 N=32），会导致：


  * 信号延迟增大（如 50Hz 采样，N=32 时延迟 0.64ms）；

  * 动态电压变化时，RMS 值更新滞后（如电压从 220V 降到 200V，滤波后需多次采样才会响应）。

* **调优建议**：


  * 先测试无滤波时的噪声幅度，再逐步降低 α/ 增大 N，直到 RMS 波动≤±0.5%；

  * 动态场景（如电压快速变化）优先用 α=0.2\~0.3 的一阶低通，而非长滑动平均。

#### 2. 无 FPU MCU 的滤波优化

若 MCU 无硬件 FPU（如 STM32F103、STM8），需将浮点运算转为整数运算（放大 1000 倍，单位 mV）：



```
// 整数版一阶低通滤波（电压单位：mV，α=0.2→用整数比例1/5）

uint32\_t first\_order\_filter\_int(uint32\_t raw\_mv) {

&#x20;   static uint32\_t last\_filtered\_mv = 0;

&#x20;   if (last\_filtered\_mv == 0) {

&#x20;       last\_filtered\_mv = raw\_mv;

&#x20;       return raw\_mv;

&#x20;   }

&#x20;   // 等价于：0.2\*raw + 0.8\*last → (raw + 4\*last)/5（避免浮点）

&#x20;   uint32\_t filtered\_mv = (raw\_mv + 4 \* last\_filtered\_mv) / 5;

&#x20;   last\_filtered\_mv = filtered\_mv;

&#x20;   return filtered\_mv;

}
```

#### 3. 滤波与 RMS 的精度平衡



* 滤波仅需滤除「无意义的噪声」，无需追求 “绝对平滑”——RMS 本身是统计值，少量噪声会被均方根计算自然平均；

* 若采样点数足够（如每次 RMS 计算取 100 个采样点），可适当降低滤波强度（如 α=0.3），减少滤波引入的失真。

#### 4. 滤波参数的动态调整（进阶）

对电压变化快的场景（如电机启动），可动态调整滤波系数：



```
// 动态调整α：电压变化率大时增大α（减弱滤波），变化小时减小α（增强滤波）

float dynamic\_alpha(float raw\_volt, float last\_volt) {

&#x20;   float delta = fabs(raw\_volt - last\_volt);

&#x20;   if (delta > 1.0f) return 0.5f;  // 变化大，减弱滤波

&#x20;   else if (delta > 0.2f) return 0.3f; // 中等变化，默认滤波

&#x20;   else return 0.1f; // 变化小，增强滤波

}
```

### 五、总结



1. **是否需要滤波**：嵌入式场景下**必须滤波**，至少做一阶低通滤波，否则 RMS 值精度和稳定性无法满足实际使用；

2. **优先选择**：无特殊需求时，「限幅滤波 + 一阶低通滤波」是最优组合（内存占用最小、计算最快、适配所有 MCU）；

3. **核心原则**：滤波的目标是 “去除噪声，保留有效信号”，而非 “让采样值绝对平滑”，需根据实际噪声情况调优参数，兼顾精度与实时性。

> （注：文档部分内容可能由 AI 生成）