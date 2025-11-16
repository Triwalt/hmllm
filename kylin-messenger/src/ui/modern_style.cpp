#include "ui/modern_style.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QColor>

namespace KylinMessenger::UI {

namespace {
QPalette buildPalette()
{
    QPalette palette;
    const QColor window(32, 34, 40);
    const QColor windowText(230, 233, 239);
    const QColor base(24, 26, 30);
    const QColor alternateBase(40, 43, 49);
    const QColor highlight(76, 163, 224);
    const QColor onHighlight(255, 255, 255);
    const QColor disabledText(125, 130, 140);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, windowText);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternateBase);
    palette.setColor(QPalette::ToolTipBase, windowText);
    palette.setColor(QPalette::ToolTipText, window);
    palette.setColor(QPalette::Text, windowText);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    palette.setColor(QPalette::Button, window);
    palette.setColor(QPalette::ButtonText, windowText);
    palette.setColor(QPalette::BrightText, QColor(Qt::red));
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, onHighlight);

    return palette;
}
}

void applyModernStyle(QApplication& app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setPalette(buildPalette());
    app.setStyleSheet(QStringLiteral(
        "QToolTip {"
        "  color: #0f1114;"
        "  background-color: #e7eaef;"
        "  border: 1px solid #4ca3e0;"
        "  padding: 4px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton {"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "}"
        "QMenu {"
        "  border: 1px solid #3b3f46;"
        "}"
        ));
}

} // namespace KylinMessenger::UI
