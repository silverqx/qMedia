#pragma once
#ifndef MAINEVENTFILTER_H
#define MAINEVENTFILTER_H

#include <QAbstractNativeEventFilter>

class MainWindow;

class MainEventFilter final : public QAbstractNativeEventFilter
{
public:
    explicit MainEventFilter(MainWindow *mainWindow);

    bool nativeEventFilter(
            const QByteArray &eventType, void *message, long *result) final; // NOLINT(google-runtime-int)

private:
    MainWindow *const m_mainWindow;
};

#endif // MAINEVENTFILTER_H
