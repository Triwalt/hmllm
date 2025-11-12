
一个非常经典的静电学问题，它求解的是在接地导体板之间放置线电荷所激发的电势分布。我们将使用**本征函数展开法**（一种广义的分离变量法）来求解此问题。

### 问题分析（图 3.27）

1.  **物理模型：** 两块无限大、接地的平行导体板，分别位于 $y=0$ 和 $y=d$。板间为真空（介电常数为 $\epsilon_0$）。
2.  **电荷分布：** 一条平行于 $z$ 轴的无限长线电荷，线电荷密度为 $q_l$（图中也标为 $\rho_l$）。
3.  **电荷位置：** 题目文字描述为 $(0, d)$，这**是一个明显的印刷错误**，因为该位置在接地的上极板上。根据图示，线电荷应位于两板之间。我们设其坐标为 $(x_0, y_0) = (0, y_0)$，其中 $0 < y_0 < d$。（在您的第一张图中，这个位置被标为 $d'$，我们这里使用 $y_0$）。
4.  **求解目标：** 板间的电势分布 $\phi(x, y)$。

### 控制方程与边界条件

1.  **控制方程（泊松方程）：**
    由于板间存在自由电荷，电势 $\phi$ 满足**泊松方程 (Poisson's Equation)**：
    $$\nabla^2 \phi = \frac{\partial^2 \phi}{\partial x^2} + \frac{\partial^2 \phi}{\partial y^2} = -\frac{\rho(x, y)}{\epsilon_0}$$
    源项 $\rho(x, y)$ 是一个线电荷，用狄拉克 $\delta$ 函数表示为：
    $$\rho(x, y) = q_l \cdot \delta(x - 0) \cdot \delta(y - y_0) = q_l \delta(x) \delta(y - y_0)$$

2.  **边界条件 (BCs)：**
    * (BC 1) 下极板接地： $\phi(x, 0) = 0$
    * (BC 2) 上极板接地： $\phi(x, d) = 0$
    * (BC 3) 远离线电荷处电势为零： $\phi(x \to \pm \infty, y) = 0$

---

### 求解步骤（本征函数展开法）

当偏微分方程（PDE）是非齐次的（如泊松方程），但其边界条件在**某个维度**上是齐次的（本题中 $y$ 维度的边界为 $\phi=0$），我们使用本征函数展开法。

#### 步骤 1：寻找齐次边界方向的本征函数

* 我们先求解 $y$ 方向上的本征函数 $Y(y)$，它满足齐次方程 $Y'' + k^2 Y = 0$ 以及齐次边界 $Y(0)=0, Y(d)=0$。
* 通解为 $Y(y) = A \sin(ky) + B \cos(ky)$。
* 由 $Y(0)=0$ 得 $B=0$。
* 由 $Y(d)=0$ 得 $A \sin(kd)=0$，为得到非零解，必须有 $kd = n\pi$ ($n=1, 2, 3, \dots$)。
* **本征值：** $k_n = \frac{n\pi}{d}$
* **本征函数（正交基）：** $Y_n(y) = \sin\left(\frac{n\pi y}{d}\right)$

#### 步骤 2：将电势 $\phi$ 和源项 $\rho$ 按本征函数展开

1.  **展开电势 $\phi(x, y)$：**
    我们将待求电势 $\phi$ 展开为这组基函数的级数，系数 $X_n(x)$ 仅是 $x$ 的函数：
    $$\phi(x, y) = \sum_{n=1}^{\infty} X_n(x) \sin\left(\frac{n\pi y}{d}\right)$$
    这个形式自动满足了 $y=0$ 和 $y=d$ 处的接地边界条件。

2.  **展开源项 $\rho(x, y)$：**
    我们必须也将 $\delta$ 函数源 $\rho(x, y) = q_l \delta(x) \delta(y - y_0)$ 按这组基展开。
    $$\rho(x, y) = \sum_{n=1}^{\infty} \rho_n(x) \sin\left(\frac{n\pi y}{d}\right)$$
    其中系数 $\rho_n(x)$ 是：
    $$\rho_n(x) = \frac{2}{d} \int_0^d \rho(x, y) \sin\left(\frac{n\pi y}{d}\right) dy$$
    代入 $\rho(x, y)$：
    $$\rho_n(x) = \frac{2}{d} \int_0^d \left[ q_l \delta(x) \delta(y - y_0) \right] \sin\left(\frac{n\pi y}{d}\right) dy$$
    $\delta(x)$ 可提到积分外，利用 $\delta(y-y_0)$ 的筛选性质：
    $$\rho_n(x) = \frac{2 q_l \delta(x)}{d} \int_0^d \delta(y - y_0) \sin\left(\frac{n\pi y}{d}\right) dy$$
    $$\rho_n(x) = \left[ \frac{2 q_l}{d} \sin\left(\frac{n\pi y_0}{d}\right) \right] \delta(x)$$

#### 步骤 3：将 PDE 转化为 ODE

将 $\phi$ 和 $\rho$ 的展开式代入泊松方程 $\nabla^2 \phi = -\rho/\epsilon_0$：
$$\sum_{n=1}^{\infty} \left[ \frac{d^2 X_n}{d x^2} - \left(\frac{n\pi}{d}\right)^2 X_n \right] \sin\left(\frac{n\pi y}{d}\right) = -\frac{1}{\epsilon_0} \sum_{n=1}^{\infty} \rho_n(x) \sin\left(\frac{n\pi y}{d}\right)$$
由于 $\sin$ 基是正交的，我们只需比较每一项 $n$ 的系数：
$$\frac{d^2 X_n}{d x^2} - \left(\frac{n\pi}{d}\right)^2 X_n = -\frac{\rho_n(x)}{\epsilon_0}$$
代入 $\rho_n(x)$：
$$\frac{d^2 X_n}{d x^2} - \left(\frac{n\pi}{d}\right)^2 X_n = -\frac{2 q_l}{\epsilon_0 d} \sin\left(\frac{n\pi y_0}{d}\right) \delta(x)$$
这是一个带 $\delta$ 函数源的一维常微分方程（ODE）。

#### 步骤 4：求解 ODE

为方便，令 $k_n = \frac{n\pi}{d}$ 和 $C_n = \frac{2 q_l}{\epsilon_0 d} \sin\left(\frac{n\pi y_0}{d}\right)$。方程为：
$$\frac{d^2 X_n}{d x^2} - k_n^2 X_n = -C_n \delta(x)$$
1.  **当 $x \neq 0$ 时（齐次解）：**
    $X_n'' - k_n^2 X_n = 0$，通解为 $A e^{k_n x} + B e^{-k_n x}$。
    * 应用 (BC 3) $X_n(x \to \pm \infty) = 0$，解必须为：
        $$X_n(x) = D_n e^{-k_n |x|}$$
2.  **在 $x = 0$ 处（匹配条件）：**
    * **连续性：** 电势必须连续，$X_n(0^+) = X_n(0^-)$。$D_n e^{-k_n |x|}$ 自动满足。
    * **导数跳变：** 将 ODE 从 $x=-\epsilon$ 积分到 $x=+\epsilon$：
        $$\int_{-\epsilon}^{\epsilon} X_n'' dx - \int_{-\epsilon}^{\epsilon} k_n^2 X_n dx = \int_{-\epsilon}^{\epsilon} -C_n \delta(x) dx$$
        $$\left[ \frac{d X_n}{d x} \right]_{-\epsilon}^{\epsilon} - 0 = -C_n$$
        $$\frac{d X_n}{d x}\bigg|_{0^+} - \frac{d X_n}{d x}\bigg|_{0^-} = -C_n$$
    * 计算导数：
        * $x>0$: $\frac{d X_n}{d x} = -D_n k_n e^{-k_n x} \implies \frac{d X_n}{d x}\bigg|_{0^+} = -D_n k_n$
        * $x<0$: $\frac{d X_n}{d x} = D_n k_n e^{k_n x} \implies \frac{d X_n}{d x}\bigg|_{0^-} = D_n k_n$
    * 代入跳变条件：
        $$(-D_n k_n) - (D_n k_n) = -C_n$$
        $$-2 D_n k_n = -C_n \implies D_n = \frac{C_n}{2 k_n}$$
3.  **求解系数 $D_n$：**
    $$D_n = \frac{\frac{2 q_l}{\epsilon_0 d} \sin\left(\frac{n\pi y_0}{d}\right)}{2 \left(\frac{n\pi}{d}\right)} = \frac{q_l}{\epsilon_0 n \pi} \sin\left(\frac{n\pi y_0}{d}\right)$$
    因此，$X_n(x)$ 的解为：
    $$X_n(x) = \left[ \frac{q_l}{\epsilon_0 n \pi} \sin\left(\frac{n\pi y_0}{d}\right) \right] e^{-\frac{n\pi |x|}{d}}$$

#### 步骤 5：组合得到最终解

将 $X_n(x)$ 代回到 $\phi(x, y)$ 的级数展开式中：
$$\phi(x, y) = \sum_{n=1}^{\infty} X_n(x) \sin\left(\frac{n\pi y}{d}\right)$$
$$\phi(x, y) = \sum_{n=1}^{\infty} \left[ \frac{q_l}{\epsilon_0 n \pi} \sin\left(\frac{n\pi y_0}{d}\right) e^{-\frac{n\pi |x|}{d}} \right] \sin\left(\frac{n\pi y}{d}\right)$$

**最终答案：**
板间的电势分布为：
$$\phi(x, y) = \frac{q_l}{\pi \epsilon_0} \sum_{n=1}^{\infty} \frac{1}{n} e^{-\frac{n \pi |x|}{d}} \sin\left(\frac{n \pi y_0}{d}\right) \sin\left(\frac{n \pi y}{d}\right)$$

---

这个问题**可以使用镜像法求解**，而且这是一个非常经典的镜像法应用案例。

与单个接地平板（只需要一个镜像）不同，两块平行接地平板会产生“无穷多”个镜像，就像两面相对的镜子会产生无限影像一样。

### 镜像法求解思路

**1. 问题的核心：**
我们需要找到一个电荷分布，它与原始电荷 $q_l$ 组合在一起，能使 $y=0$ 和 $y=d$ 两个平面上的电势同时为零。

* **原始电荷：** $+q_l$ 位于 $(0, y_0)$。

**2. 镜像的产生（“镜中之镜”）：**

我们一步一步来看镜像是如何产生的：

* **步骤 1 (对 $y=0$ 平板成像)：**
    * 为了使 $\phi(y=0)=0$，原始电荷 $+q_l$（在 $y=y_0$）需要在 $y=0$ 平板“对面”产生一个镜像。
    * **镜像 1 (I1)：** $-q_l$ 位于 $(0, -y_0)$。

* **步骤 2 (对 $y=d$ 平板成像)：**
    * 现在 $y=d$ 平板看到了两个电荷：原始电荷 $+q_l$（在 $y_0$）和镜像 I1（在 $-y_0$）。
    * 原始电荷的镜像（**I2**）：$-q_l$ 位于 $y = d + (d-y_0) = 2d - y_0$。
    * I1 的镜像（**I3**）：$-(-q_l) = +q_l$ 位于 $y = d + (d-(-y_0)) = 2d + y_0$。

* **步骤 3 (对 $y=0$ 平板再次成像)：**
    * 现在 $y=0$ 平板又看到了新产生的 I2 和 I3。
    * I2 的镜像（**I4**）：$-(-q_l) = +q_l$ 位于 $y = -(2d-y_0) = y_0 - 2d$。
    * I3 的镜像（**I5**）：$-(+q_l) = -q_l$ 位于 $y = -(2d+y_0) = -2d - y_0$。

* **步骤 4 (对 $y=d$ 平板再次成像)：**
    * $y=d$ 平板又看到了新产生的 I4 和 I5...

这个过程会无限持续下去，产生一个**无限的镜像序列**。

**3. 镜像的规律总结：**

我们可以把这个无限序列归纳为两组：

* **正电荷 $+q_l$ 序列：**
    位于 $y = y_0 \pm 2nd$，其中 $n = 0, 1, 2, \dots$
    (即：$y_0$, $y_0+2d$, $y_0-2d$, $y_0+4d$, $y_0-4d$, ...)

* **负电荷 $-q_l$ 序列：**
    位于 $y = -y_0 \pm 2nd$，其中 $n = 0, 1, 2, \dots$
    (即：$-y_0$, $2d-y_0$, $-2d-y_0$, $4d-y_0$, $-4d-y_0$, ...)

**4. 求解电势 (叠加原理)：**

板间 $0 < y < d$ 区域内的电势，就是这个无限序列中**所有**电荷（包括原始电荷）产生的电势的**总和**。

* 单个线电荷 $q_i$ 位于 $(x_i, y_i)$ 产生的电势为：
    $$\phi_i(x, y) = -\frac{q_i}{2\pi\epsilon_0} \ln(R_i) + C$$
    其中 $R_i = \sqrt{(x-x_i)^2 + (y-y_i)^2}$ 是场点到该线电荷的距离。

* 总电势 $\phi(x,y)$ 为所有电荷的电势叠加（我们取无穷远处电势为零，常数 C 可以忽略，因为我们最终是成对组合的）：
    $$\phi(x, y) = \sum_{n=-\infty}^{\infty} \left[ \phi_{pos, n} + \phi_{neg, n} \right]$$
    $$\phi(x, y) = \sum_{n=-\infty}^{\infty} \left[ \left(-\frac{q_l}{2\pi\epsilon_0} \ln \sqrt{x^2 + (y - (y_0 + 2nd))^2}\right) + \left(\frac{q_l}{2\pi\epsilon_0} \ln \sqrt{x^2 + (y - (-y_0 + 2nd))^2}\right) \right]$$

* 整理后得到镜像法的最终解：
    $$\phi(x, y) = \frac{q_l}{2\pi\epsilon_0} \sum_{n=-\infty}^{\infty} \left[ \ln\left(\sqrt{x^2 + (y + y_0 - 2nd)^2}\right) - \ln\left(\sqrt{x^2 + (y - y_0 - 2nd)^2}\right) \right]$$
    $$\phi(x, y) = \frac{q_l}{4\pi\epsilon_0} \sum_{n=-\infty}^{\infty} \ln \left( \frac{x^2 + (y + y_0 - 2nd)^2}{x^2 + (y - y_0 - 2nd)^2} \right)$$

---

### 两种方法的关系

您现在有了求解这个问题的两种方法：

1.  **本征函数展开法（傅里叶级数法）：**
    $$\phi(x, y) = \frac{q_l}{\pi \epsilon_0} \sum_{n=1}^{\infty} \frac{1}{n} e^{-\frac{n \pi |x|}{d}} \sin\left(\frac{n \pi y_0}{d}\right) \sin\left(\frac{n \pi y}{d}\right)$$
2.  **镜像法（无穷级数求和法）：**
    $$\phi(x, y) = \frac{q_l}{4\pi\epsilon_0} \sum_{n=-\infty}^{\infty} \ln \left( \frac{x^2 + (y + y_0 - 2nd)^2}{x^2 + (y - y_0 - 2nd)^2} \right)$$

这两种方法得到的解**是完全等价的**。它们只是同一种物理势场的两种不同数学表达形式。

* **本征函数展开（傅里叶级数）**的解在 $x$ 很大时（远离线电荷）收敛很快。
* **镜像法（对数求和）**的解在 $x$ 很小时（靠近线电荷）收敛很快，因为此时电势主要由最近的几个电荷（原始电荷和最近的镜像）决定。

证明这两种形式的等价性是一个经典的高等数学练习，它展示了傅里叶级数（来自本征函数法）和一个无穷乘积（来自镜像法）之间的深刻联系。

我们将分三步完成证明：
1.  **简化 $\phi_1$（本征函数解）：** 将 $\sin \cdot \sin$ 级数求和，得到一个紧凑的对数（log）表达式。
2.  **简化 $\phi_2$（镜像法解）：** 将对数求和（$\sum \ln$）转化为一个无穷乘积（$\ln \prod$）的表达式。
3.  **证明恒等式：** 证明第 1 步和第 2 步得到的两个表达式是相等的，这需要用到一个来自复变分析的无穷乘积恒等式。

---

### 第 1 步：简化 $\phi_1$ (本征函数解)

我们从本征函数解开始：
$$\phi_1(x, y) = \frac{q_l}{\pi \epsilon_0} \sum_{n=1}^{\infty} \frac{1}{n} e^{-\frac{n \pi |x|}{d}} \sin\left(\frac{n \pi y_0}{d}\right) \sin\left(\frac{n \pi y}{d}\right)$$

**a) 使用三角恒等式：**
利用 $2 \sin(A) \sin(B) = \cos(A-B) - \cos(A+B)$：
$$\sin\left(\frac{n \pi y_0}{d}\right) \sin\left(\frac{n \pi y}{d}\right) = \frac{1}{2} \left[ \cos\left(\frac{n\pi(y-y_0)}{d}\right) - \cos\left(\frac{n\pi(y+y_0)}{d}\right) \right]$$

