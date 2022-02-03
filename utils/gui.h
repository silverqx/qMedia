#pragma once
#ifndef UTILS_GUI_H
#define UTILS_GUI_H

#include <QtGlobal>

class QString;
class QWidget;

namespace Utils {

    /*! Gui library class. */
    class Gui
    {
        Q_DISABLE_COPY(Gui)

    public:
        /*! Deleted default constructor, this is a pure library class. */
        Gui() = delete;
        /*! Deleted destructor. */
        ~Gui() = delete;

        /*! Open the given path with an appropriate application. */
        static bool openPath(const QString &absolutePath);
        /*! Center on main desktop screen. */
        static void centerDialog(QWidget *widget);
    };

} // namespace Utils

#endif // UTILS_GUI_H
