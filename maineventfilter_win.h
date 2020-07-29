#ifndef MAINEVENTFILTER_H
#define MAINEVENTFILTER_H

#include <QAbstractNativeEventFilter>

class MainWindow;

class MainEventFilter final : public QAbstractNativeEventFilter
{
public:
    explicit MainEventFilter(MainWindow *const mainWindow);

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

private:
    MainWindow *const m_mainWindow = nullptr;
};

#endif // MAINEVENTFILTER_H