**b) 重写 $\phi_1$：**
$$\phi_1 = \frac{q_l}{2\pi \epsilon_0} \sum_{n=1}^{\infty} \frac{1}{n} e^{-\frac{n \pi |x|}{d}} \left[ \cos\left(\frac{n\pi(y-y_0)}{d}\right) - \cos\left(\frac{n\pi(y+y_0)}{d}\right) \right]$$
$$\phi_1 = \frac{q_l}{2\pi \epsilon_0} \left[ \sum_{n=1}^{\infty} \frac{e^{-n\alpha}}{n} \cos(n\beta) - \sum_{n=1}^{\infty} \frac{e^{-n\alpha}}{n} \cos(n\gamma) \right]$$
其中 $\alpha = \frac{\pi|x|}{d}$， $\beta = \frac{\pi(y-y_0)}{d}$， $\gamma = \frac{\pi(y+y_0)}{d}$。

**c) 使用幂级数求和恒等式：**
我们使用一个标准的幂级数求和公式（由 $-\ln(1-z)$ 的泰勒展开的实部导出）：
$$\sum_{n=1}^{\infty} \frac{r^n}{n} \cos(n\theta) = -\frac{1}{2} \ln(1 - 2r\cos\theta + r^2)$$
将 $r = e^{-\alpha}$ 和 $\theta = \beta$ 或 $\gamma$ 代入：
* $\sum_{n=1}^{\infty} \frac{e^{-n\alpha}}{n} \cos(n\beta) = -\frac{1}{2} \ln(1 - 2e^{-\alpha}\cos\beta + e^{-2\alpha})$
* $\sum_{n=1}^{\infty} \frac{e^{-n\alpha}}{n} \cos(n\gamma) = -\frac{1}{2} \ln(1 - 2e^{-\alpha}\cos\gamma + e^{-2\alpha})$

