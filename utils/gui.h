#ifndef GUI_H
#define GUI_H

class QString;
class QWidget;

namespace Utils {
    namespace Gui {
        /*! Open the given path with an appropriate application. */
        bool openPath(const QString &absolutePath);
        /*! Center on main desktop screen. */
        void centerDialog(QWidget *const widget);
    }
}

#endif // GUI_H
