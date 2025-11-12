#include <QtTest>

#include "../include/main_window.h"

using namespace KylinMessenger;

class UISmokeTest : public QObject
{
    Q_OBJECT

private slots:
    void canInstantiateMainWindow()
    {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(window.isVisible());
    }
};

QTEST_MAIN(UISmokeTest)
#include "ui_smoke_test.moc"