**d) 代回 $\phi_1$：**
$$\phi_1 = \frac{q_l}{2\pi \epsilon_0} \left[ -\frac{1}{2} \ln(1 - 2e^{-\alpha}\cos\beta + e^{-2\alpha}) - \left(-\frac{1}{2} \ln(1 - 2e^{-\alpha}\cos\gamma + e^{-2\alpha})\right) \right]$$
$$\phi_1 = \frac{q_l}{4\pi \epsilon_0} \left[ \ln(1 - 2e^{-\alpha}\cos\gamma + e^{-2\alpha}) - \ln(1 - 2e^{-\alpha}\cos\beta + e^{-2\alpha}) \right]$$

**e) 进一步简化：**
利用 $1 - 2e^{-\alpha}\cos\gamma + e^{-2\alpha} = e^{-\alpha}(e^{\alpha} - 2\cos\gamma + e^{-\alpha}) = 2e^{-\alpha}(\cosh(\alpha) - \cos(\gamma))$。
$$\phi_1 = \frac{q_l}{4\pi \epsilon_0} \left[ \ln(2e^{-\alpha}(\cosh(\alpha) - \cos(\gamma))) - \ln(2e^{-\alpha}(\cosh(\alpha) - \cos(\beta))) \right]$$
$$\phi_1 = \frac{q_l}{4\pi \epsilon_0} \left[ \ln(2e^{-\alpha}) + \ln(\cosh(\alpha) - \cos(\gamma)) - \ln(2e^{-\alpha}) - \ln(\cosh(\alpha) - \cos(\beta)) \right]$$
$$\phi_1(x, y) = \frac{q_l}{4\pi \epsilon_0} \ln \left( \frac{\cosh(\alpha) - \cos(\gamma)}{\cosh(\alpha) - \cos(\beta)} \right)$$

