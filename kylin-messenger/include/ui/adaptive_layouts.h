/**
 * @file adaptive_layouts.h
 * @brief 自适应布局组件
 * @version 1.0.0
 */

#ifndef KYLIN_MESSENGER_UI_ADAPTIVE_LAYOUTS_H
#define KYLIN_MESSENGER_UI_ADAPTIVE_LAYOUTS_H

#include <QWidget>
#include <QSplitter>
#include <QStackedWidget>
#include <QMap>
#include <QRect>
#include <memory>

namespace KylinMessenger::UI {

/**
 * @brief 屏幕尺寸分类
 */
enum class ScreenSize {
    Small,      // < 768px (手机)
    Medium,     // 768px - 1024px (平板)
    Large,      // 1024px - 1440px (笔记本)
    XLarge      // > 1440px (桌面)
};

/**
 * @brief 自适应分割器
 * 根据屏幕尺寸自动调整布局
 */
class AdaptiveSplitter : public QSplitter {
    Q_OBJECT

public:
    explicit AdaptiveSplitter(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~AdaptiveSplitter() override;

    /**
     * @brief 设置不同屏幕尺寸下的比例
     * @param size 屏幕尺寸
     * @param ratio 分割比例 (0.0 - 1.0)
     */
    void setSizeRatio(ScreenSize size, float ratio);

    /**
     * @brief 获取当前比例
     */
    float currentRatio() const;

    /**
     * @brief 启用/禁用自适应
     */
    void setAdaptiveEnabled(bool enabled) { adaptiveEnabled_ = enabled; }

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    bool adaptiveEnabled_ = true;
    QMap<ScreenSize, float> sizeRatios_;
    ScreenSize currentScreenSize_ = ScreenSize::Large;

    ScreenSize detectScreenSize() const;
    void applySizeRatio(ScreenSize size);
    float getSizeRatio(ScreenSize size) const;
};

/**
 * @brief 自适应堆叠窗口
 * 在小屏幕上自动切换为全屏模式
 */
class AdaptiveStackedWidget : public QStackedWidget {
    Q_OBJECT

public:
    explicit AdaptiveStackedWidget(QWidget* parent = nullptr);
    ~AdaptiveStackedWidget() override;

    /**
     * @brief 添加自适应页面
     * @param widget 页面组件
     * @param smallScreenWidget 小屏幕专用组件（可选）
     */
    void addAdaptivePage(QWidget* widget, QWidget* smallScreenWidget = nullptr);

    /**
     * @brief 设置当前页面
     * @param index 页面索引
     */
    void setCurrentIndex(int index) override;

    /**
     * @brief 获取当前页面
     */
    int currentIndex() const override;

    /**
     * @brief 启用/禁用自适应
     */
    void setAdaptiveEnabled(bool enabled) { adaptiveEnabled_ = enabled; }

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    struct PageInfo {
        QWidget* normalWidget = nullptr;
        QWidget* smallScreenWidget = nullptr;
        QWidget* currentWidget = nullptr;
    };

    bool adaptiveEnabled_ = true;
    QVector<PageInfo> pages_;
    QMap<QWidget*, int> widgetIndexMap_;

    ScreenSize detectScreenSize() const;
    void updatePageDisplay(int index, ScreenSize size);
    void switchToSmallScreenMode();
    void switchToNormalMode();
};

/**
 * @brief 响应式网格布局
 * 根据宽度自动调整列数
 */
class ResponsiveGridLayout : public QWidget {
    Q_OBJECT

public:
    explicit ResponsiveGridLayout(QWidget* parent = nullptr);
    ~ResponsiveGridWidget() override;

    /**
     * @brief 添加子组件
     * @param widget 子组件
     */
    void addWidget(QWidget* widget);

    /**
     * @brief 移除子组件
     * @param widget 子组件
     */
    void removeWidget(QWidget* widget);

    /**
     * @brief 设置间距
     * @param spacing 间距（像素）
     */
    void setSpacing(int spacing) { spacing_ = spacing; updateLayout(); }

    /**
     * @brief 设置最小项宽度
     * @param width 最小宽度（像素）
     */
    void setMinItemWidth(int width) { minItemWidth_ = width; updateLayout(); }

    /**
     * @brief 设置最大列数
     * @param maxColumns 最大列数
     */
    void setMaxColumns(int maxColumns) { maxColumns_ = maxColumns; updateLayout(); }

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    int spacing_ = 10;
    int minItemWidth_ = 200;
    int maxColumns_ = 6;
    QVector<QWidget*> widgets_;

    void updateLayout();
    int calculateColumnCount() const;
    void repositionWidgets();
};

/**
 * @brief 折叠面板
 * 在小屏幕上自动折叠
 */
class CollapsiblePanel : public QWidget {
    Q_OBJECT

public:
    explicit CollapsiblePanel(const QString& title, QWidget* parent = nullptr);
    ~CollapsiblePanel() override;

    /**
     * @brief 设置内容组件
     * @param widget 内容组件
     */
    void setContent(QWidget* widget);

    /**
     * @brief 获取内容组件
     */
    QWidget* content() const { return content_; }

    /**
     * @brief 展开/折叠
     * @param collapsed 是否折叠
     */
    void setCollapsed(bool collapsed);

    /**
     * @brief 是否折叠
     */
    bool isCollapsed() const { return collapsed_; }

    /**
     * @brief 启用/禁用自动折叠
     * @param enabled 是否启用
     */
    void setAutoCollapseEnabled(bool enabled) { autoCollapseEnabled_ = enabled; }

signals:
    void collapsedChanged(bool collapsed);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    bool collapsed_ = false;
    bool autoCollapseEnabled_ = true;
    QWidget* content_ = nullptr;
    QWidget* header_ = nullptr;

    void toggleCollapsed();
    void updateVisibility();
    ScreenSize detectScreenSize() const;
};

} // namespace KylinMessenger::UI

#endif // KYLIN_MESSENGER_UI_ADAPTIVE_LAYOUTS_H