代回 $\alpha, \beta, \gamma$，我们得到 $\phi_1$ 的紧凑形式：
$$\phi_1(x, y) = \frac{q_l}{4\pi \epsilon_0} \ln \left( \frac{\cosh\left(\frac{\pi |x|}{d}\right) - \cos\left(\frac{\pi (y+y_0)}{d}\right)}{\cosh\left(\frac{\pi |x|}{d}\right) - \cos\left(\frac{\pi (y-y_0)}{d}\right)} \right)$$

---

### 第 2 步：简化 $\phi_2$ (镜像法解)

我们从镜像法解开始：
$$\phi_2(x, y) = \frac{q_l}{4\pi\epsilon_0} \sum_{n=-\infty}^{\infty} \ln \left( \frac{x^2 + (y + y_0 - 2nd)^2}{x^2 + (y - y_0 - 2nd)^2} \right)$$
利用对数性质 $\sum \ln(A_n) = \ln(\prod A_n)$：
$$\phi_2(x, y) = \frac{q_l}{4\pi\epsilon_0} \ln \left[ \prod_{n=-\infty}^{\infty} \frac{x^2 + (y + y_0 - 2nd)^2}{x^2 + (y - y_0 - 2nd)^2} \right]$$

---

### 第 3 步：证明恒等式

现在，我们的任务是证明这两种形式的**对数内部参数**是相等的：
$$\frac{\cosh\left(\frac{\pi |x|}{d}\right) - \cos\left(\frac{\pi (y+y_0)}{d}\right)}{\cosh\left(\frac{\pi |x|}{d}\right) - \cos\left(\frac{\pi (y-y_0)}{d}\right)} \stackrel{?}{=} \prod_{n=-\infty}^{\infty} \frac{x^2 + (y + y_0 - 2nd)^2}{x^2 + (y - y_0 - 2nd)^2}$$

为了证明这一点，我们引入一个来自复变分析的**无穷乘积展开恒等式**（由 $\sinh$ 函数的魏尔施特拉斯（Weierstrass）因子分解导出）：
$$\cosh(A) - \cos(B) = C \cdot \prod_{n=-\infty}^{\infty} \left[ A^2 + (B - 2n\pi)^2 \right]$$
其中 $C$ 是一个不依赖于 $A$ 或 $B$ 的常数 ( $C = 1/(2\pi^2)$，但它的具体值不重要，因为它会被约掉)。

现在我们来处理无穷乘积（RHS）：
$$\text{RHS} = \prod_{n=-\infty}^{\infty} \frac{x^2 + (y + y_0 - 2nd)^2}{x^2 + (y - y_0 - 2nd)^2}$$
我们可以在分子和分母上同乘以 $(\pi/d)^2$：
$$\text{RHS} = \prod_{n=-\infty}^{\infty} \frac{\left(\frac{\pi}{d}\right)^2 \left[ x^2 + (y + y_0 - 2nd)^2 \right]}{\left(\frac{\pi}{d}\right)^2 \left[ x^2 + (y - y_0 - 2nd)^2 \right]}$$
$$\text{RHS} = \prod_{n=-\infty}^{\infty} \frac{\left(\frac{\pi x}{d}\right)^2 + \left(\frac{\pi(y + y_0)}{d} - 2n\pi \right)^2}{\left(\frac{\pi x}{d}\right)^2 + \left(\frac{\pi(y - y_0)}{d} - 2n\pi \right)^2}$$

现在我们进行变量替换：
* $A = \frac{\pi |x|}{d}$ (由于 $x^2$， $|x|$ 的符号不重要)
* $B_s = \frac{\pi (y+y_0)}{d}$ (s for "sum")
* $B_d = \frac{\pi (y-y_0)}{d}$ (d for "difference")

RHS 变为：
$$\text{RHS} = \prod_{n=-\infty}^{\infty} \frac{A^2 + (B_s - 2n\pi)^2}{A^2 + (B_d - 2n\pi)^2}$$
RHS = $\frac{\prod_{n=-\infty}^{\infty} \left[ A^2 + (B_s - 2n\pi)^2 \right]}{\prod_{n=-\infty}^{\infty} \left[ A^2 + (B_d - 2n\pi)^2 \right]}$

现在，我们应用上面的无穷乘积恒等式：
* $\prod_{n=-\infty}^{\infty} \left[ A^2 + (B_s - 2n\pi)^2 \right] = \frac{1}{C} \left( \cosh(A) - \cos(B_s) \right)$
* $\prod_{n=-\infty}^{\infty} \left[ A^2 + (B_d - 2n\pi)^2 \right] = \frac{1}{C} \left( \cosh(A) - \cos(B_d) \right)$

代回 RHS 表达式：
$$\text{RHS} = \frac{ (1/C) \left( \cosh(A) - \cos(B_s) \right) }{ (1/C) \left( \cosh(A) - \cos(B_d) \right) } = \frac{\cosh(A) - \cos(B_s)}{\cosh(A) - \cos(B_d)}$$

这正是我们从 $\phi_1$ 导出的 LHS（紧凑形式的对数参数）。

### 结论

我们已经证明：
$$\phi_1 = \frac{q_l}{4\pi \epsilon_0} \ln(\text{LHS}) \quad \text{且} \quad \phi_2 = \frac{q_l}{4\pi \epsilon_0} \ln(\text{RHS})$$
并且我们通过一个标准的无穷乘积恒等式证明了：
$$\text{LHS} = \text{RHS}$$
因此，$\phi_1(x, y) = \phi_2(x, y)$。

**本征函数展开法（傅里叶级数）和镜像法（无穷求和）所得到的解是完全等价的。** 它们只是同一种物理势场的两种不同的数学表达形式。
